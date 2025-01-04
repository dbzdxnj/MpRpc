#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "zookeeperutil.h"

#include <functional>
#include <google/protobuf/descriptor.h>

// 框架提供给外部使用的 可以发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    ServiceInfo service_info;

    // 获取了服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    // 获取服务的名字
    std::string service_name = pserviceDesc->name();
    // 获取服务对象service方法的数量
    int methodCnt = pserviceDesc->method_count();

    for (int i = 0; i < methodCnt; ++i)
    {
        // 获取了服务对象指定下标的服务方法的描述
        const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap_.insert(std::make_pair(method_name, pmethodDesc));
    }
    service_info.m_service_ = service;
    m_serviceMap_.insert({service_name, service_info});
}

// 启动rpc服务节点，开始提供rpc远程网络调用服务
void RpcProvider::run()
{
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    InetAddress address(port, ip);

    // 创建TcpServer对象
    TcpServer server(&m_eventLoop_, address, "RpcProvider");

    //绑定连接回调和消息读写回调方法    
    server.setConnectionCallback(std::bind(
        &RpcProvider::OnConnection, this, std::placeholders::_1
    ));

    //绑定消息读写回调
    server.setMessageCallback(std::bind(
        &RpcProvider::OnMessage, this, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3
    ));

    // 设置muduo库的线程数量
    server.setThreadNum(4);

    // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    ZkClient zkCli;
    zkCli.Start();
    // service_name为永久性节点，method_name为临时性节点
    for (auto &sp : m_serviceMap_)
    {
        // /service_name
        std::string serveice_path = "/" + sp.first;
        zkCli.Create(serveice_path.c_str(), nullptr, 0, 0);
        for (auto &mp : sp.second.m_methodMap_)
        {
            // /service_name/method_name 
            std::string method_path = serveice_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }
    LOG_INFO("RpcProvider start service at ip: %s, port: %d", ip.c_str(), port);
    //启动网络服务
    server.start();
    m_eventLoop_.loop();
}

// 新的socket连接回调
void RpcProvider::OnConnection(const TcpConnectionPtr& conn)
{
    if (!conn->connected())
    {
        conn->shutdown();
    }
}

// 在框架内部，RpcProvider和RpcConsumer协商好通信用的protobuf数据类型

// 字符串buf：header_size(4bytes) + header_str + args_str
// header_str : service_name + method_name + args.size

// 消息读写回调
void RpcProvider::OnMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp stamp)
{
    // 网络上接受的远程rpc请求调用的字符流
    std::string recv_buf = buf->retrieveAllAsString();

    // 从字符流中读取前4个字节的内容表示header_size
    uint32_t header_size = 0;
    recv_buf.copy((char*)&header_size, 4, 0);

    // 根据header_size读取数据头的原始字符流，反序列化数据得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4, header_size);

    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;

    if (rpcHeader.ParseFromString(rpc_header_str))
    {
        // 数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }
    else
    {
        // 数据头反序列化失败
        LOG_ERROR("rpc_header_str: %s parse error!", rpc_header_str.c_str());
        //std::cout << "rpc_header_str: " << rpc_header_str << "parse error!" << std::endl;
        return;
    }

    // 获取rpc方法参数的字符流数据
    std::string args_str = recv_buf.substr(4 + header_size, args_size);
    
    // 打印调试信息
    std::cout << "==============================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_size: " << args_size << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "==============================" << std::endl;

    // 获取service对象和method对象
    auto it = m_serviceMap_.find(service_name);
    if (it == m_serviceMap_.end())
    {
        LOG_ERROR("%s is not exit!", service_name.c_str());
        return;
    }
    
    // 获取service对象
    google::protobuf::Service* service = it->second.m_service_;

    auto mit = it->second.m_methodMap_.find(method_name);
    if (mit == it->second.m_methodMap_.end())
    {
        LOG_ERROR("%s's %s is not exit!", service_name.c_str(), method_name.c_str());
        //std::cout << service_name << " 's " << method_name << " is not exit!" << std::endl;
        return;
    }
    //获取method对象
    const google::protobuf::MethodDescriptor* method = mit->second;

    // 生成rpc方法调用的请求request和response参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str))
    {
        LOG_ERROR("request parse error, content : %s", args_str.c_str());
        //std::cout << "request parse error, content : " << args_str << std::endl;
        return;
    }
    
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();
    
    // 给下面method方法的调用绑定一个Closure回调函数
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider, 
                                                                    const TcpConnectionPtr&, 
                                                                    google::protobuf::Message*>
    (this, &RpcProvider::SendRpcResponse, conn, response);

    // 在框架上根据远端rpc请求，调用当前rpc节点发布的方法
    service->CallMethod(method, nullptr, request, response, done);
}

// Closure的回调操作，用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const TcpConnectionPtr& conn, google::protobuf::Message* response)
{
    std::string response_str;
    // response 进行序列化
    if (response->SerializeToString(&response_str)) 
    {
        // 序列化成功后，通过网络把rpc方法执行的结果发送回rpc的调用方
        conn->send(response_str);
    }
    else
    {
        std::cout << "Serialize response_str error!" << std::endl;
    }
    // 模拟http的短链接服务，由rpcprovider主动断开连接
    conn->shutdown();
}