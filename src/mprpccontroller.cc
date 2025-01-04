#include "mprpccontroller.h"

MprpcController::MprpcController()
{
    m_failed_ = false;
    m_errText_ = "";
}

void MprpcController::Reset()
{
    m_failed_ = false;
    m_errText_ = "";
}

bool MprpcController::Failed() const
{
    return m_failed_;
}

std::string MprpcController::ErrorText() const
{
    return m_errText_;
}
void MprpcController::SetFailed(const std::string& reason)
{
    m_failed_ = true;
    m_errText_ = reason;
}

// 目前未实现具体功能
    void MprpcController::StartCancel(){}
    bool MprpcController::IsCanceled() const{ return true; }
    void MprpcController::NotifyOnCancel(google::protobuf::Closure* callback){}