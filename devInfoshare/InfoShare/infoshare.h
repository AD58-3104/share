#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>
#include <string>
#include <mutex>
#include <functional>
#include <chrono>
#include <cstdint>
#include <deque>
#include <cassert>
#include "hpl_types.h"
#include "infoshare.pb.h"
#include "header.hpp"

namespace Citbrains
{
    namespace infosharemodule
    {
/*
        TODO!!!!
        commandstring 
        behavierstringをここにハードコーディングする
        
        
        */
//inli{ne変数を使っているのでc++17以上でしかコンパイル出来ない。
#ifdef __cpp_inline_variables
        // static_assert(false,"this environment has inline variables");確認用
        inline static constexpr int32_t NUM_PLAYERS = 4;
        inline static constexpr int32_t COMM_INFO_PORT0 = 7110; //!< CommInfoで使用するPORTのはじめのポート
                                                                //1:7110, 2:7111, 3:7112, 4:7113, 5:7114, 6:7115

        inline static constexpr int32_t MAX_COMM_INFO_OBJ = 7; //!< 共有するオブジェクトの最大値(はじめはボール)
        inline static constexpr int32_t MAX_STRING = 42;       //!< メッセージの最大値
        inline static constexpr int32_t MAX_BEHAVIOR_STRING = 32;

        inline static constexpr uint8_t COMM_NOT_EXIST = 0b0000'0000;     //!< 発見しないときの値　もう必要なさそうhasを使えば良いし
        inline static constexpr uint8_t COMM_EXIST = 0b0000'0001;         //!< 発見しかたどうかのフラグ
        inline static constexpr uint8_t COMM_OUR_SIDE = 0b0000'0010;      //!< 自チームかどうかのフラグ
        inline static constexpr uint8_t COMM_OPPOSITE_SIDE = 0b0000'0100; //!< 敵チームかどうかのフラグ

        inline static constexpr int32_t MAX_BLACK_POLES = 6; //!< 黒ポールの数
        inline static constexpr int32_t MAX_TARGET_POS = 1;
        inline static constexpr int32_t MAX_MAGENTA_OBJECTS = 3;                //!< マゼンタロボットの数
        inline static constexpr int32_t MAX_CYAN_OBJECTS = MAX_MAGENTA_OBJECTS; //!< シアンロボットの数

        inline static constexpr int32_t NO_MAGENTA = 0; //!< マゼンタの番号(色の番号とは異なる)
        inline static constexpr int32_t NO_CYAN = 1;    //!< シアンの番号
#else
        //jetsonで無理だったら実装する.書くと長すぎて見にくいので.
        static_assert(false, "this environment doesn't have inline variables");
#endif
        struct OtherRobotInfomation
        {
            enum class MutexTag : int32_t
            {
                OUR_ROBOT_GL,
                ENEMY_ROBOT_GL,
                BLACK_POLE_GL,
                TARGET_POS_VEC,
                RECV_TIME,
                LENGTH //最大数を知りたい時に使うので何か追加する時はこれの前に追加する。
            };
            //scoped_lockして
            OtherRobotInfomation(int32_t id, float (*timeFunc)()) : id_(id), timeFunc_(timeFunc)
            {
                assert((0 <= id) && (id <= 3));//内部で扱うidは0 indexed
                for (int32_t i = 0; i < static_cast<int32_t>(MutexTag::LENGTH); ++i)
                {
                    dataMutexes_.emplace_back();
                }
                setRecv_time();
            }
            void setTimeFunc(float (*timeFunc)())
            {
                timeFunc_ = timeFunc;
            }

            void setRecv_time(void)
            {
                std::lock_guard lock(dataMutexes_[static_cast<int32_t>(MutexTag::RECV_TIME)]);
                if (timeFunc_ != nullptr)
                {
                    recv_time_ = timeFunc_();
                }
                recv_time_ = (float)time(0);
            }

            const int32_t id_; //idは0スタートで管理する
            float (*timeFunc_)();
            std::deque<std::mutex> dataMutexes_; //vectorだとmutexのコピーコンストラクタを呼んでしまうのでdeque

            //------------atomic datas----------------
            std::atomic_uint32_t cf_own_ = 0;
            std::atomic_uint32_t cf_ball_ = 0; //fastにした方が良いのかもしれない
            std::atomic_uint32_t status_ = 0;  //なんかこれフラグの詰め合わせっぽいので分けてatomic_boolで持つべき。
            std::atomic_uint32_t fps_ = 0;
            std::atomic_uint32_t voltage_ = 0; //使ってない。
            std::atomic_uint32_t temperature_ = 0;
            std::atomic_uint32_t highest_servo_ = 0;
            std::atomic_bool is_detect_ball_ = false;
            std::atomic_uint32_t strategy_no_ = 0;
            std::atomic_uint32_t command_ = 0;               //文字列は長いので数字で管理する    returnで返すdataはどっかで文字列に変換して返すべき
            std::atomic_uint32_t current_behavior_name_ = 0; //上に同じく
            //-------------non atomic datas---------------
            float recv_time_ = 0.0;           //これ整数でよくね？？
            std::vector<Pos2D> our_robot_gl_; //扱いが良く分からんやつら
            std::vector<Pos2D> enemy_robot_gl_;
            std::vector<Pos2D> black_pole_gl_;
            std::vector<Pos2D> target_pos_vec_;
        };

        class InfoShare
        {
        public:
            InfoShare();
            InfoShare(const InfoShare &) = delete;
            InfoShare &operator=(const InfoShare &) = delete;
            ~InfoShare();

            //-----------------------other robot data getter--------------------------
            //自分のidを指定された場合0に相当するものを返す。
            [[nodiscard]] int32_t getcf_own(const int32_t &id) const noexcept;
            [[nodiscard]] int32_t getcf_ball(const int32_t &id) const noexcept;
            [[nodiscard]] int32_t getstatus(const int32_t &id) const noexcept;
            [[nodiscard]] int32_t getfps(const int32_t &id) const noexcept;
            [[nodiscard]] int32_t getvoltage(const int32_t &id) const noexcept;
            [[nodiscard]] int32_t gettemperature(const int32_t &id) const noexcept;
            [[nodiscard]] int32_t gethighest_servo(const int32_t &id) const noexcept;
            [[nodiscard]] bool    getis_detect_ball(const int32_t &id) const noexcept;
            [[nodiscard]] int32_t getstrategy_no(const int32_t &id) const noexcept;
            [[nodiscard]] int32_t getcommand(const int32_t &id) const noexcept;
            [[nodiscard]] int32_t getcurrent_behavior_name(const int32_t &id) const noexcept;
            //----mutex付きのやつ
            [[nodiscard]] float getrecv_time(const int32_t &id) const; //残念ながらscoped_lockは例外を投げるらしい
            [[nodiscard]] std::vector<Pos2D> getour_robot_gl(const int32_t &id) const;
            [[nodiscard]] std::vector<Pos2D> getenemy_robot_gl(const int32_t &id) const;
            [[nodiscard]] std::vector<Pos2D> getblack_pole_gl(const int32_t &id) const;
            [[nodiscard]] std::vector<Pos2D> gettarget_pos_vec(const int32_t &id) const;
            //-------------------------------------------------------------------------

            void terminate();
            void changeColor(const int32_t color);
            void setTimeFunc(float (*func)());
            void setup(const bool allow_broadcast_sending,const int32_t self_id, const int32_t our_color, const std::string ip_address, const int32_t port, float (*func)());
            float getTime() const; //getelapsedtimeとかの方が良いかも
            int32_t getOurcolor() const noexcept;
            int32_t getID() const noexcept;
            //TODO:名前変える
            int32_t sendCommonInfo /* setSharingDataAndSendInfomationにしたい */ (); //パラメータパックで受け取っても良いが、使われる場所がhplの一か所のみなので寧ろそうしない方が良さそう。

        private:
            int32_t self_id_;
            int32_t our_color_;
            std::string ip_address_;
            int32_t port_;
            float (*timeFunc_)();
            bool terminated_;

            std::vector<std::unique_ptr<Citbrains::infosharemodule::OtherRobotInfomation>> robot_data_list_;
            CitbrainsMessage::SharingData sharing_data_;
            std::function<void(std::string &&)> receivedDataHandler_;
            //server-----------------------------------------------
            std::unique_ptr<Server> server;
            //client-----------------------------------------------
            std::unique_ptr<Client> client;
        };

    }
}

/*
 * memo
 * IDは内部での扱いは0スタート、インターフェースでは1スタートとする。
 * hplから使っている必要なインターフェース
 * setup().多分IDと色とアドレスを設定する
 * terminate()普通にterminate
 * elapsed()recvTimeを返す これreceiveのハンドラに参照渡さないとまずいな
 * get_ourcolor
 * changecolor()
 * sendCommonInfo
 * 
 * -----一連のgetter
 * get_command()コマンドの文字列返す
 * target_pos_vec(Pos2Dのやつ)のgetter.
 * 
 * 
 * 
 */