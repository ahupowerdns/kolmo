#include "kolmoconf.hh"
#include <stdexcept>
#include <fstream>
#include <iostream>
#include "LuaContext.hpp"
#include <boost/algorithm/string.hpp>
using namespace std;

bool g_kolmoRuntimeDeclared;

KolmoConf::KolmoConf()
{
  d_prototypes["bool"]=std::make_unique<KolmoBool>(false);
  d_prototypes["string"]=std::make_unique<KolmoString>("");
  d_prototypes["integer"]=std::make_unique<KolmoInteger>(0);
  d_prototypes["struct"]=std::make_unique<KolmoStruct>();
  d_prototypes["ipendpoint"]=std::make_unique<KolmoIPEndpoint>();
  luaInit();
}

void KolmoStruct::registerBool(const std::string& name, bool value)
{
  d_store[name]=KolmoBool::make(value);
}

void KolmoStruct::registerString(const std::string& name, const std::string& value)
{
  d_store[name]=KolmoString::make(value);
}

void KolmoStruct::registerIPEndpoint(const std::string& name, const std::string& value)
{
  d_store[name]=KolmoIPEndpoint::make(value);
}


std::map<std::string, std::unique_ptr<KolmoVal> > d_prototypes; // XXX NO NO NO, has to be in kolmoconf!

void KolmoStruct::registerStruct(const std::string& name, const std::string& strname)
{
  auto f = d_prototypes.find(strname);
  if(f == d_prototypes.end())
    throw std::runtime_error("No class named "+name+" found");
  auto s=dynamic_cast<KolmoStruct*>(f->second.get());
  if(!s)
    throw std::runtime_error("requested wrong type for configuration item "+name);

  KolmoStruct ks;
  ks.setMemberType(strname);
  d_store[name]=ks.clone();
}


void KolmoStruct::registerStructMember(const std::string& name, KolmoStruct* s)
{
  d_store[name]=s->clone();
}


std::unique_ptr<KolmoVal> KolmoStruct::clone() const
{
  KolmoStruct* ret = new KolmoStruct; // XXX
  ret->copyInBasics(*this);
  for(const auto& a : d_store) {
    ret->d_store[a.first] = a.second->clone();
  }
  ret->setMemberType(getMemberType());
  ret->d_mytype=d_mytype;
  return std::unique_ptr<KolmoVal>(ret);
}


void KolmoVal::checkRuntime()
{
  if(g_kolmoRuntimeDeclared && !runtime)
    throw std::runtime_error("Attempting to change a variable at runtime that does not support runtime changes");
}

void KolmoStruct::setValueAt(const std::string& name, const std::string& value)
{
  vector<string> split;
  boost::split(split,name,boost::is_any_of("/"));
  auto ptr=this;
  for(auto iter=split.begin(); iter != split.end(); ++iter) {
    if(iter + 1 != split.end()) {
      ptr=dynamic_cast<KolmoStruct*>(ptr->getMember(*iter));
      if(!ptr) {
        throw std::runtime_error("Traversing variable path, hit non-struct");
      }
    }
    else {
      auto rptr=ptr->getMember(*iter);
      rptr->setValue(value);
    }
  }  
}

void KolmoStruct::rmValueAt(const std::string& name)
{
  vector<string> split;
  boost::split(split,name,boost::is_any_of("/"));
  auto ptr=this;
  for(auto iter=split.begin(); iter != split.end(); ++iter) {
    if(iter + 1 != split.end()) {
      ptr=dynamic_cast<KolmoStruct*>(ptr->getMember(*iter));
      if(!ptr) {
        throw std::runtime_error("Traversing variable path, hit non-struct");
      }
    }
    else {
      ptr->d_store.erase(*iter);
    }
  }  
}


KolmoVal* KolmoStruct::getValueAt(const std::string& name) const
{
  vector<string> split;
  boost::split(split,name,boost::is_any_of("/"));
  auto ptr=this;
  for(auto iter=split.begin(); iter != split.end(); ++iter) {
    if(iter + 1 != split.end()) {
      ptr=dynamic_cast<KolmoStruct*>(ptr->getMember(*iter));
      if(!ptr) {
        throw std::runtime_error("Traversing variable path, hit non-struct");
      }
    }
    else {
      return (KolmoVal*)ptr->getMember(*iter);
    }
  }
  return 0; // XXX CRASH
}


KolmoStruct* KolmoStruct::getNewMember()
{
  auto f = d_prototypes.find(d_membertype);
  if(f == d_prototypes.end())
    throw std::runtime_error("No class named '"+d_membertype+"' found in getNewMember");
  
  auto ret= dynamic_cast<KolmoStruct*>(f->second->clone().release());
  ret->d_mytype =d_membertype;
  return ret;
}

void KolmoStruct::registerVariableLua(const std::string& name, const std::string& type, std::unordered_map<string, string> attributes)
{
  auto f = d_prototypes.find(type);
  if(f == d_prototypes.end())
    throw std::runtime_error("No class named "+type+" found");

  auto thing=f->second->clone();
  auto iter = attributes.find("default");
  if(iter != attributes.end()) {
    thing->setValue(iter->second);
    thing->defaultValue=iter->second;
  }
  iter = attributes.find("runtime");
  if(iter != attributes.end())
    thing->runtime=iter->second=="true";
  iter = attributes.find("description");
  if(iter != attributes.end())
    thing->description=iter->second;

  iter = attributes.find("cmdline");
  if(iter != attributes.end())
    thing->cmdline=iter->second;
  
  iter = attributes.find("unit");
  if(iter != attributes.end())
    thing->unit=iter->second;

  iter = attributes.find("member_type");
  if(iter != attributes.end()) {
    auto f = d_prototypes.find(iter->second);
    if(f == d_prototypes.end())
      throw std::runtime_error("No class named "+name+" found for member-type lookup");
    auto s=dynamic_cast<KolmoStruct*>(thing.get());
    if(!s)
      throw std::runtime_error("requested wrong type for configuration item "+name);
    s->setMemberType(iter->second);
  }

  
  //  cout<<"Registering variable "<<name<<" of type "<<type<<", description="<<thing->description<<endl;
  d_store[name]=thing->clone();
}


void KolmoConf::luaInit()
{
  d_lua=new LuaContext();


  d_lua->writeFunction("getMain", [this](){ return &d_main; });
  d_lua->writeVariable("main", &d_main);
  d_lua->registerFunction("registerVariable", &KolmoStruct::registerVariableLua);
  d_lua->registerFunction("registerBool", &KolmoStruct::registerBool);
  
  d_lua->registerFunction("registerString", &KolmoStruct::registerString);
  d_lua->registerFunction("registerStructMember", &KolmoStruct::registerStructMember);
  d_lua->registerFunction("getNewMember", &KolmoStruct::getNewMember);

  
  d_lua->registerFunction("getStruct", &KolmoStruct::getStruct);
  d_lua->registerFunction("setString", &KolmoStruct::setString);
  d_lua->registerFunction("setIPEndpoint", &KolmoStruct::setIPEndpoint);
  d_lua->registerFunction("setBool", &KolmoStruct::setBool);

  d_lua->registerFunction("addValueToStruct", &KolmoStruct::addValueToStruct);

  
  d_lua->writeFunction("createClass", [this](const std::string& name,
					     vector<pair<string, vector<pair<int, string> > > > i) {
                         //			 cout<<"Registering class "<<name<<endl;
			 KolmoStruct ks;
			 for(const auto& e : i) {
			   cout<<" Attribute '"<<e.first<<"' of type '"<<e.second[0].second<<"' and default value: '"<<e.second[1].second<<"'"<<endl;
			   if(e.second[0].second=="bool")
			     ks.registerBool(e.first, e.second[1].second=="true");
			   else if(e.second[0].second=="string")
			     ks.registerString(e.first, e.second[1].second);
                           else if(e.second[0].second=="struct")
			     ks.registerStruct(e.first, e.second[1].second);
                           else if(e.second[0].second=="ipendpoint")
			     ks.registerIPEndpoint(e.first, e.second[1].second);
			   else {
			     throw std::runtime_error("attempting to register unknown type "+e.second[0].second);
			   }

			 }
			 d_prototypes[name]=ks.clone();
                         return (KolmoStruct*)d_prototypes[name].get();
		       });


  
}

KolmoConf::~KolmoConf()
{
  delete d_lua;
}

void KolmoConf::initSchemaFromString(const std::string& str)
{
  d_lua->executeCode(str);
}

void KolmoConf::initSchemaFromFile(const std::string& str)
try
{
  std::ifstream ifs{str};
  d_lua->executeCode(ifs);
  d_default = std::move(d_main.clone());
}
catch(const LuaContext::ExecutionErrorException& e) {
  std::cerr << e.what(); 
  try {
    std::rethrow_if_nested(e);
    
    std::cerr << std::endl;
    abort();
  } catch(const std::exception& ne) {
    // ne is the exception that was thrown from inside the lambda
    std::cerr << ": " << ne.what() << std::endl;
    abort();
  }
}

void KolmoConf::initConfigFromCmdline(int argc, char** argv)
{
  for(int n=1; n < argc; ++n) {
    for(const auto m : d_main.getAll()) {
      if(argv[n]==m.second->cmdline) {
        cerr<<"Hit!"<<endl;
        auto ptr = dynamic_cast<KolmoBool*>(m.second);
        if(ptr)
          ptr->setBool(true);
      }
    }
  } 
}

void KolmoConf::initConfigFromJSON(const std::string& str)
{
  std::ifstream ifs{str};
  if(!ifs)
    throw std::runtime_error("Could not open "+str+" for configuration file reading");
  nlohmann::json wv;
  ifs >> wv;
  JsonToKS(wv, &d_main);
}

void KolmoConf::initConfigFromLua(const std::string& str)
try
{
  std::ifstream ifs{str};
  d_lua->executeCode(ifs);
}
catch(const LuaContext::ExecutionErrorException& e) {
  std::cerr << e.what(); 
  try {
    std::rethrow_if_nested(e);
    
    std::cerr << std::endl;
  } catch(const std::exception& ne) {
    // ne is the exception that was thrown from inside the lambda
    std::cerr << ": " << ne.what() << std::endl;
  }
  abort();
}

bool KolmoStruct::getBool(const std::string& name)
{
  auto f=d_store.find(name);
  if(f==d_store.end())
    throw std::runtime_error("requested non-existent configuration item '"+name+"'");
  auto s=dynamic_cast<KolmoBool*>(f->second.get());
  if(!s)
    throw std::runtime_error("requested wrong type for configuration item '"+name+"'");
  return s->getBool();
}

ComboAddress KolmoStruct::getIPEndpoint(const std::string& name) const
{
  auto f=d_store.find(name);
  if(f==d_store.end())
    throw std::runtime_error("requested non-existent IPEndpoint configuration item '"+name+"'");
  auto s=dynamic_cast<KolmoIPEndpoint*>(f->second.get());
  if(!s)
    throw std::runtime_error("requested wrong type for configuration item "+name);
  return s->getIPEndpoint();
}


string KolmoStruct::getString(const std::string& name)
{
  auto f=d_store.find(name);
  if(f==d_store.end())
    throw std::runtime_error("requested non-existent string configuration item '"+name+"'");
  auto s=dynamic_cast<KolmoString*>(f->second.get());
  if(!s)
    throw std::runtime_error("requested wrong type (string) for configuration item "+name);
  return s->d_v;
}


void KolmoStruct::setString(const std::string& name, const std::string& val)
{
  auto f=d_store.find(name);
  if(f==d_store.end())
    throw std::runtime_error("requested non-existent string configuration item '"+name+"'");
  auto s=dynamic_cast<KolmoString*>(f->second.get());
  if(!s)
    throw std::runtime_error("requested wrong type (string) for configuration item "+name);
  s->d_v = val;
}

// THIS ONE IS WEIRD AND UNLIKE ALL THE OTHERS
void KolmoStruct::setStruct(const std::string& name, std::unique_ptr<KolmoStruct> s)
{
  d_store[name]=std::move(s);
}


void KolmoStruct::addValueToStruct(const std::string& name, const std::string& val)
{
  auto f=d_store.find(name);
  if(f==d_store.end())
    throw std::runtime_error("requested non-existent configuration item "+name);
  auto s=dynamic_cast<KolmoStruct*>(f->second.get());
  if(!s)
    throw std::runtime_error("requested wrong type struct for configuration item "+name);
  //  cout<<"Adding to struct called "<<name<<", description: '"<<s->description<<"'"<<endl;
  auto iter = d_prototypes.find(s->d_membertype);
  if(iter == d_prototypes.end())
    throw std::runtime_error("configuration item "+name+" should contain "+s->d_membertype+" but can't find it");
  auto thing=iter->second->clone();
  thing->setValue(val);
  s->d_store[std::to_string(s->d_store.size())]=std::move(thing);
}

void KolmoStruct::addValue(const std::string& val)
{
  auto iter = d_prototypes.find(d_membertype);
  if(iter == d_prototypes.end())
    throw std::runtime_error("configuration item should contain '"+d_membertype+"' but can't find it");
  auto thing=iter->second->clone();
  thing->setValue(val);
  d_store[std::to_string(d_store.size())]=std::move(thing);
}

void KolmoStruct::setBool(const std::string& name, bool val)
{
  auto f=d_store.find(name);
  if(f==d_store.end())
    throw std::runtime_error("requested non-existent configuration item "+name);
  auto s=dynamic_cast<KolmoBool*>(f->second.get());
  if(!s)
    throw std::runtime_error("requested wrong type (bool) for configuration item "+name);
  s->setBool(val);
}

void KolmoStruct::setInteger(const std::string& name, uint64_t val)
{
  auto f=d_store.find(name);
  if(f==d_store.end())
    throw std::runtime_error("requested non-existent configuration item "+name);
  auto s=dynamic_cast<KolmoInteger*>(f->second.get());
  if(!s)
    throw std::runtime_error("requested wrong type (integer) for configuration item "+name);
  s->setInteger(val);
}

// this should take a ComboAddress, but for Lua this is easier
void KolmoStruct::setIPEndpoint(const std::string& name, const std::string& in)
{
  auto f=d_store.find(name);
  if(f==d_store.end())
    throw std::runtime_error("requested non-existent configuration item "+name);
  auto s=dynamic_cast<KolmoIPEndpoint*>(f->second.get());
  if(!s)
    throw std::runtime_error("requested wrong type (ipendpoint) for configuration item "+name);
  s->setIPEndpoint(ComboAddress(in));
}

// this should take a ComboAddress, but for Lua this is easier
void KolmoStruct::setIPEndpointCA(const std::string& name, const ComboAddress& ca)
{
  auto f=d_store.find(name);
  if(f==d_store.end())
    throw std::runtime_error("requested non-existent configuration item "+name);
  auto s=dynamic_cast<KolmoIPEndpoint*>(f->second.get());
  if(!s)
    throw std::runtime_error("requested wrong type (ipendpoint) for configuration item "+name);
  s->setIPEndpoint(ca);
}



KolmoStruct* KolmoStruct::getStruct(const std::string& name)
{
  auto f=d_store.find(name);
  if(f==d_store.end())
    throw std::runtime_error("requested non-existent configuration item "+name);
  auto s=dynamic_cast<KolmoStruct*>(f->second.get());
  if(!s)
    throw std::runtime_error("requested wrong type (struct) for configuration item "+name);
  return s;
}

KolmoVal* KolmoStruct::getMember(const std::string& name) const
{
  auto f=d_store.find(name);
  if(f==d_store.end())
    throw std::runtime_error("requested non-existent configuration member item '"+name+"'");
  return f->second.get();
}

void KolmoStruct::tieBool(const std::string& name, std::atomic<bool>* target)
{
  auto ptr = dynamic_cast<KolmoBool*>(getMember(name));
  if(!ptr)
    throw std::runtime_error("Requested bool, but did not get one");

  ptr->tie([target](auto kv) {
      *target=dynamic_cast<const KolmoBool*>(kv)->getBool();
    });
}


/*

  diff is called with a template that describes 'us', and another KolmoStruct to compare against
  Every field of a struct is compared pair by pair
  Fields can be primitives like bool, string, integer
  Fields can also be structs themselves, containing further structs

  sites
      ds9a.nl
          name
          enabled
	  listeners
	     0
	         1.2.3.4
             1
                 2.3.4.5
      kolmo
          name
          enabled

   
  When recursing


*/

std::unique_ptr<KolmoStruct> KolmoStruct::diff(const KolmoStruct& templ, const KolmoStruct& other, int nest) const
{
  bool dbg=false;
  string prefix(nest, '\t');
  std::unique_ptr<KolmoStruct> ret=std::unique_ptr<KolmoStruct>(dynamic_cast<KolmoStruct*>(templ.clone().release()));
  if(dbg) cerr<<prefix<<"In diff"<<endl;

  set<std::string> visited;
  for(const auto& m : d_store) {
    if(dbg) cerr<<prefix<<"Looking at "<<m.first<<endl;
    visited.insert(m.first);
    if(auto ptr = dynamic_cast<const KolmoStruct*>(m.second.get())) {
      
      if(dbg) cerr<<prefix<<"Is a struct, membertype: "<<ptr->d_mytype<<endl;
      auto f = d_prototypes.find(ptr->d_mytype);
      if(f == d_prototypes.end()) {
	if(dbg) cerr<<prefix<<"Have a struct member of unknown default, template is empty"<<endl;
	auto subdiff=ptr->diff(KolmoStruct(), *dynamic_cast<KolmoStruct*>(other.getMember(m.first)), nest+1);
	if(subdiff->empty()) {
	  if(dbg) cerr<<prefix<<"Nothing came back"<<endl;
	  ret->unregisterVariable(m.first);
	}
	else
	  ret->setStruct(m.first, std::move(subdiff));
      }
      else {
	auto s=dynamic_cast<KolmoStruct*>(f->second.get());
	auto subdiff=ptr->diff(*s, *dynamic_cast<KolmoStruct*>(other.getMember(m.first)), nest+1);
	if(subdiff->empty()) {
	  if(dbg) cerr<<prefix<<"Nothing came back"<<endl;
	  ret->unregisterVariable(m.first);
	}
	else
	  ret->setStruct(m.first, std::move(subdiff));
      }
    }
    else if(auto ptr = dynamic_cast<const KolmoBool*>(m.second.get())) {
      if(dbg) cerr<<prefix<<"Comparing a bool"<<endl;
      auto rhs=dynamic_cast<const KolmoBool*>(other.getMember(m.first));
      if(*ptr != *rhs) {
	if(dbg) cerr<<prefix<<"Field "<<m.first<<" is different!!"<<endl;
	ret->setBool(m.first, rhs->getBool());
      }
      else
	ret->unregisterVariable(m.first);
    }
    else if(auto ptr = dynamic_cast<const KolmoString*>(m.second.get())) {
      if(dbg) cerr<<prefix<<"Comparing a string"<<endl;
      auto rhs=dynamic_cast<const KolmoString*>(other.getMember(m.first));
      if(*ptr != *rhs) {
	if(dbg) cerr<<prefix<<"Field "<<m.first<<" is different!!"<<endl;
	ret->setString(m.first, rhs->getValue());
      }
      else
	ret->unregisterVariable(m.first);
    }
    else if(auto ptr = dynamic_cast<const KolmoInteger*>(m.second.get())) {
      if(dbg) cerr<<prefix<<"Comparing an integer"<<endl;
      auto rhs=dynamic_cast<const KolmoInteger*>(other.getMember(m.first));
      if(*ptr != *rhs) {
	if(dbg) cerr<<prefix<<"Field "<<m.first<<" is different!!"<<endl;
	ret->setInteger(m.first, rhs->getInteger());
      }
      else
	ret->unregisterVariable(m.first);
    }
    else if(auto ptr = dynamic_cast<const KolmoIPEndpoint*>(m.second.get())) {
      if(dbg) cerr<<prefix<<"Comparing an IP Endpoint"<<endl;
      auto rhs=dynamic_cast<const KolmoIPEndpoint*>(other.getMember(m.first));
      if(*ptr != *rhs) {
	if(dbg) cerr<<prefix<<"Field "<<m.first<<" is different!!"<<endl;
	ret->setIPEndpointCA(m.first, rhs->getIPEndpoint());
      }
      else {
	if(dbg) cerr<<prefix<<"Was unchanged "<<ptr->getIPEndpoint().toStringWithPort()<<", "<<
	  rhs->getIPEndpoint().toStringWithPort()<<endl;
	ret->unregisterVariable(m.first);
      }
    }
  }
  if(dbg) cerr<<prefix<<"Done with ourselves, had "<<visited.size()<<" members, checking what er missed on other side"<<endl;
  for(const auto& rhs : other.getAll()) {
    if(dbg) cerr<<prefix<<"Other field: "<<rhs.first<<endl;
    if(visited.count(rhs.first)) {
      if(dbg) cerr<<prefix<<"Skipping other field: "<<rhs.first<<endl;
      continue;
    }

    auto newstruct=std::unique_ptr<KolmoStruct>(dynamic_cast<KolmoStruct*>(rhs.second->clone().release()));
    if(dbg) cerr<<prefix<<"Adding other field "<<rhs.first<<", type="<<newstruct->d_mytype<<endl;

    auto f = d_prototypes.find(newstruct->d_mytype);
    // XXX check here
    for(auto& m : newstruct->getAll()) {
      if(*m.second == *dynamic_cast<const KolmoStruct*>(f->second.get())->getMember(m.first))
          newstruct->unregisterVariable(m.first);
    }

    
    ret->setStruct(rhs.first, std::move(newstruct));
  }
  
  return ret;
}

void KSToJson(KolmoStruct* ks, nlohmann::json& x)
{
  x=nlohmann::json::object();
  for(const auto& m : ks->getAll()) {
    if(auto ptr=dynamic_cast<KolmoBool*>(m.second)) {
      x[m.first]=ptr->getBool();
    }
    else if(auto ptr=dynamic_cast<KolmoInteger*>(m.second)) {
      x[m.first]=ptr->getInteger();
    }
    else if(auto ptr=dynamic_cast<KolmoIPEndpoint*>(m.second)) {
      x[m.first]=ptr->getValue();
    }
    else if(auto ptr=dynamic_cast<KolmoString*>(m.second)) {
      x[m.first]=ptr->getValue();
    }
    else if(auto ptr=dynamic_cast<KolmoStruct*>(m.second)) {
      KSToJson(ptr, x[m.first]);
    }
  }
}


void JsonToKS(nlohmann::json& x, KolmoStruct* ks, int indent)
{
  string prefix(indent, '\t');
  bool dbg=false;
  
  if(dbg)cout<<prefix<<"membertype: '"<<ks->d_membertype<<"'"<<endl;
  for(auto iter=x.begin() ; iter != x.end(); ++iter) {
    if(dbg)cout<<prefix<<iter.key()<<endl;
    if(!ks->d_membertype.empty() && !ks->isMember(iter.key())) {
      if(dbg)cout<<prefix<<"This struct is a typed vector, creating entry of type "<<ks->d_membertype<<" for "<<iter.key()<<endl;

      // HURRRR!!! This should actually check if member type is a vector or not XXX
      if(ks->d_membertype=="ipendpoint") {
	ks->registerIPEndpoint(iter.key(), "");
      }
      else {
	auto nstruct=ks->getNewMember();
	ks->registerStructMember(iter.key(), nstruct);
      }
    }

    if(auto ptr=dynamic_cast<KolmoStruct*>(ks->getMember(iter.key()))) {
      if(dbg)cout<<prefix<<"This is a struct, recursing"<<endl;
      nlohmann::json nstruct;
      JsonToKS(iter.value(), ptr, indent+1);
    }
    else if(auto ptr=ks->getMember(iter.key())) {
      if(dbg)cout<<prefix<<"SETTING to "<<iter.value()<<endl;
      if(iter.value().is_boolean())
	ptr->setValue(iter.value().get<bool>() ? "true" : "false");
      else if(iter.value().is_string())
	ptr->setValue(iter.value().get<string>());
      else if(iter.value().is_number())
	ptr->setValue(std::to_string(iter.value().get<int>()));

    }
  }
}
