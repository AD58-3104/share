#include "header.hpp"

int main(int argc, char const *argv[])
{

    Client client("127.0.0.1",7110);

    
    for(int i = 0;i < 5;++i)
        client.send(std::string("fuck"));
    client.send(std::string("end"));
    std::cout << "running\n";
    std::cout << "\n\nend !!!!!!!!!!!!!!!!!!!!!!!!!!!";
    return 0;
}
