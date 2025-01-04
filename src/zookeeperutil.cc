#include "zookeeperutil.h"
#include "mprpcapplication.h"

#include <iostream>

ZkClient::ZkClient() : m_zhandle_(nullptr)
{}

ZkClient::~ZkClient()
{
    if (m_zhandle_ != nullptr)
    {
        zookeeper_close(m_zhandle_);
    }
}

// 全局的watcher观察器  zkserver给zeclient的通知
void global_watcher(zhandle_t *zh, int type, int state, const char *path, void  *wathcerCtx)
{
    if (type == ZOO_SESSION_EVENT)  // 回调的消息类型是会话session相关
    {
        if (state == ZOO_CONNECTED_STATE)   // zkclient和zkserver连接成功
        {
            sem_t *sem = (sem_t *)zoo_get_context(zh);
            sem_post(sem);
        }
    }
}

// zkclient启动连接zkserver
void ZkClient::Start()
{
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;
    //std::cout << "Host: [" << host << "], Port: [" << port << "]" << std::endl;

    /*
    zookeeper_mt : 多线程版本，有3个线程
    1. zookeeper的API客户端，即现在用的这个
    2. 网络I/O线程
    3，watcher回调线程
    */
    m_zhandle_ = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if (nullptr == m_zhandle_)
    {
        std::cout << "zookeeper_init_error!" << std::endl;
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle_, &sem);
    
    sem_wait(&sem);
    std::cout << "zookeeper_init success!" << std::endl;
}
// 在zkserver上根据指定的path创建znode节点
void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    // 先判断path表示的znode节点是否存在
    int flag = zoo_exists(m_zhandle_, path, 0, nullptr);
    if (ZNONODE == flag)    // 表示path的znode节点不存在
    {
        // 创建指定的path的znode节点
        flag = zoo_create(m_zhandle_, path, data, datalen, 
            &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        if (flag == ZOK)
        {
            std::cout << "znode create success... path : " << path << std::endl;
        }
        else
        {
            std::cout << "flag : " << flag << std::endl;
            std::cout << "znode create error ... path: " << path << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    
}
// 根据指定路径节点获得znode的值    
std::string ZkClient::GetDdate(const char *path)
{
    char buffer[1024] = {0};
    int bufferlen = sizeof(buffer);
    int flag = zoo_get(m_zhandle_, path, 0, buffer, &bufferlen, nullptr);
    if (flag != ZOK)
    {
        std::cout << "get znode error... path: " << path << std::endl;
        return "";
    }
    else
    {
        return buffer;
    }
}