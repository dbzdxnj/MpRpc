#pragma once

#include "google/protobuf/service.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "logger.h"

#include <memory>
#include <mymuduo/TcpServer.h>
#include <mymuduo/EventLoop.h>
#include <mymuduo/InetAddress.h>
#include <mymuduo/TcpConnection.h>
#include <string>
#include <unordered_map>
#include <google/protobuf/descriptor.h>

// 框架提供的专门负责发布rpc服务的网络对象类
class RpcProvider
{
public:
    // 框架提供给外部使用的 可以发布rpc方法的函数接口
    void NotifyService(google::protobuf::Service *service);

    // 启动rpc服务节点，开始提供rpc远程网络调用服务
    void run();
private:
    // 组合了EventLoop
    EventLoop m_eventLoop_;

    struct ServiceInfo
    {
        // 保存服务对象
        google::protobuf::Service *m_service_;
        // 服务对象里面方法描述 映射表
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> m_methodMap_;
    };

    // 存储注册成功的服务对象及其服务方法的所有信息
    std::unordered_map<std::string, ServiceInfo> m_serviceMap_;
    
    // 新的socket连接回调
    void OnConnection(const TcpConnectionPtr&);

    // 消息读写回调
    void OnMessage(const TcpConnectionPtr&, Buffer*, Timestamp);

    // Closure的回调操作，用于序列化rpc的响应和网络发送
    void SendRpcResponse(const TcpConnectionPtr&, google::protobuf::Message*);
};