#include "kolmoconf.hh"
#include "http.hh"
#include <crow/json.h>
#include "minicurl.hh"
#include "CLI/CLI.hpp"
using namespace std;



int main(int argc, char** argv)
try
{
  CLI::App app("kolctl");

  vector<std::string> files;
  string setstring;
  app.add_option("-f,--file,file", files, "File name");
  app.add_option("-s", setstring, "Set string");

  try {
    app.parse(argc, argv);
  } catch(const CLI::Error &e) {
    return app.exit(e);
  }
  KolmoConf kc;

  if(boost::starts_with(files[0], "http://") ||boost::starts_with(files[0], "https://")) {
    MiniCurl mc;
    cout<<mc.getURL(files[0]+"/"+files[1])<<endl;
    return 0;
    
  }
  
  kc.initSchemaFromFile(files[0]);

  if(boost::ends_with(files[1], ".json")) {
    ifstream ifs(files[1]);
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
    kc.initConfigFromLua(files[1]);
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
