#include <crow.h>
#include "kolmoconf.hh"



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

int main()
{
  KolmoConf kc;
  kc.initSchemaFromFile("ws-schema.lua");


  auto verbose=kc.d_main.getBool("verbose");
  if(verbose) {
    cerr<<"Must be verbose"<<endl;
    cerr<<"Server name is "<<kc.d_main.getString("server-name")<<endl;
  }

  auto site = kc.d_main.getStruct("website"); // XXX SHOULd BE TYPESAFE

  cerr<<"We run a website called "<<site->getString("name")<<endl;
  cerr<<"The site enable status: "<<site->getBool("enabled")<<endl;
  cerr<<"We serve from path: "<<site->getString("path")<<endl;


  crow::SimpleApp app;
  CROW_ROUTE(app, "/")([](){
      return "Hello world";
    });
  
  app.port(18080).bindaddr("::1").multithreaded().run();
  
}
