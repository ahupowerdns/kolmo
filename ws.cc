#define CROW_ENABLE_SSL
#include <crow.h>
#include "kolmoconf.hh"
#include "comboaddress.hh"
#include <set>

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

string getPath(KolmoConf* kc, const crow::request& req, const std::string& rest)
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
  auto func=[kc](const crow::request& req, const std::string& a){
      auto path=getPath(kc, req, a);
      if(path.empty() || path.find("..") != string::npos)
        return string("Can't find it mate");

      FILE* fp=fopen(path.c_str(), "r");
      if(!fp) {
        return string("Can't find it mate 2");
      }
      string ret;
      char buffer[4096];
      size_t nbytes;
      while((nbytes=fread(buffer,1,sizeof(buffer), fp))!=0) {
        cout<<nbytes<<endl;
        ret.append(buffer, nbytes);
      }
      fclose(fp);
      return ret;
      
  };
  CROW_ROUTE(app, "/<path>")(func);

  CROW_ROUTE(app, "/")([&func](const crow::request& rec) { return func(rec, "/index.html");});

  CROW_ROUTE(app, "/kctl")([]() {
      return "hi";
    });
  
  app.port(ntohs(ca.sin4.sin_port)).bindaddr(ca.toString()).multithreaded().run();
}
catch(std::exception& e)
{
  cerr<<"Error from webserver for "<<ca.toString()<<": "<<e.what()<<endl;
}

int main(int argc, char** argv)
{
  KolmoConf kc;
  kc.initSchemaFromFile("ws-schema.lua");
  kc.initConfigFromFile("ws.conf");

  auto verbose=kc.d_main.getBool("verbose");
  if(verbose) {
    cerr<<"Must be verbose"<<endl;
    cerr<<"Server name is "<<kc.d_main.getString("server-name")<<endl;
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


  cout<<"Full config: "<<endl;
  cout<<kc.d_main.display();

  cerr<<"Need to listen on "<<listenAddresses.size()<<" addresses"<<endl;
  
  vector<std::thread> listeners;
  for(const auto& a : listenAddresses) {
    ComboAddress addr(a);
    listeners.emplace_back(listenThread, &kc, addr);
  }
  
  for(auto& t: listeners)
    t.join();

  
}
