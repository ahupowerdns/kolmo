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

void listenThread(KolmoConf* kc, ComboAddress ca)
{
  crow::SimpleApp app;
  CROW_ROUTE(app, "/<path>")([kc](const crow::request& req, const std::string& a){
      cout<<req.url<<endl;
      auto f=req.headers.find("host");
      if(f != req.headers.end()) {
	cout<<"Host: "<<f->second<<endl;
	auto sites = kc->d_main.getStruct("sites"); // XXX SHOULd BE TYPESAFE
	for(const auto& m : sites->getMembers()) {
	  auto site = sites->getStruct(m);
	  if(site->getString("name")==f->second) {
	    return "Found your host, should get file "+a+" from path "+site->getString("path");
	  }
	}
      }
      // set 404
      return string("Not found");
    });
  
  app.port(ntohs(ca.sin4.sin_port)).bindaddr(ca.toString()).multithreaded().run();

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
    cerr<<"The site enable status: "<<site->getBool("enabled")<<endl;
    cerr<<"We serve from path: "<<site->getString("path")<<endl;
    cerr<<"We serve on address: "<<site->getString("listen")<<endl;
    listenAddresses.insert(site->getString("listen"));
    cerr<<endl;
  }

  cerr<<"Need to listen on "<<listenAddresses.size()<<" addresses";
  vector<std::thread> listeners;
  for(const auto& a : listenAddresses) {
    ComboAddress addr(a);
    listeners.emplace_back(listenThread, &kc, addr);
  }

  for(auto& t: listeners)
    t.join();

  
}
