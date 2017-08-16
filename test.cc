#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "txtparser.hh"
#include "pegtl/trace.hh"




TEST_CASE( "Basic math", "[basic]" ) {
  REQUIRE(1+1 == 2);
}



TEST_CASE( "Basic parser", "[parser]") {
  std::string name;
  pegtl::parse_string< hello::grammar, hello::action >( "listen-address 1.2.3.4", "str", name);
  REQUIRE(name=="1.2.3.4");

  std::cout<<"Werkte\n";
  
  name.clear();
  pegtl::trace_string< hello::grammar, hello::action >( "listen-address ::1", "str", name);
  REQUIRE(name=="::1");

  name.clear();
  pegtl::parse_string< hello::grammar, hello::action >( "listen-address 2001:470:1f15:bba:beee:7bff:fe89:f0fb", "str", name);
  REQUIRE(name=="2001:470:1f15:bba:beee:7bff:fe89:f0fb");

  name.clear();
  pegtl::parse_string< hello::grammar, hello::action >( "listen-address ::", "str", name);
  REQUIRE(name=="::");


  name.clear();
  pegtl::trace_string< hello::grammar, hello::action >( "listen-address 2001:470:7dc9::f", "str", name);
  REQUIRE(name=="2001:470:7dc9::f");

  
  name.clear();
  pegtl::trace_string< hello::grammar, hello::action >( "listen-address 2001::", "str", name);
  REQUIRE(name=="2001::");

  name.clear();
  pegtl::trace_string< hello::grammar, hello::action >( "listen-address 2001::1", "str", name);
  REQUIRE(name=="2001::1");


  

  REQUIRE_THROWS((pegtl::parse_string< hello::grammar, hello::action >( "listen-address 20001:470:7dc9::f", "str", name)));
    
}
