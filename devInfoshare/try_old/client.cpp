#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <thread>
#include <memory>
#include <chrono>
using boost::asio::ip::udp;
using namespace std::literals::chrono_literals;

#if (((BOOST_VERSION / 1000) + ((BOOST_VERSION) % 1000)) > 165)
//Boost1.65より新しい時(io_serviceがdeprecatedな為分けている)
#define BOOST_VERSION_IS_HIGHER_THAN_1_65
#else
// Boost1.65以下の時
#define BOOST_VERSION_IS_1_65_OR_LOWER
#endif //(((BOOST_VERSION / 1000) + ((BOOST_VERSION) % 1000)) >= 165)

#undef BOOST_VERSION_IS_HIGHER_THAN_1_65

class Client
{
    boost::asio::io_service io_service_;
    udp::socket socket_;
    int cnt;
    int32_t port_;
    std::string ip_address_;
    std::unique_ptr<std::thread> client_thread_;
#ifdef BOOST_VERSION_IS_HIGHER_THAN_1_65
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> w_guard_;
#else
    std::shared_ptr<boost::asio::io_service::work> w_guard_;
#endif //BOOST_VERSION_IS_HIGHER_THAN_1_65

public:
    Client(int32_t port, std::string address)
        : io_service_(), socket_(io_service_), cnt(0), port_(port), ip_address_(address)
    {
        try
        {
            if (socket_.is_open())
            {
                socket_.close();
            }
            socket_.open(udp::v4());
        }
        catch(boost::system::system_error &e){
            e.what();
        }
#ifdef BOOST_VERSION_IS_HIGHER_THAN_1_65
        w_guard_ = boost::asio::make_work_guard(io_service_);
#else
        w_guard_ = std::make_shared<boost::asio::io_service::work>(io_service_);
#endif //BOOST_VERSION_IS_HIGHER_THAN_1_65
        client_thread_ = std::make_unique<std::thread>([&](){
            io_service_.run();
        });
    }
    ~Client(){
        terminate();
    }


    // void f(){
    //     send_data_ = "ping";
    //     if (cnt > 4)
    //     {
    //         send_data_.clear();
    //         send_data_.shrink_to_fit();
    //         send_data_ = "end";
    //     }
    //     cnt++;
    // };

    // メッセージ送信
    void send(std::string&& bytestring)
    {
        std::string send_data = std::move(bytestring);
        // boost::asio::ip::address adress;
        boost::asio::ip::udp::endpoint destination(boost::asio::ip::address::from_string(ip_address_), port_);
        socket_.async_send_to(
            boost::asio::buffer(send_data),
            destination,
            [this](const boost::system::error_code &error, size_t bytes_transferred)
            { sendHandler(error, bytes_transferred); });
    }

    // 送信完了
    // error : エラー情報
    // bytes_transferred : 送信したバイト数
    void sendHandler(const boost::system::error_code &error, size_t bytes_transferred)
    {
        if (error)
        {
            std::cout << "send failed: " << error.message() << std::endl;
        }
        else
        {
            std::cout << "send correct!" << std::endl;
        }
    }
    void terminate()
    {
        w_guard_.reset();//ここでwork_guardかworkを破棄してrun()のブロッキングを終わらせる
        client_thread_->join();
    };
};

int main()
{

    Client client(7110, "127.0.0.1");

    
    for(int i = 0;i < 5;++i)
        client.send(std::string("fuck"));
    client.send(std::string("end"));
    std::cout << "running\n";
    std::cout << "\n\nend !!!!!!!!!!!!!!!!!!!!!!!!!!!";
}