#include <iostream>
#include <string>
#include "infoshare.pb.h"
using namespace std;

struct St{
    #ifdef  __cpp_inline_variables
    inline static constexpr int I = 0b1111'1111;
    #else 
    static const int I = 0b1111'1001;
    #endif
};


int main(int argc, char const *argv[])
{
    CitbrainsMessage::SharingData proto;
    string pack;
    int i = 12;
    uint8_t c = St::I;
    cout << static_cast<int>(c) <<  endl;
    // pack = string{i} + string{12} + string{23} + string{43}+ string{98}+ string{93}+ string{73}+ string{74};
    // proto.set_pack(pack);
    // proto.set_x(1024);
    // proto.set_id( string{i});
    // proto.set_cf_own(string{i});
    // proto.set_cf_ball(string{i});
    // proto.set_status(string{i});
    // proto.set_fps(string{i});
    // proto.set_voltage(string{i});
    // proto.set_temperature(string{i});
    // proto.set_highest_servo(string{i});
    // string command = "walking for ball";
    // // proto.set_command(command);
    // string beh = "attacker now";
    // cout << command.length() << "::" << beh.length() << endl;
    // // proto.set_current_behavior_name(beh);
    // string Bytes = proto.SerializeAsString();
    // cout << sizeof(proto) << endl;
    // cout << Bytes.length();
    return 0;
}
