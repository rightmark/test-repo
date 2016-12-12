#pragma once

#include "ui_bvpn.h"

class CBlinkVPN : public QWidget
{
    Q_OBJECT

public:
    CBlinkVPN(QWidget* parent = Q_NULLPTR);

    // CBlinkVPN class has Presenter role
public Q_SLOTS:
    void connectRequest(bool);
    void loginRequest(bool);

Q_SIGNALS:
    void connectReply(bool);
    void loginReply(const QString&);

private:
    Ui::BlinkVPNClass ui;
};
