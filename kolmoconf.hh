#pragma once
#include <string>
#include <boost/variant.hpp>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <boost/core/noncopyable.hpp>
#include "comboaddress.hh"
#include <atomic>
#include <json.hpp>


extern bool g_kolmoRuntimeDeclared;

class KolmoVal
{
public:
  virtual std::unique_ptr<KolmoVal> clone() const =0;
  void copyInBasics(const KolmoVal& rhs)
  {
    mandatory = rhs.mandatory;
    runtime = rhs.runtime;
    defaultValue = rhs.defaultValue;
    description = rhs.description;
    cmdline = rhs.cmdline;
    unit = rhs.unit;
  }
  bool mandatory{false};
  bool runtime{false};
  std::string defaultValue; // I know
  std::string description;
  std::string unit;
  std::string cmdline;
  virtual void setValue(const std::string&)=0;
  virtual std::string display(int indent=0) const=0;
  void tie(std::function<void(KolmoVal*)> v)
  {
    d_ties.push_back(v);
    v(this);
  }

protected:
  void runTies()
  {
    for(auto& t : d_ties)
      t(this);
  }

  void checkRuntime();
  
private:
  std::vector<std::function<void(KolmoVal*)> > d_ties;
};

class KolmoBool : public KolmoVal
{
public:
  explicit KolmoBool(bool v) : d_v(v)
  {}


  std::unique_ptr<KolmoVal> clone() const
  {
    auto ret= std::make_unique<KolmoBool>(d_v);
    ret->copyInBasics(*this);
    return ret;
  }

  void setValue(const std::string& str) 
  {
    if(str=="true")
      d_v = true;
    else if(str=="false")
      d_v = false;
    else
      throw std::runtime_error("Attempt to set bool to something not true or false");

    checkRuntime();
     
    runTies();
  }
  
  static std::unique_ptr<KolmoVal> make(bool v)
  {
    KolmoBool ret(v);
    return ret.clone();
  }

  std::string display(int indent=0) const
  {
    std::ostringstream ret;
    ret<<std::string(indent, ' ')<<(d_v ? "true" : "false")<<" ["<<description<<"], default="<<defaultValue<<std::endl;
    return ret.str();
  }

  bool getBool() const
  {
    return d_v;
  }
  void setBool(bool v) 
  {
    d_v=v;
    runTies();
  }

  bool operator==(const KolmoBool& rhs) const
  {
    return d_v == rhs.d_v;
  }
  bool operator!=(const KolmoBool& rhs) const
  {
    return !operator==(rhs);
  }
  
private:
  bool d_v;

};


class KolmoString : public KolmoVal
{
public:
  explicit KolmoString(const std::string& s) : d_v(s)
  {}

  static std::unique_ptr<KolmoVal> make(const std::string& s)
  {
    return std::make_unique<KolmoString>(s);
  }
  void setValue(const std::string& str) 
  {
    checkRuntime();
    d_v = str;
  }

  std::string getValue() const
  {
    return d_v;
  }
  
  std::unique_ptr<KolmoVal> clone() const
  {
    auto ret=std::make_unique<KolmoString>(d_v);
    ret->copyInBasics(*this);
    return ret;
  }
  std::string display(int indent=0) const
  {
    std::ostringstream ret;
    ret<<std::string(indent, ' ')<<d_v<<" ["<<description<<"]"<<std::endl;
    return ret.str();
  }

  bool operator==(const KolmoString& rhs) const
  {
    return d_v == rhs.d_v;
  }
  bool operator!=(const KolmoString& rhs) const
  {
    return d_v != rhs.d_v;
  }

  
  std::string d_v;  
};

class KolmoIPEndpoint : public KolmoVal
{
public:
  explicit KolmoIPEndpoint(const std::string& str) 
  {
    if(str.empty())
      d_v.sin4.sin_family=0;
    else
      d_v=ComboAddress(str);
  }

  explicit KolmoIPEndpoint(const ComboAddress& ca) : d_v(ca)
  {}

  explicit KolmoIPEndpoint()
  {
    d_v.sin4.sin_family=0;
  }

  
  static std::unique_ptr<KolmoVal> make(const std::string& str)
  {
    return std::make_unique<KolmoIPEndpoint>(str);
  }

  std::unique_ptr<KolmoVal> clone() const
  {
    auto ret=std::make_unique<KolmoIPEndpoint>(d_v);
    ret->copyInBasics(*this);
    return ret;
  }

  void setValue(const std::string& in)
  {
    checkRuntime();
    if(in.empty())
      d_v.sin4.sin_family=0;
    else
      d_v=ComboAddress(in);
  }

  std::string getValue() const
  {
    if(d_v.sin4.sin_family)
      return d_v.toStringWithPort();
    else
      return "";
  }
  
  std::string display(int indent=0) const
  {
    std::ostringstream ret;
    ret<<std::string(indent, ' ')<<d_v.toStringWithPort()<<" ["<<description<<"] default="<<defaultValue<<std::endl;
    return ret.str();
  }

  void setIPEndpoint(const ComboAddress& ca)
  {
    d_v=ca;
  }

  ComboAddress getIPEndpoint() const
  {
    return d_v;
  }

  
  bool operator==(const KolmoIPEndpoint& rhs) const
  {
    if(d_v.sin4.sin_family == 0 && rhs.d_v.sin4.sin_family==0)
      return true;

    return d_v == rhs.d_v;
  }
  bool operator!=(const KolmoIPEndpoint& rhs) const
  {
    return !operator==(rhs);
  }

  
private:
  ComboAddress d_v;
};

class KolmoInteger : public KolmoVal
{
public:
  explicit KolmoInteger(uint64_t v) : d_v(v)
  {}

  static std::unique_ptr<KolmoVal> make(uint64_t v)
  {
    return std::make_unique<KolmoInteger>(v);
  }

  std::unique_ptr<KolmoVal> clone() const
  {
    auto ret=std::make_unique<KolmoInteger>(d_v);
    ret->copyInBasics(*this);
    return ret;
  }

  void setValue(const std::string& in)
  {
    checkRuntime();
    d_v=atoi(in.c_str()); // XXX 64 bit
  }

  uint64_t getInteger() const
  {
    return d_v;
  }

  void setInteger(uint64_t v)
  {
    d_v=v;
  }

  std::string display(int indent=0) const
  {
    std::ostringstream ret;
    ret<<std::string(indent, ' ')<<d_v<<" ["<<description<<"] default="<<defaultValue;
    if(!unit.empty())
      ret<<" unit="<<unit;
    ret<<std::endl;
    return ret.str();
  }

  bool operator==(const KolmoInteger& rhs) const
  {
    return d_v == rhs.d_v;
  }
  bool operator!=(const KolmoInteger& rhs) const
  {
    return d_v != rhs.d_v;
  }


  
private:
  
  uint64_t d_v;  
};


class KolmoStruct : public KolmoVal, public boost::noncopyable
{
public:
  KolmoStruct() {}
  bool getBool(const std::string& name);
  std::string getString(const std::string& name);
  void setString(const std::string& name, const std::string& value);
  void setBool(const std::string& name, bool v);
  void tieBool(const std::string& name, std::atomic<bool>* target);
  void setIPEndpoint(const std::string& name, const std::string& value);
  void setIPEndpointCA(const std::string& name, const ComboAddress& ca);
  void setStruct(const std::string& name, std::unique_ptr<KolmoStruct> s);
  void setInteger(const std::string& name, uint64_t value);
  KolmoVal* getMember(const std::string& name) const;
  KolmoStruct* getStruct(const std::string& name);
  void registerVariableLua(const std::string& name, const std::string& type, std::unordered_map<std::string, std::string> attributes);
  void registerBool(const std::string& name, bool v);
  void registerString(const std::string& name, const std::string& );
  void registerIPEndpoint(const std::string& name, const std::string& );
  void registerStruct(const std::string& name, const std::string& );
  void registerStructMember(const std::string& name, KolmoStruct* s);
  void addStringToStruct(const std::string& name, const std::string& val);

  KolmoStruct* getNewMember();
  
  void unregisterVariable(const std::string& name)
  {
    d_store.erase(name);
  }
  
  std::unique_ptr<KolmoStruct> diff(const KolmoStruct& temp, const KolmoStruct& rhs, int nest=0) const;
  
  std::unique_ptr<KolmoVal> clone() const;


  std::vector<std::string> getMembers() const
  {
    std::vector<std::string> ret;
    for(const auto& m : d_store)
      ret.push_back(m.first);
    return ret;
  }

  std::vector<std::pair<std::string, KolmoVal*> > getAll() const
  {
    std::vector<std::pair<std::string, KolmoVal*>> ret;
    for(const auto& m : d_store)
      ret.push_back({m.first, m.second.get()});
    return ret;
  }


  void setValue(const std::string& )
  {
    checkRuntime();
    // should maybe do JSON
    abort();
  } 
  void setValueAt(const std::string& str, const std::string& val);
  
  std::string display(int indent=0) const
  {
    std::ostringstream ret;
    ret<<std::string(indent, ' ')<<"["<<description<<"]"<<std::endl;
    for(const auto& m : d_store) {
      ret<<std::string(indent, ' ')<<m.first<<":"<<std::endl;
      ret<<m.second->display(indent+8);
    }
    
    return ret.str();
  }
  void setMemberType(const std::string& str)
  {
    d_membertype = str;
  }
  std::string getMemberType() const
  {
    return d_membertype;
  }

  bool empty() const
  {
    return d_store.empty();
  }
private:
  std::string d_membertype, d_mytype;
  std::map<std::string, std::unique_ptr<KolmoVal>> d_store;  
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
  void initConfigFromLua(const std::string& str);
  void initConfigFromJSON(const std::string& str);
  void initConfigFromCmdline(int argc, char**argv);

  KolmoStruct d_main;

  void declareRuntime()
  {
    d_startup = std::move(d_main.clone());
    g_kolmoRuntimeDeclared=true;
  }

  std::unique_ptr<KolmoStruct> getRuntimeDiff() 
  {
    return dynamic_cast<const KolmoStruct*>(d_startup.get())->diff(
								   *dynamic_cast<const KolmoStruct*>(d_default.get()),
								   d_main);
  }

  std::unique_ptr<KolmoStruct> getMinimalConfig() 
  {
    //    return d_main.diff(*dynamic_cast<const KolmoStruct*>(d_default.get()), *dynamic_cast<const KolmoStruct*>(d_default.get()));
    return dynamic_cast<const KolmoStruct*>(d_default.get())->diff(
    								   *dynamic_cast<const KolmoStruct*>(d_default.get()),
    								   d_main);
  }

private:
  void luaInit();
  std::unique_ptr<KolmoVal> d_default, d_startup;

  LuaContext* d_lua{0};
};

void KSToJson(KolmoStruct* ks, nlohmann::json& x);
