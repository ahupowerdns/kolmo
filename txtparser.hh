#pragma once
#include "pegtl.hh"

namespace hello
{
  // Parsing rule that matches a literal "Hello, ".
  
  struct prefix
    : pegtl_string_t("listen-address") {};
  
  // Parsing rule that matches a non-empty sequence of
  // alphabetic ascii-characters with greedy-matching.

  struct name
    : pegtl::plus< pegtl::alpha > {};
  
  
  struct octet
    : pegtl::rep_min_max< 1,3, pegtl::digit > {};

  //h16           = 1*4HEXDIG
  struct h16
    : pegtl::rep_min_max< 1,4, pegtl::xdigit > {};

  
  struct ipv4
    : pegtl::seq<pegtl::rep<3, pegtl::seq<octet, pegtl::one<'.'> > >, octet> {};


  // ls32          = ( h16 ":" h16 ) / IPv4address
  struct ls32
    : pegtl::sor<pegtl::seq<h16, pegtl::one<':'>, h16>, ipv4> {};

  // 2001:470:7dc9::f   -  case 7 should match
  
  /*
0   IPv6address   =                            6( h16 ":" ) ls32
1                 /                       "::" 5( h16 ":" ) ls32
2                 / [               h16 ] "::" 4( h16 ":" ) ls32
3                 / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
4                 / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
5                 / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
6                 / [ *4( h16 ":" ) h16 ] "::"              ls32
7                 / [ *5( h16 ":" ) h16 ] "::"              h16
8                 / [ *6( h16 ":" ) h16 ] "::"
  */


  struct colon : pegtl::one<':'> {};
  struct dcolon : pegtl_string_t("::") {};

  struct h16colon : pegtl::seq<h16, colon> {};

  using namespace pegtl;
  #if 0
  struct ipv6
    : sor<
        seq<rep<6, h16colon>, ls32>,
        seq<dcolon, rep<5, h16colon>, ls32>,
        seq<rep_opt<1, h16>, dcolon, rep<4, h16colon>, ls32>,
        seq<
          rep_opt<1, seq<rep_max<1, h16colon>, h16> >,
          dcolon, rep<3, h16colon>, ls32
        >,
        seq<
          rep_opt<1, seq<rep_max<2, h16colon>, h16> >,
          dcolon, rep<2, h16colon>, ls32
        >,
        seq<
          rep_opt<1, seq<rep_max<3, h16colon>, h16> >,
          dcolon, h16colon, ls32
        >,
        seq<
          rep_opt<1, seq<rep_max<4, h16colon>, h16> >,
          dcolon, ls32
        >,
        seq<
          rep_opt<1,seq<rep_max<5, h16colon>, must<h16> > >,
          dcolon, h16
        >,
        seq<
          rep_opt<1, seq<rep_max<6, h16colon>, h16> >,
          dcolon
        >
    > {};
#endif
    struct IPv6address : 
sor< 
        seq< rep< 6, seq< h16, colon > >, ls32 >,
      seq<until< dcolon, h16colon >, seq<rep_max<6, h16colon>, h16> >,
        dcolon
     > {};


  struct ip
    : pegtl::sor<ipv4, IPv6address>{};
  
  // Parsing rule that matches a sequence of the 'prefix'
  // rule, the 'name' rule, a literal "!", and 'eof'
  // (end-of-file/input), and that throws an exception
  // on failure.
  
  struct grammar
    : pegtl::must< prefix, pegtl::plus<pegtl::space>, ip, pegtl::eof > {};
  
  // Class template for user-defined actions that does
  // nothing by default.
  
   template< typename Rule >
   struct action
     : pegtl::nothing< Rule > {};
  
  // Specialisation of the user-defined action to do
  // something when the 'name' rule succeeds; is called
  // with the portion of the input that matched the rule.
  
  template<> struct action< ipv4 >
  {
    template< typename Input >
    static void apply( const Input & in, std::string & name )
    {
      name = in.string();
    }
  };
  /*
  template<> struct action< ipv6 >
  {
    template< typename Input >
    static void apply( const Input & in, std::string & name )
    {
      name = in.string();
    }
  };
  */
  template<> struct action< IPv6address >
  {
    template< typename Input >
    static void apply( const Input & in, std::string & name )
    {
      name = in.string();
    }
  };

  
} // hello
