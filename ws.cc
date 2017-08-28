#include <crow.h>
#include "kolmoconf.hh"


int main()
{
  crow::SimpleApp app;
  
  CROW_ROUTE(app, "/")([](){
      return "Hello world";
    });
  
  app.port(18080).bindaddr("::1").multithreaded().run();
  
}
