#pragma once

//////////////////////////////////////////////////////////////////////////
// dummy VPN Provider

class CBlinkVPNProvider
{
public:
    CBlinkVPNProvider()
        : m_bConnect(false)
        , m_bLogin(false)
    {
    }

    ~CBlinkVPNProvider() Q_DECL_NOEXCEPT {}

    // dummy methods
    bool Connect(bool b) Q_DECL_NOEXCEPT
    {
        if (m_bConnect != b)
        {
            m_bConnect = b;
            return true;
        }
        return false;
    }

    bool Login(const QString& s) Q_DECL_NOEXCEPT
    {
        // @KLUDGE: the algorithm does not depend on VPN connection state. IRL it should.
        // anyways this is implementation dependent..
        bool b = s.isEmpty();
        if (m_bLogin != b)
        {
            m_bLogin = b;
            return true;
        }
        return false;
    }


private:
    bool m_bConnect;
    bool m_bLogin;
};

