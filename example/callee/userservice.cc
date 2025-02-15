#include <iostream>
#include <string>

#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "logger.h"

// UserService原来是一个本地服务，提供了两个进程内的本地方法：Login GetFriendLists

class UserService : public fixbug::UserServiceRpc
{
public:
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "doing local service : Login " << std::endl;
        std::cout << "name : " << name << "pwd : " << pwd << std::endl;
        return true;
    }

    // 重写基类UserServiceRpc的虚函数
    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)
    {
        // 框架给业务上报了请求参数LoginRequest，应用获取相应数据做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 做本地业务
        bool login_result = Login(name, pwd); 

        // 把响应写入response 
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        // 执行回调操作  执行响应对象数据的序列化和网络发送（由框架完成）
        done->Run();
    }
};

int main(int argc, char **argv)
{
    //调用框架初始化操作
    MprpcApplication::Init(argc, argv);

    // provider是一个rpc网络服务对象，把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new UserService());

    //启动一个rpc网络发布节点 Run以后，进程进入阻塞状态，等待远程rpc调用请求
    provider.run();

    return 0;
}