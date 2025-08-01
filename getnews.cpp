#include <cpr/cpr.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <string>

const std::string apiKey = "d26fj7pr01qvraiq4ntgd26fj7pr01qvraiq4nu0";

struct NewsItem{
  std::string headline;
  std::string source;
  long long timestamp;
  std::string ticker;
  std::string summary;
  std::string url;
};

std::string serializeNewsItem(const NewsItem& item){
  return item.ticker + "|" + std::to_string(item.timestamp) + "|" + item.source + "|" + item.headline + "|" + item.url;
}

std::vector<std::string> getNews(const std::string& apiKey){
  std::vector<std::string> items;
  std::string url = "https://finnhub.io/api/v1/news?category=general&token=" + apiKey;



  auto response = cpr::Get(cpr::Url{url});
  std::cout << "HTTP status: " << response.status_code << std::endl;


  //Testing code for json
  //std::ofstream out("news_raw.json");
  //out << response.text;
  //out.close();


  if (response.status_code != 200) {
    std::cerr << "Failed to fetch news" << std::endl;
    return items;
  };

  auto json = nlohmann::json::parse(response.text);

  for (const auto& item : json){
    NewsItem news;
    news.headline = item.value("headline", "");
    news.timestamp = item.value("datetime", 0LL);
    news.source = item.value("source", "");
    news.ticker = item.value("related", "");
    news.summary = item.value("summary", "");
    news.url = item.value("url", "");

    items.push_back(serializeNewsItem(news));    
  };

  return items;

}


int main(){
  auto newsItems = getNews(apiKey);

  for(const auto& item : newsItems){
    std::cout << item << std::endl;
  }

  return 0;
}
