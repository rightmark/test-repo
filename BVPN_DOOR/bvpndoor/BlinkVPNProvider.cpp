#include "stdafx.h"
#include "BlinkVPNProvider.h"


CBlinkVPNProvider::CBlinkVPNProvider(QWidget* parent)
    : QWidget(parent)
    , m_bConnect(false)
    , m_bLogin(false)
{
}


// dummy methods
bool CBlinkVPNProvider::Connect(bool b) Q_DECL_NOEXCEPT
{
    if (m_bConnect != b)
    {
        m_bConnect = b;
        return true;
    }
    return false;
}

bool CBlinkVPNProvider::Login(const QString& s) Q_DECL_NOEXCEPT
{
    // @KLUDGE: the algorithm does not depend on VPN connection state. IRL it should.
    // anyways this is implementation dependent..
    bool b = !s.isEmpty();
    if (m_bLogin != b)
    {
        m_bLogin = b;
        return true;
    }
    return false;
}

void CBlinkVPNProvider::connectRequest(bool bConnect)
{
    if (Connect(bConnect))
    {
        qDebug(">> signal CBlinkVPNProvider.connectReply()");
        Q_EMIT connectReply(bConnect); // signal on successful connect/disconnect..
    }
}

void CBlinkVPNProvider::loginRequest(bool bLogin)
{
    QString s;
    bool bRet = true;
    if (bLogin) { bRet = GetUserName(s); }

    if (bRet && Login(s))
    {
        qDebug(">> signal CBlinkVPNProvider.loginReply()");
        Q_EMIT loginReply(s);
    }
}
