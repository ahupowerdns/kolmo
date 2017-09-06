#include "kolmoconf.hh"
#include "http.hh"
#include <crow/json.h>
#include "minicurl.hh"
#include "CLI/CLI.hpp"
#include <boost/format.hpp>
using namespace std;

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
    ifstream ifs(configfile);
    nlohmann::json wv;
    ifs >> wv;

    JsonToKS(wv, &kc.d_main);

    auto res=kc.getMinimalConfig();
    nlohmann::json out;
    KSToJson(res.get(), out);
    cout<<std::setw(4)<<out<<endl;
    return 0;
  }
  else {
    kc.initConfigFromLua(configfile);
  }

  kc.declareRuntime();
  
  nlohmann::json wv;

  KSToJson(&kc.d_main, wv);
  {
    std::ofstream of("config.json");
    of << std::setw(4) << wv;
  }
  

  if(!setstring.empty()) {
    cerr<<"Setstring: "<<setstring<<endl;
    string var, val;
    
    auto pos=setstring.find('=');
    if(pos==string::npos) {
      var=setstring;
      val="true";
    }
    else {
      var=setstring.substr(0, pos);
      val=setstring.substr(pos+1);
    }
    kc.d_main.setValueAt(var, val);
  }


  KSToJson(&kc.d_main, wv);
  {
    std::ofstream of("config-new.json");
    of << std::setw(4) << wv;
  }

  auto diff = kc.getRuntimeDiff();
  wv={};
  KSToJson(diff.get(), wv);
  cout << std::setw(4) << wv;
}
catch(std::exception& e)
{
  cerr<<"Fatal error: "<<e.what()<<endl;
  return EXIT_FAILURE;
}
