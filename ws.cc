#define CROW_ENABLE_SSL
#include <crow.h>
#include "kolmoconf.hh"
#include "comboaddress.hh"
#include <set>
#include <boost/utility/string_ref.hpp>
#include <sstream>
#include "http.hh"

/* 
   Welcome to the default free zone!
   No single number or default can live here EVERYTHING comes from the configuration file.

   The webserver serves URL of SITES which have NAMES and ALTERNATE NAMES.
   URLs map to DIRECTORIES or ACTIONS (Redirects of various types)

   A SITE listens on one or more IP address LISTENERS

   LISTENERS can have settings too


   Settings: 

   GLOBAL
       Syslog on/off

   LISTENER
       Address
       Port
       Listen-limit
       Free-bind / non-local

   SITE
       Domain Name
       Alternate name
       Make-canonical 
       Mapping / -> 
            DIRECTORY 
               Path /var/www/
               Directory-listing: no
       Mapping /new/ -> 
            ACTION
               Target https://new.site/
               Code 302

       Password /data/ -> Passwords: default

       PASSWORDS
            Name: default  
            Entries: [{User: user, Password: password}] 

 */

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::set;
using std::vector;
using std::string;

static int readFile(const boost::string_ref& path, std::string& content)
{
  FILE* ffp=fopen(&path[0], "r");
  if(!ffp) {
    return -1;
  }
  std::shared_ptr<FILE> fp(ffp, fclose);
  content.clear();
  char buffer[4096];
  size_t nbytes;
  while((nbytes=fread(buffer,1,sizeof(buffer), fp.get()))!=0) {
    content.append(buffer, nbytes);
  }
  return content.size();
}

string getPath(KolmoConf* kc, const crow::request& req, const std::string& rest, KolmoStruct** siteptr)
{
  cout<<req.url<<endl;
  auto f=req.headers.find("host");
  if(f != req.headers.end()) {
    string host=f->second;
    auto pos = host.find(':');
    if(pos != string::npos)
      host.resize(pos);
    cout<<"Host: "<<host<<endl;
    auto sites = kc->d_main.getStruct("sites"); // XXX SHOULd BE TYPESAFE
    for(const auto& m : sites->getMembers()) {
      auto site = sites->getStruct(m);
      if(site->getString("name")==host && site->getBool("enabled")) {
        cout<<"Got it, should serve up to "<<site->getString("path")+"/"+rest<<endl;
        *siteptr=site;
        return site->getString("path")+"/"+rest;
      }
    }
  }
  return "";
}


void listenThread(KolmoConf* kc, ComboAddress ca)
try
{
  crow::SimpleApp app;
  bool tls{false};
  auto func=[ca,kc,&tls](const crow::request& req, crow::response& resp, const std::string& a){
    cerr<<"Request for "<<req.url<<" came in on "<<ca.toStringWithPort()<<", tls="<<tls<<endl;
    KolmoStruct *site=0;
    auto path=getPath(kc, req, a, &site);

    if(!tls && site && site->getBool("redirect-to-https")) {
      resp.code=301;
      resp.set_header("Location", "https://"+site->getString("name")+req.url);
      resp.body=R"(<html>
<head><title>301 Moved Permanently</title></head>
<body bgcolor="white">
<center><h1>301 Moved Permanently</h1></center>
<hr><center>ws</center>
</body>
</html>)";

      resp.end();
      return;
    }
    
    
    if(path.empty() || path.find("..") != string::npos) {
      cerr<<"Can't find path for "<<req.url<<endl;
      resp.code=404;
      resp.body="Can't find URL";
      resp.end();
      return;
    }

    string content;
    if(readFile(path, resp.body) < 0) {
      resp.code=404;
      resp.body="Can't find URL";
      resp.end();
    }
    resp.set_header("Content-Type", pickContentType(path));
    resp.end();
  };
  CROW_ROUTE(app, "/<path>")(func);

  CROW_ROUTE(app, "/")([&func](const crow::request& rec, crow::response& resp) { return func(rec, resp,  "/index.html");});


  auto listeners=kc->d_main.getStruct("listeners");
  for(const auto& m : listeners->getMembers()) {
    ComboAddress pot(m);
    if(pot==ca) {
      cerr<<"Found a listener that applies!"<<endl;
      auto listener=listeners->getStruct(m);
      auto pemfile=listener->getString("pem-file");
      auto certfile=listener->getString("cert-file");
      auto keyfile=listener->getString("key-file");
      if(!pemfile.empty()) {
        cerr<<"Doing TLS on "<<ca.toStringWithPort()<<", pem-file: "<<pemfile<<endl;
        tls=true;
        app.port(ntohs(ca.sin4.sin_port)).bindaddr(ca.toString()).ssl_file(pemfile).multithreaded().run();
        cerr<<"Ended?"<<endl;
        return;
      }
      else if(!certfile.empty()) {
        tls=true;
        cerr<<"Doing certfile TLS on "<<ca.toStringWithPort()<<", pem-file: "<<pemfile<<endl;
        crow::ssl_context_t ctx{boost::asio::ssl::context::sslv23};
        ctx.use_certificate_chain_file(certfile);

        ctx.use_private_key_file(keyfile, crow::ssl_context_t::pem);
        ctx.set_options(
                                 boost::asio::ssl::context::default_workarounds
                                 | boost::asio::ssl::context::no_sslv2
                                 | boost::asio::ssl::context::no_sslv3
                                 );
        
        app.port(ntohs(ca.sin4.sin_port)).bindaddr(ca.toString()).ssl(std::move(ctx)).multithreaded().run();
        cerr<<"Ended?"<<endl;
        return;
        
      }
    }
  }
  
  app.port(ntohs(ca.sin4.sin_port)).bindaddr(ca.toString()).multithreaded().run();
}
catch(std::exception& e)
{
  cerr<<"Error from webserver for "<<ca.toString()<<": "<<e.what()<<endl;
}

std::atomic<bool> g_verbose;


void emitJSON(crow::response& resp, nlohmann::json& wv)
{
  std::ostringstream str;
  str << std::setw(4) << wv << endl;
  resp.write(str.str());
  resp.set_header("Content-Type","application/json");
  resp.end();
}

void KolmoThread(KolmoConf* kc, ComboAddress ca)
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

int main(int argc, char** argv)
{
  //  crow::logger::setLogLevel(crow::LogLevel::Debug);
  KolmoConf kc;
  kc.initSchemaFromFile("ws-schema.lua");
  kc.initConfigFromLua("ws.conf");
  kc.initConfigFromJSON("ws.json");
  kc.declareRuntime();
  kc.initConfigFromCmdline(argc, argv);

  kc.d_main.tieBool("verbose", &g_verbose);
  
  if(g_verbose) {
    cerr<<"Must be verbose"<<endl;
    cerr<<"Server name is "<<kc.d_main.getString("server-name")<<endl;
  }
  else {
    cerr<<"Verbose is false"<<endl;
  }

  set<string> listenAddresses;
  auto sites = kc.d_main.getStruct("sites"); // XXX SHOULd BE TYPESAFE

  for(const auto& m : sites->getMembers()) {
    auto site = sites->getStruct(m);
    cerr<<"["<<m<<"] We run a website called "<<site->getString("name")<<endl;
    if(!site->getBool("enabled")) {
      cerr<<"However, site is not enabled, skipping"<<endl;
    }
    else {
      cerr<<"The site enable status: "<<site->getBool("enabled")<<endl;
      cerr<<"We serve from path: "<<site->getString("path")<<endl;
      cerr<<"We serve on addresses: ";

      auto listeners = site->getStruct("listen");
      for(const auto& i : listeners->getMembers()) {
        cerr<<listeners->getString(i)<<endl;
        listenAddresses.insert(listeners->getString(i));
      }
    }
    cerr<<endl;
  }

  //  cout<<"Full config: "<<endl;
  //  cout<<kc.d_main.display();

  cerr<<"Need to listen on "<<listenAddresses.size()<<" addresses"<<endl;
  
  vector<std::thread> listeners;

  auto addr=kc.d_main.getIPEndpoint("kolmo-server");

  if(addr.sin4.sin_family) // this should actually be some kind of 'isSet()' thing
    listeners.emplace_back(KolmoThread, &kc, addr);
  
  for(const auto& a : listenAddresses) {
    ComboAddress addr(a);
    listeners.emplace_back(listenThread, &kc, addr);
  }
  
  for(auto& t: listeners)
    t.join();

  
}
