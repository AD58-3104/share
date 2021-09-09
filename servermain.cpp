#include "header.hpp"
#include <vector>
int main(int argc, char const *argv[])
{
    boost::asio::io_service io_service;
    std::vector<std::string> stringContainer; 
    InfoShareServer server(7110,[&stringContainer](std::string&& s){
        std::string moved_string(std::move(s));
        std::cout << moved_string;
        stringContainer.push_back(moved_string);
    });
    std::cout << "server clieted\n";
    std::this_thread::sleep_for(3s);
    server.terminate();
    std::cout << "terminated ";
    std::this_thread::sleep_for(3s);
    for(const auto& itr:stringContainer)
        std::cout << itr << std::endl;
    return 0;
}
