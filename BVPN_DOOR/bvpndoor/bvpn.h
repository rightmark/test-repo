#pragma once

#include "ui_bvpn.h"

// CBlinkVPN class also plays the Presenter role in the current MVP scheme implemented..

class CBlinkVPN : public QWidget
{
    Q_OBJECT

public:
    CBlinkVPN(QWidget* parent = Q_NULLPTR);

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
    bool GetUserName(QString& s) Q_DECL_NOEXCEPT;

protected:
    // overridables
    void closeEvent(QCloseEvent*) override;

protected:
    // helper methods
    bool setupFonts(void) Q_DECL_NOEXCEPT;
    long getEmbeddedFontId(const QString&) Q_DECL_NOEXCEPT;
    bool setupEmbeddedFont(const QString&, int, int, int = 100) Q_DECL_NOEXCEPT;

private:
    bool m_bConnect;    // VPN connection is active
    bool m_bLogin;      // user is logged in

    Ui::BlinkVPNClass ui;
};
