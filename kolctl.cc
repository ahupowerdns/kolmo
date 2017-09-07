#include "kolmoconf.hh"
#include "http.hh"
#include <crow/json.h>
#include "minicurl.hh"
#include "CLI/CLI.hpp"
#include <boost/format.hpp>
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

int main(int argc, char** argv)
try
{
  CLI::App app("kolctl");

  vector<std::string> cmds;
  string setstring, remote, schemafile, configfile;
  app.add_option("--commands,command", cmds, "Commands");
  app.add_option("-r,--remote,remote", remote, "Remote server");
  app.add_option("--schema,schema", schemafile, "Schema file");
  app.add_option("--config,c", configfile, "Config file");
  app.add_option("-d,--do", setstring, "Set string");

  try {
    app.parse(argc, argv);
  } catch(const CLI::Error &e) {
    return app.exit(e);
  }
  KolmoConf kc;

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
