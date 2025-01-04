#include <iostream>

#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"

int main(int argc, char **argv)
{
    // 整个程序启动以后，想使用mprpc框架来享受调用服务，
    // 必须先调用框架初始化函数(初始化一次)
    MprpcApplication::Init(argc, argv);

    // 演示调用远程发布的rpc方法Login
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");

    // rpc方法的响应
    fixbug::LoginResponse response;

    MprpcController controller;
    // 发起rpc方法的调用 同步rpc的调用过程 MprpcChannel::callmethod
    stub.Login(&controller, &request, &response, nullptr);

    if (controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        // rpc 调用完成 读响应结果response
        if (response.result().errcode() == 0)
        {
            std::cout << "rpc login response:" << response.success() << std::endl;
        }
        else
        {
            std::cout << "rpc login response error : " << response.result().errmsg() << std::endl;
        }
    }
    return 0;
}