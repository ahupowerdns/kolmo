#include "minicurl.hh"
#include <curl/curl.h>
#include <stdexcept>

MiniCurl::MiniCurl()
{
  d_curl = curl_easy_init();
}

MiniCurl::~MiniCurl()
{
  curl_easy_cleanup(d_curl);
}

size_t MiniCurl::write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  MiniCurl* us = (MiniCurl*)userdata;
  us->d_data.append(ptr, size*nmemb);
  return size*nmemb;
}

void MiniCurl::setupURL(const std::string& str)
{
  curl_easy_setopt(d_curl, CURLOPT_URL, str.c_str());
  curl_easy_setopt(d_curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(d_curl, CURLOPT_WRITEDATA, this);
  d_data.clear();
}
std::string MiniCurl::getURL(const std::string& str)
{
  setupURL(str);
  auto res = curl_easy_perform(d_curl);
  if(res != CURLE_OK)
    throw std::runtime_error("Retrieving URL: "+std::string(curl_easy_strerror(res)));
  std::string ret=d_data;
  d_data.clear();
  return ret;
}

std::string MiniCurl::postURL(const std::string& str, const std::string& postdata)
{
  setupURL(str);
  curl_easy_setopt(d_curl, CURLOPT_POSTFIELDS, postdata.c_str());

  auto res = curl_easy_perform(d_curl);
  if(res != CURLE_OK)
    throw std::runtime_error("Posting to URL: "+std::string(curl_easy_strerror(res)));
  std::string ret=d_data;

  d_data.clear();
  return ret;
}
