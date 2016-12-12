#include "stdafx.h"
#include "bvpn.h"


CBlinkVPN::CBlinkVPN(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
}

bool CBlinkVPN::GetUserName(QString& s) Q_DECL_NOEXCEPT
{
    // @KLUDGE: should invoke Login Dialog to obtain credentials..
    s = "User McName";
    return true;
}

void CBlinkVPN::connectRequest(bool bConnect)
{
    if (m_vpn.Connect(bConnect))
    {
        Q_EMIT connectReply(!bConnect); // signal on successful connect/disconnect..
    }
}

void CBlinkVPN::loginRequest(bool bLogin)
{
    QString s;
    bool bRet = false;
    if (bLogin) { bRet = GetUserName(s); }

    if (bRet && m_vpn.Login(s))
    {
        Q_EMIT loginReply(s);
    }
}
