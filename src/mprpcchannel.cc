#include "mprpcchannel.h"
#include "rpcheader.pb.h"
#include "zookeeperutil.h"

#include <sys/socket.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

#include <google/protobuf/message.h>
#include <string>
#include <errno.h>

/*
数据格式：header_size + service_name method_name args_size + args_str
*/

// 所有通过stub代理对象调用的rpc方法都会执行这一步，统一做rpc方法调用的数据序列化和网络发送
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                                    google::protobuf::RpcController* controller, 
                                    const google::protobuf::Message* request,
                                    google::protobuf::Message* response, 
                                    google::protobuf::Closure* done)
{
    const google::protobuf::ServiceDescriptor* service_descriptor = method->service();
    std::string service_name = service_descriptor->name();
    std::string method_name = method->name();

    // 获取参数的序列化字符串长度
    int args_size = 0;
    std::string args_str;
    if (request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
        //std::cout << "Serialize request error !" << std::endl;
        controller->SetFailed("Serialize request error !");
        return ;
    }
    
    // 定义rpc的请求header
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if (rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        controller->SetFailed("Serialize rpc header error!");
        //std::cout << "Serialize rpc header error!" << std::endl;
        return;
    }
    
    // 组织待发送的rpc请求的字符串
    std::string send_rpc;
    send_rpc.insert(0, std::string((char*)&header_size, 4)); // header_size
    send_rpc += rpc_header_str; // rpc_header
    send_rpc += args_str;   // args

    // 打印调试信息
    std::cout << "==============================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_size: " << args_size << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "==============================" << std::endl;

    // 使用TCP编程完成rpc方法的远程调用
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        char errorText[512] = {0};
        sprintf(errorText, "create socket error! errno: %d \n", errno);
        controller->SetFailed(errorText);
        return;
    }
    
    // 读取配置rpcserver的信息
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());

    // rpc调用方想调用serveice_name和method_name的服务，需要查询zk上该服务所在的host信息
    ZkClient zkCli;
    zkCli.Start();
    std::string method_path = "/" + service_name + "/" + method_name;
    // host_data格式：127.0.0.1:9000
    std::string host_data = zkCli.GetDdate(method_path.c_str());
    if (host_data == "")
    {
        controller->SetFailed(method_path + " is not exist!");
        return;
    }
    int idx = host_data.find(":");
    if (idx == -1)
    {
        controller->SetFailed(method_name + " address is invalid!");
        return;
    }
    std::string ip = host_data.substr(0, idx);
    uint16_t port = atoi(host_data.substr(idx + 1, host_data.size() - idx).c_str());

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        close(clientfd);
        char errorText[512] = {0};
        sprintf(errorText, "connect error! errno: %d \n", errno);
        controller->SetFailed(errorText);
        return;
    }
    
    if (-1 == send(clientfd, send_rpc.c_str(), send_rpc.size(), 0))
    {
        close(clientfd);
        char errorText[512] = {0};
        sprintf(errorText, "send error! errno: %d \n", errno);
        controller->SetFailed(errorText);
        return;
    }
    
    // 接收rpc请求相应值
    char buf[1024] = {0};
    int recv_size = 0;
    if (-1 == (recv_size = recv(clientfd, buf, 1024, 0)))
    {
        close(clientfd);
        char errorText[512] = {0};
        sprintf(errorText, "recv error! errno: %d \n", errno);
        controller->SetFailed(errorText);
        return;
    }
    
    // 将接收到的数据放入到response中
    // std::string response_str(buf, 0, recv_size);
    // if (!response->ParseFromString(response_str))
    if (!response->ParseFromArray(buf, recv_size))
    {
        close(clientfd);
        char errorText[2048] = {0};
        sprintf(errorText, "parse error! response_str : %s \n", buf);
        controller->SetFailed(errorText);
        return;
    }
    
    close(clientfd);
}