#pragma once

#include "ui_bvpn.h"
#include "BlinkVPNProvider.h"


class CBlinkVPN : public QWidget
{
    Q_OBJECT

public:
    CBlinkVPN(QWidget* parent = Q_NULLPTR);

    bool GetUserName(QString& s) Q_DECL_NOEXCEPT;

    // CBlinkVPN class has Presenter role
public Q_SLOTS:
    void connectRequest(bool);
    void loginRequest(bool);

Q_SIGNALS:
    void connectReply(bool);
    void loginReply(QString);

private:
    Ui::BlinkVPNClass ui;

    CBlinkVPNProvider m_vpn;
};
