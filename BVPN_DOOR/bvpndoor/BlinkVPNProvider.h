#pragma once

//////////////////////////////////////////////////////////////////////////
// dummy VPN Provider class is Presenter in MVP model

class CBlinkVPNProvider : public QWidget
{
    Q_OBJECT

public:
    CBlinkVPNProvider(QWidget* parent = Q_NULLPTR);
    ~CBlinkVPNProvider() Q_DECL_NOEXCEPT {}

public Q_SLOTS:
    void connectRequest(bool);
    void loginRequest(bool);

Q_SIGNALS:
    void connectReply(bool);
    void loginReply(QString);

public:
    // dummy methods
    bool Connect(bool b) Q_DECL_NOEXCEPT;
    bool Login(const QString& s) Q_DECL_NOEXCEPT;
    bool GetUserName(QString& s) Q_DECL_NOEXCEPT
    {
        // @KLUDGE: should invoke Login Form to obtain the credentials..
        s = "User McName";
        return true;
    }

private:
    bool m_bConnect;    // VPN connection is active
    bool m_bLogin;      // user is logged in
};

