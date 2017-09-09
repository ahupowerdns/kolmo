#include "kolmoconf.hh"
#include "http.hh"
#include <crow/json.h>
#include "minicurl.hh"
#include "CLI/CLI.hpp"
#include <boost/format.hpp>
#include "kolmoweb.hh"
#include <thread>

using namespace std;

static string getTimeSuffix()
{
  char buf[20];
  time_t tdate = time(0);
  struct tm date_tm;

  localtime_r(&tdate, &date_tm);
  strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", &date_tm);

  return string(buf);
}

void syncConfig(const KolmoConf& kc, const std::string& configfile)
{
  string newname=configfile+"."+getTimeSuffix();
  {
    ofstream ofs(newname);
    if(!ofs)
      throw std::runtime_error("Could not write new configuration file '"+newname+"'");
    nlohmann::json wv;
    auto minimal=kc.getMinimalConfig();
    KSToJson(minimal.get(), wv);
    ofs << std::setw(4) << wv;
  }
  unlink(configfile.c_str());
  if(symlink(newname.c_str(), configfile.c_str()) < 0)
    throw std::runtime_error("Unable to change symlink for configuration file: "+string(strerror(errno)));
}

void markdownStruct(const std::string& name, const KolmoStruct* ks, int depth=2)
{
  if(ks->getAll().empty())
    return;
  cout<<string(depth, '#')<<" "<<name<<" members"<<endl;
  cout<<ks->description<<endl;
  cout<<"Variable | Type | Default | Runtime | Mandatory | Description"<<endl;
  cout<<"---------|------|---------|---------|-----------| -----------"<<endl;

  for(const auto& m : ks->getAll()) {
    cout<<m.first<< " | " << m.second->getTypename()<< " | "<< m.second->defaultValue <<" | "<<(m.second->runtime ? " true " : "false")<<" | "<<(m.second->mandatory ? " true " : "false") << " | " <<m.second->description<<endl;
  }
  for(const auto& m : ks->getAll()) {
    if(auto ptr = dynamic_cast<const KolmoStruct*>(m.second))
      markdownStruct(m.first, ptr, depth+1);
  }
}

void markdown(const KolmoConf& kc)
{
  cout<<R"(                <meta charset="utf-8" emacsmode="-*- markdown -*-">
                            **Automatically generated Kolmo configuration for x**)"<<endl;

  cout<<"# Markdown documentation for x"<<endl;
  cout<<"## Classes used in configuration"<<endl;
  for(const auto& p : d_prototypes) {
    if(p.first == "struct")
      continue;
    if(auto ptr=dynamic_cast<KolmoStruct*>(p.second.get())) {
      cout<<"### "<<p.first<<endl;
      cout<<ptr->description<<endl;
      cout<<"Variable | Type | Default | Runtime | Mandatory | Description"<<endl;
      cout<<"---------|------|---------|---------|-----------| -----------"<<endl;

      for(const auto& m : ptr->getAll()) {
        cout<<m.first<< " | " << m.second->getTypename()<< " | "<< m.second->defaultValue <<" | "<<(m.second->runtime ? " true " : "false")<<" | "<<(m.second->mandatory ? " true " : "false") << " | " <<m.second->description<<endl;
      }
    }
  }
  cout<<"# Configuration"<<endl;
  markdownStruct("main", &kc.d_main,2);
  cout<<R"(<!-- Markdeep: --><style class="fallback">body{visibility:hidden;white-space:pre;font-family:monospace}</style><script src="markdeep.min.js"></script><script 	src="https://casual-effects.com/markdeep/latest/markdeep.min.js"></script><script>window.alreadyProcessedMarkdeep||(document.body.style.visibility="visible")</script>)"<<endl;
}


int main(int argc, char** argv)
try
{
  CLI::App app("kolctl");

  vector<std::string> cmds;
  string setstring, remote, schemafile, configfile;
  bool webserver;
  app.add_option("command", cmds, "Commands");
  app.add_option("-r,--remote", remote, "Remote server");
  app.add_option("--schema", schemafile, "Schema file");
  app.add_option("--config", configfile, "Config file");
  app.add_flag("--webserver,-w", webserver, "Launch a webserver");

  try {
    app.parse(argc, argv);
  } catch(const CLI::Error &e) {
    return app.exit(e);
  }
  KolmoConf kc;

  if(remote.empty() && schemafile.empty()) {
    cerr<<"Specify a Kolmo-enabled tool to query, OR a schema file"<<endl;
    cout<<app.help()<<endl;
  }
  
  if(!remote.empty()) {
    MiniCurl mc;
    string cmd=cmds[0];
    if(cmd=="ls") {
      string res=mc.getURL(remote+"/ls/"+cmds[1]);

      nlohmann::json x;
      x = nlohmann::json::parse(res);
            
      boost::format fmt("%1$-20s %|25t|%2$s %|45t|%3$s");
      for(auto iter=x.begin(); iter!=x.end();++iter) {
	cout<<(fmt % iter.key() % iter.value()["value"].get<string>() % iter.value()["description"].get<string>()).str()<<endl;
      }
    }
    else if(cmd=="set") {
      string var, val;
      
      auto pos=cmds[1].find('=');
      if(pos==string::npos) {
	var=cmds[1];
	val="true";
      }
      else {
	var=cmds[1].substr(0, pos);
	val=cmds[1].substr(pos+1);
      }
	  
      string res=mc.getURL(remote+"/runtime/set-value?variable="+var+"&value="+val);
	  
      nlohmann::json x;
      x = nlohmann::json::parse(res);
      cout<<x<<endl;
    }
    else if(cmd=="minimal-config") {
      string res=mc.getURL(remote+"/minimal-config");
      cout<<res<<endl;
    }
    else if(cmd=="delta-config") {
      string res=mc.getURL(remote+"/delta-config");
      cout<<res<<endl;
    }
    else
      cerr<<"Unknown command '"<<cmd<<"'\n";

    return 0;
  }

  kc.initSchemaFromFile(schemafile);

  if(boost::ends_with(configfile, ".json")) {
    kc.initConfigFromJSON(configfile);
  }
  else {
    kc.initConfigFromLua(configfile);
  }

  //  kc.declareRuntime();

  if(webserver) {
    std::thread t(KolmoThread, &kc, ComboAddress("[::]:1234"));
    t.join();
    return 0;
  }
  if(cmds.empty()) {
    cerr<<"No commands were passed!"<<endl;
    cout << app.help()<<endl;
  }
  string cmd=cmds[0];
  if(cmd=="minimal-config") {
    auto minimal=kc.getMinimalConfig();
    nlohmann::json wv;
    KSToJson(minimal.get(), wv);
    cout<<std::setw(4)<<wv<<endl;
  }
  else if(cmd=="full-config") {
    nlohmann::json wv;
    KSToJson(&kc.d_main, wv);
    cout<<std::setw(4)<<wv<<endl;
  }
  else if(cmd=="markdown") {
    markdown(kc);
  }
  else if(cmd=="ls") {
    KolmoStruct* x;
    if(cmds.size()>1)
      x=dynamic_cast<KolmoStruct*>(kc.d_main.getValueAt(cmds[1]));
    else
      x=&kc.d_main;
    boost::format fmt("%1$-20s %|25t|%2$s %|45t|%3$s");
    if(x) {
      for(auto i : x->getAll()) {
        cout<<(fmt % i.first % i.second->getValue() % i.second->description).str()<<endl;
      }
    }
    else {
      KolmoVal* y=kc.d_main.getValueAt(cmds[1]);
      cout<<(fmt % cmds[1] % y->getValue() % y->description).str()<<endl;
    }
  }
  else if(cmd=="set") {
    string var, val;
    auto pos=cmds[1].find('=');
    if(pos==string::npos) {
      var=cmds[1];
      val="true";
    }
    else {
      var=cmds[1].substr(0, pos);
      val=cmds[1].substr(pos+1);
    }
    kc.d_main.setValueAt(var, val);
    syncConfig(kc, configfile);
  }
  else if(cmd=="rm") {
    kc.d_main.rmValueAt(cmds[1]);
    syncConfig(kc, configfile);
  }
  else if(cmd=="add") { // add sites www '{name: "ds9a.nl", path: "/var/www/ds9a.nl/"}'
    KolmoStruct* x=dynamic_cast<KolmoStruct*>(kc.d_main.getValueAt(cmds[1]));
    if(!x) 
      throw std::runtime_error("Can only add value to a struct, which "+cmds[1]+" is not");
    //    cerr<<"Should attempt to add '"<<x->d_membertype<<"' object"<<endl;

    if(x->d_membertype=="ipendpoint") {
      x->addValue(cmds[2]);
    }
    else {
      auto a=x->getNewMember();
      nlohmann::json j=nlohmann::json::parse(cmds[3]);
      
      JsonToKS(j, a);
      x->setStruct(cmds[2], std::unique_ptr<KolmoStruct>(a));
    }
    syncConfig(kc, configfile);
  }
  else {
    cerr<<"Unknown command "<<cmd<<endl;
  }
  return 0;
}
catch(std::exception& e)
{
  cerr<<"Fatal error: "<<e.what()<<endl;
  return EXIT_FAILURE;
}
