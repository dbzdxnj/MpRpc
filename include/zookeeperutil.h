#pragma once

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <string>

//封装zookeeper客户端类
class ZkClient
{
public:
    ZkClient();
    ~ZkClient();
    // zkclient启动连接zkserver
    void Start();
    // 在zkserver上根据指定的path创建znode节点
    void Create(const char *path, const char *data, int datalen, int state = 0);
    // 根据指定路径节点获得znode的值    
    std::string GetDdate(const char *path);
private:
    // zk的客户端句柄
    zhandle_t *m_zhandle_;
};