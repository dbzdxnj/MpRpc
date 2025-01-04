#include "test.pb.h"
#include <iostream>

using namespace fixbug;

// int func1()
// {
//     // 封装了login请求对象的数据
//     LoginRequest req;
//     req.set_name("zhang san");
//     req.set_pwd("123456");

//     // 对象数据序列化
//     std::string send_str;
//     if (req.SerializeToString(&send_str))
//     {
//         std::cout << send_str << std::endl;
//     }

//     // 反序列化 从send_str反序列化一个login请求对象
//     LoginRequest reqB;
//     if (reqB.ParseFromString(send_str))
//     {
//         std::cout << reqB.name() << std::endl;
//         std::cout << reqB.pwd() << std::endl;
//     }
    
//     return 0;
// }

int main()
{
    // LoginResponse rsp;
    // resultCode *rc = rsp.mutable_result();
    // rc->set_errcode(1);
    // rc->set_errmsg("login false");

    GetFriendListResponse rsp;
    resultCode *rc = rsp.mutable_result();
    rc->set_errcode(0);
    rc->set_errmsg("get false");

    User *user1 = rsp.add_friendlist();
    user1->set_name("zhang san");
    user1->set_age(20);
    user1->set_sex(User::man);

    User *user2 = rsp.add_friendlist();
    user2->set_name("li si");
    user2->set_age(20);
    user2->set_sex(User::woman);

    std::cout << rsp.friendlist_size() << std::endl;

    for (int i = 0; i < rsp.friendlist_size(); ++i)
    {
        const User &temp = rsp.friendlist(i);
        std::cout << temp.name() << temp.age() << temp.sex() << std::endl;
    }
}