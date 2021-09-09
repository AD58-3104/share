#include "header.hpp"

int main(int argc, char const *argv[])
{
    boost::asio::io_service io_service;
    InfoShareServer server(7110);
    std::cout << "server clieted\n";
    std::this_thread::sleep_for(3s);
    server.terminate();
    std::cout << "terminated ";
    std::this_thread::sleep_for(3s);

    return 0;
}
