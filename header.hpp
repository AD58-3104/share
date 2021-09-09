#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <thread>
#include <memory>
#include <chrono>
#include <string>
using boost::asio::ip::udp;
using namespace std::literals::chrono_literals;

#if (((BOOST_VERSION / 100000) + ((BOOST_VERSION)/ 100 % 1000)) > 165)
//Boost1.65より新しい時(io_serviceがdeprecatedな為分けている)
/**/ #define BOOST_VERSION_IS_HIGHER_THAN_1_65
#else
// Boost1.65以下の時
/**/ #define BOOST_VERSION_IS_1_65_OR_LOWER
#endif //(((BOOST_VERSION / 1000) + ((BOOST_VERSION) % 1000)) >= 165)

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
    Client(std::string address, int32_t port)
        : io_service_(), socket_(io_service_), cnt(0), port_(port), ip_address_(address),
#ifdef BOOST_VERSION_IS_HIGHER_THAN_1_65
          w_guard_(boost::asio::make_work_guard(io_service_))
#else
          w_guard_(std::make_shared<boost::asio::io_service::work>(io_service_))
#endif //BOOST_VERSION_IS_HIGHER_THAN_1_65
    {
        try
        {
            if (socket_.is_open())
            {
                socket_.close();
            }
            socket_.open(udp::v4());
        }
        catch (boost::system::system_error &e)
        {
            e.what();
        }

        client_thread_ = std::make_unique<std::thread>([&]()
                                                       { io_service_.run(); });
    }
    ~Client()
    {
        terminate();
    }

    // メッセージ送信
    void send(std::string &&bytestring)
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

    // 送信のハンドラ
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
        static bool already_called = false;
        if (!already_called)
        {
            w_guard_.reset(); //ここでwork_guardかworkを破棄してrun()のブロッキングを終わらせる
            client_thread_->join();
        }
    };
};

class InfoShareServer
{

    boost::asio::io_service io_service_;
    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    bool terminated_;
    boost::asio::streambuf receive_buff_;
    static const int32_t buffer_size_ = 1024;
    int32_t port_;
    std::function<void(std::string&& s)> bytestringHandler_;
    std::unique_ptr<std::thread> server_thread_;
#ifdef BOOST_VERSION_IS_HIGHER_THAN_1_65
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> w_guard_;
#else
    std::shared_ptr<boost::asio::io_service::work> w_guard_;
#endif //BOOST_VERSION_IS_HIGHER_THAN_1_65

public:
    InfoShareServer(int32_t port,std::function<void(std::string&&)> func) //コンストラクタでRobotstatusの参照を渡しておく
        : io_service_(),
          socket_(io_service_, udp::endpoint(udp::v4(), port)), terminated_(false), port_(port),bytestringHandler_(func),
#ifdef BOOST_VERSION_IS_HIGHER_THAN_1_65
          w_guard_(boost::asio::make_work_guard(io_service_))
#else
          w_guard_(std::make_shared<boost::asio::io_service::work>(io_service_))
#endif //BOOST_VERSION_IS_HIGHER_THAN_1_65
    {
        server_thread_ = std::make_unique<std::thread>([&]()
                                                       { io_service_.run(); });
        startReceive();
    }
    ~InfoShareServer()
    {
        terminate();
    }
    // 受信開始
    void startReceive()
    {
        std::this_thread::sleep_for(10ms);
        if (!terminated_)
            socket_.async_receive_from(
                receive_buff_.prepare(buffer_size_),
                remote_endpoint_,
                // boost::bind(&InfoShareServer::receiveHandler, this,
                //             asio::placeholders::error, asio::placeholders::bytes_transferred)
                [this](const boost::system::error_code &error, size_t bytes_transferred)
                {
                    receiveHandler(error, bytes_transferred);
                });
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
            std::string data(boost::asio::buffer_cast<const char *>(receive_buff_.data()), bytes_transferred);
            std::cout << "length::" << bytes_transferred << " " << std::endl;
            bytestringHandler_(std::move(data));
            std::cout << data;
            receive_buff_.consume(receive_buff_.size());
            if (!terminated_)
            {
                startReceive();
            }
        }
    }
    void terminate()
    {
        static bool already_called = false;
        if (!already_called)
        {
            already_called = true;
            terminated_ = true;
            w_guard_.reset();
            socket_.cancel();
            io_service_.stop();
            server_thread_->join();
            std::cout << "server is terminated!!" << std::endl;
        }
    };
};
