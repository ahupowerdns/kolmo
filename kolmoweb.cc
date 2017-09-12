#include "kolmoconf.hh"
#include <crow.h>
#include <iostream>

using namespace std;

static void emitJSON(crow::response& resp, nlohmann::json& wv)
{
  std::ostringstream str;
  str << std::setw(4) << wv << endl;
  resp.write(str.str());
  resp.set_header("Content-Type","application/json");
  resp.end();
}



void KolmoThread(KolmoConf* kc, ComboAddress ca)
try
{
  crow::SimpleApp app;
  CROW_ROUTE(app, "/full-config")([kc](const crow::request& rec, crow::response& resp) {
      nlohmann::json wv;
      KSToJson(&kc->d_main, wv);
      emitJSON(resp, wv);
    });

  CROW_ROUTE(app, "/minimal-config")([kc](const crow::request& rec, crow::response& resp) {
      auto minimal=kc->getMinimalConfig();
      nlohmann::json wv;
      KSToJson(minimal.get(), wv);
      emitJSON(resp, wv);
    });
  
  CROW_ROUTE(app, "/delta-config")([kc](const crow::request& rec, crow::response& resp) {
      auto minimal=kc->getRuntimeDiff();
      nlohmann::json wv={};
      KSToJson(minimal.get(), wv);
      emitJSON(resp, wv);
    });

  CROW_ROUTE(app, "/runtime/set-value")([kc](const crow::request& rec, crow::response& resp) {
      cerr<<rec.url_params.get("variable")<<endl;
      auto variable=rec.url_params.get("variable"), value=rec.url_params.get("value");
      nlohmann::json wv;
      try {
        cerr<<"Setting '"<<variable<<"' to '"<<value<<"'"<<endl;
        kc->d_main.setValueAt(variable, value);
        wv["result"]="ok";
      }catch(std::exception& e) {
        wv["result"]="failure";
        wv["reason"]=e.what();
      }
      emitJSON(resp, wv);
    });

  CROW_ROUTE(app, "/ls<path>")([kc](const crow::request& rec, crow::response& resp, const std::string& path=std::string()) {
      cerr<<path<<endl;
      string rpath=path.substr(1);
      KolmoStruct* ks=&kc->d_main;

      if(!rpath.empty()) {
	cerr<<"Traversing path"<<endl;
	auto ptr=ks->getValueAt(rpath);
	ks=dynamic_cast<KolmoStruct*>(ptr);
	if(!ks) {
	  nlohmann::json item;
	  item["description"] = ptr->description;
	  item["runtime"] = ptr->runtime;
	  item["unit"] = ptr->unit;
	  item["mandatory"] = ptr->mandatory;
	  item["value"]=ptr->getValue();
	  item["default"]=ptr->defaultValue;
	  emitJSON(resp, item);
	  return;
	}
      }
      
      nlohmann::json wv=nlohmann::json::object();
      for(const auto& a : ks->getAll()) {
        nlohmann::json item;
        item["description"] = a.second->description;
	item["runtime"] = a.second->runtime;
	item["unit"] = a.second->unit;
	item["mandatory"] = a.second->mandatory;
	item["value"]=a.second->getValue();
	item["default"]=a.second->defaultValue;
        wv[a.first]=item;
      }
      emitJSON(resp, wv);
    });
  
  
  app.port(ntohs(ca.sin4.sin_port)).bindaddr(ca.toString()).multithreaded().run();
}
catch(std::exception& e) {
  cerr<<"Kolmo thread ended because of error: "<<e.what()<<endl;
}