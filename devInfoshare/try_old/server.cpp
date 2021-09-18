#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <thread>
#include <chrono>
#include <string>

using boost::asio::ip::udp;
using namespace std::literals::chrono_literals;
class InfoShareServer
{

    boost::asio::io_service io_service_;
    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    bool terminated_;
    boost::asio::streambuf receive_buff_;
    static const int32_t buffer_size_ = 1024;
    int32_t port_;
    std::thread server_thread_;

public:                     
    InfoShareServer(boost::asio::io_service &io_service,int32_t port) //コンストラクタでRobotstatusの参照を渡しておく
        : io_service_(),
          socket_(io_service_, udp::endpoint(udp::v4(), port)),terminated_(false),port_(port),server_thread_([this](){io_service_.run();})
    {
        std::this_thread::sleep_for(100ms);
        startReceive();
    }
    ~InfoShareServer(){
        terminate();
    }
    // 受信開始
    void startReceive()
    {
        socket_.async_receive_from(
            receive_buff_.prepare(buffer_size_),
            remote_endpoint_,
            // boost::bind(&InfoShareServer::receiveHandler, this,
            //             asio::placeholders::error, asio::placeholders::bytes_transferred)
            [this](const boost::system::error_code &error, size_t bytes_transferred){
                receiveHandler(error,bytes_transferred);
            }
            );
    }

    // 受信のハンドラ
    void receiveHandler(const boost::system::error_code &error, size_t bytes_transferred)
    {
        if (error && error != boost::asio::error::eof)
        {
            std::cout << "receive failed: " << error.message() << std::endl;
        }
        else
        {
            // const char* data = asio::buffer_cast<const char*>(receive_buff_.data());
            const std::string data(boost::asio::buffer_cast<const char*>(receive_buff_.data()),bytes_transferred);
            // const std::string data(boost::asio::buffer_cast<const char*>(receive_buff_.data()),bytes_transferred);
            std::cout << data  << "::length " << bytes_transferred << std::endl;
            receive_buff_.consume(receive_buff_.size());
            if (data.compare("end") == 0)
            {
                terminated_ = true;
            }
            // receive_buff_.consume(receive_buff_.size());
            if (!terminated_)
            {
                startReceive();
            }
        }
    }
    void terminate(){
        io_service_.stop();
        server_thread_.join();
    };
};

int main()
{
   boost::asio::io_service io_service;
    InfoShareServer server(io_service,7110);
    std::this_thread::sleep_for(4s);
    server.terminate();
    // io_service.run();
}