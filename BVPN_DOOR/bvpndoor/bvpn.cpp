#include "stdafx.h"
#include "bvpn.h"

CBlinkVPN::CBlinkVPN(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
}

void CBlinkVPN::connectRequest(bool bConnect)
{
    // @TODO: km 20161211 - write VPN Provider request..

    Q_EMIT connectReply(!bConnect); // @KLUDGE: km 20161211 - write code..
}

void CBlinkVPN::loginRequest(bool bLogin)
{
    // @TODO: km 20161211 - invoke Login Dialog to obtain credentials..
    QString s;
    if (bLogin) { s = "User McName"; }

    Q_EMIT loginReply(s);
}
