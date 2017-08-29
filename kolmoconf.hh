#pragma once
#include <string>
#include <boost/variant.hpp>
#include <cstdint>
#include <map>
#include <vector>
#include <boost/core/noncopyable.hpp>

struct KolmoVal
{
  virtual std::unique_ptr<KolmoVal> clone() const =0;
};

struct KolmoBool : public KolmoVal
{
  explicit KolmoBool(bool v) : d_v(v)
  {}


  std::unique_ptr<KolmoVal> clone() const
  {
    return std::make_unique<KolmoBool>(d_v);
  }

  
  static std::unique_ptr<KolmoVal> make(bool v)
  {
    KolmoBool ret(v);
    return ret.clone();
  }
  bool d_v;
};


struct KolmoString : public KolmoVal
{
  explicit KolmoString(const std::string& s) : d_v(s)
  {}

  static std::unique_ptr<KolmoVal> make(const std::string& s)
  {
    return std::make_unique<KolmoString>(s);
  }

  std::unique_ptr<KolmoVal> clone() const
  {
    return std::make_unique<KolmoString>(d_v);
  }
  
  std::string d_v;  
};


struct KolmoStruct : public KolmoVal, public boost::noncopyable
{
  KolmoStruct() {}
  bool getBool(const std::string& name);
  std::string getString(const std::string& name);
  void setString(const std::string& name, const std::string& value);
  KolmoStruct* getStruct(const std::string& name);

  void registerBool(const std::string& name, bool v);
  void registerString(const std::string& name, const std::string& );
  KolmoStruct* registerStruct(const std::string& name, const std::string& );
  KolmoStruct* registerVector(const std::string& name, const std::string& type);
  //  void registerStruct(const std::string& name, const KolmoStruct& );

  std::unique_ptr<KolmoVal> clone() const;
  std::map<std::string, std::unique_ptr<KolmoVal>> d_store;

  std::vector<std::string> getMembers() const
  {
    std::vector<std::string> ret;
    for(const auto& m : d_store)
      ret.push_back(m.first);
    return ret;
  }
};



struct KolmoVector : public KolmoVal, public boost::noncopyable
{
  KolmoVector(const std::string& type) : d_type(type) {}
  
  std::string d_type;
  std::vector<std::shared_ptr<KolmoVal> > d_contents;

  std::shared_ptr<KolmoVal> create();
  void append(std::shared_ptr<KolmoVal> ks); 
};

/* idea
   The schema is a Lua file that creates KolmoClasses
   Some classes exist already: bool, ComboAddress, String

   There is an empty global namespace which is vector of KolmoClasses.
   To keep things exciting, a KolmoClass can also be a vector of KolmoClasses.

   The schema file populates the global namespace with named global settings,
   and their defaults, and also creates empty holders (with a name)
   for things the target program needs: service endpoints, rules etc.

   So a sample configuration may be:

   "verbose": bool
   define Site
   "sites": vector<Site>
*/

class LuaContext;
extern std::map<std::string, std::unique_ptr<KolmoVal> > d_prototypes; // XXX NO NO NO, has to be in kolmoconf!
class KolmoConf
{
public:
  KolmoConf();
  ~KolmoConf();
  void initSchemaFromString(const std::string& str);
  void initSchemaFromFile(const std::string& str);
  void initConfigFromFile(const std::string& str);
  
  KolmoStruct d_main;

private:
  void luaInit();

  LuaContext* d_lua{0};
};
