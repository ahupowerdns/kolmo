#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "txtparser.hh"





TEST_CASE( "Basic math", "[basic]" ) {
  REQUIRE(1+1 == 2);
}



TEST_CASE( "Basic parser", "[parser]") {
  std::string name;
  pegtl::parse_string< hello::grammar, hello::action >( "Hello, Bert!", "str", name);
  REQUIRE(name=="Bert");
}
