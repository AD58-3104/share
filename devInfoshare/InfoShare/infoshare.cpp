#include "infoshare.h"
#include <array>
#include <boost/asio.hpp>
#include <ctime>
using namespace std::literals::chrono_literals;
namespace Citbrains
{
    namespace infosharemodule
    {
        //デフォルトの時間関数を使いたい場合は引数無し
        //必ずsetupを呼ぶ。
        InfoShare::InfoShare()
            : self_id_(1), our_color_(COLOR_MAGENTA), ip_address_("127.0.0.1"), port_(7110), timeFunc_(nullptr), terminated_(false)
        {
        }
        InfoShare::~InfoShare()
        {
            terminate();
        }

        // void InfoShare::receiveSharedInfomation()
        // {
        //     boost::asio::io_service io_service; //毎回構築していいのかしら。https://docs.microsoft.com/ja-jp/dotnet/framework/network-programming/asynchronous-server-socket-example を見ると流石に毎回構築してないので一度だけ作る。
        //     boost::asio::ip::udp::socket socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), COMM_INFO_PORT0 + self_id_));
        //     std::array<std::byte, 1024> receive_buffer;
        //     while (!terminated_)
        //     {
        //         try
        //         {
        //             while (!terminated_)
        //             {
        //                 if (!socket.available())
        //                 {
        //                     std::this_thread::sleep_for(50ms);
        //                     continue;
        //                 }
        //                 boost::system::error_code error;
        //                 boost::asio::ip::udp::endpoint remote_endpoint;
        //                 size_t receive_length = socket.receive_from(boost::asio::buffer(receive_buffer), remote_endpoint, 0, error);
        //                 if (error)
        //                     continue;
        //                 //いらないっぽいif(){} //TODO:ここでハッシュのチェックでもする？？ダメだったら落とすみたいな
        //             }
        //         }
        //         catch (const std::exception &e)
        //         {
        //             std::cerr << e.what() << std::endl;
        //         }
        //     }
        // }
        void InfoShare::terminate()
        {
            //clientとserverのio_serviceをstopする為のterminateを呼ぶ。
            terminated_ = true;
        }
        void InfoShare::changeColor(const int32_t color)
        {
            our_color_ = color;
        }
        void InfoShare::setTimeFunc(float (*func)())
        {
            timeFunc_ = func;
            //TODO implement 全部にセットする
            for (auto &&itr : robot_data_list_)
            {
                std::lock_guard lock(itr->dataMutexes_[static_cast<int32_t>(OtherRobotInfomation::MutexTag::RECV_TIME)]);
                itr->timeFunc_ = func;
            }
        }
        //既にtimefuncを設定済みの場合は渡さなければ変更されない。
        void InfoShare::setup(const bool allow_broadcast_sending,const int32_t self_id = 1, const int32_t our_color = COLOR_MAGENTA, const std::string ip_address = "127.0.0.1", int32_t port = 7110, float (*timefunc)() = nullptr)
        {
            assert(self_id >= 1); //self id must be 1 or more
            self_id_ = self_id;
            ip_address_ = ip_address;
            our_color_ = our_color;
            if (timefunc != nullptr)
            {
                timeFunc_ = timefunc;
            }
            for (int32_t i = 0; i < NUM_PLAYERS; ++i)
            {
                robot_data_list_.push_back(std::make_unique<Citbrains::infosharemodule::OtherRobotInfomation>(i, timeFunc_));
            }
            receivedDataHandler_ = [&](std::string &&data) { //全てキャプチャするのは嫌だが取り敢えずこうしておく。参照の寿命とラムダの寿命は同じ。
                //メンバにする方が良いかな。多分そっちの方が良いな
                std::string s(std::move(data));
                CitbrainsMessage::SharingData shared_data;
                shared_data.ParseFromString(s);
                auto &set_target = robot_data_list_[static_cast<uint32_t>(shared_data.id().at(0))]; //atだからout of rangeで落ちる。
                //------------data set-----------
                set_target->setRecv_time();
                set_target->cf_ball_.store(static_cast<uint32_t>(shared_data.cf_ball().at(0)));
                set_target->status_.store(static_cast<uint32_t>(shared_data.status().at(0)));
                set_target->fps_.store(static_cast<uint32_t>(shared_data.fps().at(0)));
                set_target->voltage_.store(static_cast<uint32_t>(shared_data.voltage().at(0)));
                set_target->temperature_.store(static_cast<uint32_t>(shared_data.temperature().at(0)));
                set_target->highest_servo_.store(static_cast<uint32_t>(shared_data.highest_servo().at(0)));
                set_target->command_.store(static_cast<uint32_t>(shared_data.command().at(0)));
                set_target->current_behavior_name_.store(static_cast<uint32_t>(shared_data.current_behavior_name().at(0)));
                set_target->is_detect_ball_.store(shared_data.is_detect_ball());
                //--------object data set--------
                if (0 < shared_data.our_robot_gl_size()) //持ってる時
                {
                    std::lock_guard lock(set_target->dataMutexes_[static_cast<int32_t>(OtherRobotInfomation::MutexTag::OUR_ROBOT_GL)]);
                    set_target->our_robot_gl_.clear(); //どうせまた格納されるのでshrink_to_fitしない。
                    for (int32_t  i = 0; i < shared_data.our_robot_gl_size(); i++)
                    {
                        auto itr = shared_data.mutable_our_robot_gl(i);
                        set_target->our_robot_gl_.emplace_back(static_cast<float>(itr->pos_x()/100.0),static_cast<float> (itr->pos_y()/100.0),static_cast<float>(itr->pos_th()/100.0));
                    }
                }
                if (0 < shared_data.enemy_robot_gl_size()) //持ってる時
                {
                    std::lock_guard lock(set_target->dataMutexes_[static_cast<int32_t>(OtherRobotInfomation::MutexTag::ENEMY_ROBOT_GL)]);
                    set_target->enemy_robot_gl_.clear(); //どうせまた格納されるのでshrink_to_fitしない。
                    for (int32_t  i = 0; i < shared_data.enemy_robot_gl_size(); i++)
                    {
                        auto itr = shared_data.mutable_enemy_robot_gl(i);
                        set_target->enemy_robot_gl_.emplace_back(static_cast<float>(itr->pos_x()/100.0),static_cast<float> (itr->pos_y()/100.0),static_cast<float>(itr->pos_th()/100.0));
                    }
                }
                if (0 < shared_data.black_pole_gl_size()) //持ってる時
                {
                    std::lock_guard lock(set_target->dataMutexes_[static_cast<int32_t>(OtherRobotInfomation::MutexTag::BLACK_POLE_GL)]);
                    set_target->black_pole_gl_.clear(); //どうせまた格納されるのでshrink_to_fitしない。
                    for (int32_t  i = 0; i < shared_data.black_pole_gl_size(); i++)
                    {
                        auto itr = shared_data.mutable_black_pole_gl(i);
                        set_target->black_pole_gl_.emplace_back(static_cast<float>(itr->pos_x()/100.0),static_cast<float> (itr->pos_y()/100.0),static_cast<float>(itr->pos_th()/100.0));
                    }
                }
                if (0 < shared_data.target_pos_vec_size()) //持ってる時
                {
                    std::lock_guard lock(set_target->dataMutexes_[static_cast<int32_t>(OtherRobotInfomation::MutexTag::TARGET_POS_VEC)]);
                    set_target->target_pos_vec_.clear(); //どうせまた格納されるのでshrink_to_fitしない。
                    for (int32_t  i = 0; i < shared_data.target_pos_vec_size(); i++)
                    {
                        auto itr = shared_data.mutable_target_pos_vec(i);
                        set_target->target_pos_vec_.emplace_back(static_cast<float>(itr->pos_x()/100.0),static_cast<float> (itr->pos_y()/100.0),static_cast<float>(itr->pos_th()/100.0));
                    }
                }
            };
            client = std::make_unique<Client>(ip_address_, port_,allow_broadcast_sending); //TODO そういやブロードキャストでは？
            server = std::make_unique<Server>(port_, receivedDataHandler_);
        }
        
        int32_t InfoShare::getOurcolor() const noexcept
        {
            return our_color_;
        }
        int32_t InfoShare::getID() const noexcept
        {
            return self_id_;
        }
        float InfoShare::getTime() const
        {
            if (timeFunc_ != nullptr)
            {
                return timeFunc_();
            }
            return (float)time(0);
        }

        [[nodiscard]] int32_t InfoShare::getcf_own(const int32_t &id) const noexcept
        {
            if (id == self_id_)
            {
                return 0;
            }
            else
            {
                return robot_data_list_[id - 1]->cf_own_.load();
            }
        }
        [[nodiscard]] int32_t InfoShare::getcf_ball(const int32_t &id) const noexcept
        {
            if (id == self_id_)
            {
                return 0;
            }
            else
            {
                return robot_data_list_[id - 1]->cf_ball_.load();
            }
        }
        [[nodiscard]] int32_t InfoShare::getstatus(const int32_t &id) const noexcept
        {
            if (id == self_id_)
            {
                return 0;
            }
            else
            {
                return robot_data_list_[id - 1]->status_.load();
            }
        }
        [[nodiscard]] int32_t InfoShare::getvoltage(const int32_t &id) const noexcept
        {
            if (id == self_id_)
            {
                return 0;
            }
            else
            {
                return robot_data_list_[id - 1]->voltage_.load();
            }
        }
        [[nodiscard]] int32_t InfoShare::getfps(const int32_t &id) const noexcept
        {
            if (id == self_id_)
            {
                return 0;
            }
            else
            {
                return robot_data_list_[id - 1]->fps_.load();
            }
        }
        [[nodiscard]] int32_t InfoShare::gettemperature(const int32_t &id) const noexcept
        {
            if (id == self_id_)
            {
                return 0;
            }
            else
            {
                return robot_data_list_[id - 1]->temperature_.load();
            }
        }
        [[nodiscard]] int32_t InfoShare::gethighest_servo(const int32_t &id) const noexcept
        {
            if (id == self_id_)
            {
                return 0;
            }
            else
            {
                return robot_data_list_[id - 1]->highest_servo_.load();
            }
        }
        [[nodiscard]] bool InfoShare::getis_detect_ball(const int32_t &id) const noexcept
        {
            if (id == self_id_)
            {
                return false;
            }
            else
            {
                return robot_data_list_[id - 1]->is_detect_ball_.load();
            }
        }
        [[nodiscard]] int32_t InfoShare::getstrategy_no(const int32_t &id) const noexcept
        {
            if (id == self_id_)
            {
                return 0;
            }
            else
            {
                return robot_data_list_[id - 1]->strategy_no_.load();
            }
        }
        [[nodiscard]] int32_t InfoShare::getcommand(const int32_t &id) const noexcept
        {
            if (id == self_id_)
            {
                return 0;
            }
            else
            {
                return robot_data_list_[id - 1]->command_.load();
            }
        }
        [[nodiscard]] int32_t InfoShare::getcurrent_behavior_name(const int32_t &id) const noexcept
        {
            if (id == self_id_)
            {
                return 0;
            }
            else
            {
                return robot_data_list_[id - 1]->current_behavior_name_.load();
            }
        }
        [[nodiscard]] float InfoShare::getrecv_time(const int32_t &id) const
        {
            if (id == self_id_)
            {
                return 0.0;
            }
            else
            {
                std::lock_guard lock(robot_data_list_[id - 1]->dataMutexes_[static_cast<int32_t>(OtherRobotInfomation::MutexTag::RECV_TIME)]);
                return (robot_data_list_[id - 1]->recv_time_);
            }
        }
        [[nodiscard]] std::vector<Pos2D> InfoShare::getour_robot_gl(const int32_t &id) const
        {
            if (id == self_id_)
            {
                return {Pos2D()};
            }
            else
            {
                std::lock_guard lock(robot_data_list_[id - 1]->dataMutexes_[static_cast<int32_t>(OtherRobotInfomation::MutexTag::OUR_ROBOT_GL)]);
                return (robot_data_list_[id - 1]->our_robot_gl_);
            }
        }
        [[nodiscard]] std::vector<Pos2D> InfoShare::getenemy_robot_gl(const int32_t &id) const
        {
            if (id == self_id_)
            {
                return {Pos2D()};
            }
            else
            {
                std::lock_guard lock(robot_data_list_[id - 1]->dataMutexes_[static_cast<int32_t>(OtherRobotInfomation::MutexTag::ENEMY_ROBOT_GL)]);
                return (robot_data_list_[id - 1]->enemy_robot_gl_);
            }
        }
        [[nodiscard]] std::vector<Pos2D> InfoShare::getblack_pole_gl(const int32_t &id) const
        {
            if (id == self_id_)
            {
                return {Pos2D()};
            }
            else
            {
                std::lock_guard lock(robot_data_list_[id - 1]->dataMutexes_[static_cast<int32_t>(OtherRobotInfomation::MutexTag::BLACK_POLE_GL)]);
                return (robot_data_list_[id - 1]->black_pole_gl_);
            }
        }
        [[nodiscard]] std::vector<Pos2D> InfoShare::gettarget_pos_vec(const int32_t &id) const
        {
            if (id == self_id_)
            {
                return {Pos2D()};
            }
            else
            {
                std::lock_guard lock(robot_data_list_[id - 1]->dataMutexes_[static_cast<int32_t>(OtherRobotInfomation::MutexTag::TARGET_POS_VEC)]);
                return (robot_data_list_[id - 1]->target_pos_vec_);
            }
        }

    }
}