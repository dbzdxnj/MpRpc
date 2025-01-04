#include "mprpcapplication.h"
#include "logger.h"

#include <iostream>
#include <unistd.h>
#include <string>

MprpcConfig MprpcApplication::m_config_;

MprpcApplication::MprpcApplication(){}

MprpcApplication& MprpcApplication::GetInstance()
{
    static MprpcApplication app;
    return app;
}

void showArgsHelp()
{
    LOG_ERROR("format: command -i <configfile>");
    //std::cout << "format: command -i <configfile>" << std::endl;
}

void MprpcApplication::Init(int argc, char **argv)
{
    if (argc < 2)
    {
        showArgsHelp(); 
        exit(EXIT_FAILURE);
    }
    int c = 0;
    std::string config_file;
    while ((c = getopt(argc, argv, "i:")) != -1)
    {
        switch (c)
        {
        case 'i':
            config_file = optarg;
            break;
        case '?':
            std::cout << "invalid args!" << std::endl;
            showArgsHelp();
            exit(EXIT_FAILURE);
        case ':':
            std::cout << "need <configfile> !" << std::endl;
            showArgsHelp();
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }

    // 开始加载配置文件
    // rpcserver_ip rpcserver_port zookeeper_ip zookeeper_port
    m_config_.LoadConfigFile(config_file.c_str());

    std::cout << "rpc_server_ip: " << m_config_.Load("rpcserverip") << std::endl;
    std::cout << "rpc_server_port: " << m_config_.Load("rpcserverport") << std::endl;
    std::cout << "zookeeper_ip: " << m_config_.Load("zookeeperip") << std::endl;
    std::cout << "zookeeper_ip: " << m_config_.Load("zookeeperport") << std::endl;

}

MprpcConfig& MprpcApplication::GetConfig()
{
    return m_config_;
}