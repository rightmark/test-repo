#pragma once

#include <QtWidgets/QWidget>
#include "ui_bvpn.h"

class CBlinkVPN : public QWidget
{
    Q_OBJECT

public:
    CBlinkVPN(QWidget *parent = Q_NULLPTR);

private:
    Ui::BlinkVPNClass ui;
};
