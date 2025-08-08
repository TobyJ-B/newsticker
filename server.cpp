#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fstream>
#include <set>
#include <deque>


std::vector<int> client_sockets;
std::mutex client_mutex;
std::vector<std::string> last_sent_news;
std::set<std::string> seen_urls;
std::deque<std::string> recent_news;
std::mutex news_mutex;

const std::string NEWS_FILE = "news_history.txt";
const std::string URLS_FILE = "seen_urls.txt";
const int MAX_RECENT_NEWS = 100;


// == Storage == 

void loadRecentNews(){
  std::ifstream file(NEWS_FILE);
  std::string line;
  while(std::getline(file, line)){
    if (!line.empty()){
      recent_news.push_back(line);
    }
  }
  while(recent_news.size() > MAX_RECENT_NEWS){
    recent_news.pop_front();
  }
  std::cout << "Loaded " << recent_news.size() << " recent news items" << std::endl;
}

void saveNewsItem(const std::string& news_item){
  std::lock_guard<std::mutex> lock(news_mutex);

  recent_news.push_back(news_item);
  if (recent_news.size() > MAX_RECENT_NEWS){
    recent_news.pop_front();
  }

  //append to end of file
  std::ofstream file(NEWS_FILE, std::ios::app);
  file << news_item << std::endl;
}

void loadSeenUrls(){
  std::ifstream file(URLS_FILE);
  std::string url;
  while (std::getline(file, url)){
    if(!url.empty()){
      seen_urls.insert(url);
    }
  }
  std::cout << "Loaded " << seen_urls.size() << " seen URLs" << std::endl;
}

void saveSeenUrl(const std::string& url){
  std::ofstream file(URLS_FILE, std::ios::app);
  file << url << std::endl;
}


// == News helper == 

//Find last item
//If valid return item if not return empty string
std::string extractUrl(const std::string& news_item){
  size_t last_pipe = news_item.find_last_of('|');
  return (last_pipe != std::string::npos) ? news_item.substr(last_pipe + 1) : "";
}


//Looks in seen_urls and returns if current url is seen.
//If the url is new return true
//If the url is not new return false
bool isNewsItemNew(const std::string& news_item){
  std::string url = extractUrl(news_item);
  if(url.empty()) return false;

  return seen_urls.find(url) == seen_urls.end();
}

void markNewsItemAsSeen(const std::string& news_item){
  std::string url = extractUrl(news_item);
  if (!url.empty() && seen_urls.find(url) == seen_urls.end()){
    seen_urls.insert(url);
    saveSeenUrl(url);
  }
}

std::vector<std::string> fetchnews() {
    std::vector<std::string> news;
    FILE* pipe = popen("./getnews", "r");
    if (!pipe) {
        std::cerr << "Failed to get news\n";
        return news;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        news.emplace_back(buffer);
    }

    pclose(pipe);
    return news;
}


// == Broadcasting ==

void broadcastNews(const std::string& msg) {
    std::lock_guard<std::mutex> lock(client_mutex);
    for (int sock : client_sockets) {
        send(sock, msg.c_str(), msg.size(), 0);
    }
}

void sendRecentNewsToClient(int client_sock){
  std::lock_guard<std::mutex> lock(news_mutex);
  int start_idx = std::max(0, (int)recent_news.size() - 20);
  for (int i = start_idx; i < recent_news.size(); i++){
    std::string msg = recent_news[i] + "\n"; 
    send(client_sock, msg.c_str(), msg.size(), 0);
  }
}

void newsBroadcaster() {
  while (true) {
    std::vector<std::string> news = fetchnews();
    int new_items_count = 0;

    for (const std::string& item : news){
      if (item.empty()) continue;

      if(isNewsItemNew(item)){
        markNewsItemAsSeen(item);
        saveNewsItem(item);
        broadcastNews(item);
        new_items_count++;
      }
    }

    if (new_items_count == 0){
      std::cout << "No new news found" << std::endl;
    } else {
      std::cout << "Broadcasted " << new_items_count << "new items." << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
}


// == Client handling ==

void handle_client(int client_sock) {
    sendRecentNewsToClient(client_sock);

    char buffer[4096];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0) {
            std::cout << "Client disconnected\n";
            break;
        }
        // You can optionally handle client messages here if you want
    }
    close(client_sock);
    {
        // FIXED: Typo: std::lock_guard (not lockguard)
        std::lock_guard<std::mutex> lock(client_mutex);
        client_sockets.erase(std::remove(client_sockets.begin(), client_sockets.end(), client_sock), client_sockets.end());
    }
}

void accept_clients(int server_sock) {
    sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);

    // FIXED: use correct variable and syntax here
    int client_sock = accept(server_sock, (sockaddr*)&client_addr, &client_size);

    if (client_sock == -1) {
        std::cerr << "Failed to accept client\n";
        return;  // use return here since it's not inside a loop
    }

    {
        std::lock_guard<std::mutex> lock(client_mutex);
        client_sockets.push_back(client_sock);
    }

    std::thread(handle_client, client_sock).detach();
    std::cout << "New client connected\n";
}

int main() {

    loadSeenUrls();
    loadRecentNews();

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(54000);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind socket\n";
        return 1;
    }

    if (listen(server_sock, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen on socket\n";
        return 1;
    }

    // Start news broadcaster thread
    std::thread news_thread(newsBroadcaster);
    news_thread.detach();

    while (true) {
        accept_clients(server_sock);
    }

    close(server_sock);
    return 0;
}

