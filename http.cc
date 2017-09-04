#include "http.hh"
#include <boost/utility/string_ref.hpp>
#include <boost/algorithm/string.hpp>

using std::string;

std::string pickContentType(const std::string& fname)
{
  const string charset = "; charset=utf-8";
  if(boost::ends_with(fname, ".html"))
    return "text/html" + charset;
  else if(boost::ends_with(fname, ".css"))
    return "text/css" + charset;
  else if(boost::ends_with(fname,".js"))
    return "application/javascript" + charset;
  else if(boost::ends_with(fname, ".png"))
    return "image/png";
  else if(boost::ends_with(fname, ".jpg"))
    return "image/jpg";

  return "text/html";
}
