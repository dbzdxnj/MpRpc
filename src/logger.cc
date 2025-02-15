#include "logger.h"

#include <iostream>
#include <time.h>

Logger::Logger()
{
    std::thread weriteLogTask([&](){
        while (true)
        {
            // 获取当前的日期，然后取队列的日志信息， 写入相应的日志文件中
            time_t now = time(nullptr);
            tm *nowtm = localtime(&now);

            char file_name[128];
            sprintf(file_name, "%d-%d-%d-log.txt", nowtm->tm_year + 1900, 
                    nowtm->tm_mon + 1, nowtm->tm_mday);
            
            FILE *pf = fopen(file_name, "a+");
            if (pf == nullptr)
            {
                std::cout << "logger file : " << file_name << "open error!" << std::endl;
                exit(EXIT_FAILURE);
            }
            std::string msg = m_lckQue_.pop();

            char time_buf[128] = {0};
            sprintf(time_buf, "%d:%d:%d => [%s] ", 
                    nowtm->tm_hour, 
                    nowtm->tm_min, 
                    nowtm->tm_sec, 
                    (m_loglevel_ == INFO ? "INFO" : "ERROR"));
            msg.insert(0, time_buf);
            msg.append("\n");

            fputs(msg.c_str(), pf);
            fclose(pf);
        }
    });
    // 设置分离线程，守护线程
    weriteLogTask.detach();
}
// 获取日志的单例
Logger& Logger::GetInstance()
{
    static Logger logger;
    return logger;
}
// 设置日志级别
void Logger::SetLogLevel(LogLevel level)
{
    m_loglevel_ = level;
}
// 写日志, 把日志信息写入lockQueue中
void Logger::Log(std::string msg)
{
    m_lckQue_.push(msg);
}