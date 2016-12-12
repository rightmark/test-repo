#include "stdafx.h"
#include "connectbutton.h"


CConnectButton::CConnectButton(QWidget* parent)
    : QLabel(parent)
    , m_bConnect(false)
    , m_bHovered(false)
    , m_bPressed(false)
    , m_clrConnect(233, 242, 250)
    , m_clrDisconnect(247, 247, 247)
{
}

// slots
void CConnectButton::connected(bool b)
{
    qDebug(">> connect=%i", b);
    m_bConnect = b;

    OnConnect();
}

// overridables
void CConnectButton::enterEvent(QEvent* e)
{
    qDebug(">> CConnectButton.enterEvent");
    OnEnter();

    QLabel::enterEvent(e);
}

void CConnectButton::leaveEvent(QEvent* e)
{
    qDebug(">> CConnectButton.leaveEvent");
    OnLeave();

    QLabel::leaveEvent(e);
}
void CConnectButton::mouseMoveEvent(QMouseEvent* e)
{
    m_bPressed = false;

    QLabel::mouseMoveEvent(e);
}
void CConnectButton::mousePressEvent(QMouseEvent* e)
{
    qDebug(">> CConnectButton.mousePressEvent");
    m_bPressed = true;

    QLabel::mousePressEvent(e);
}
void CConnectButton::mouseReleaseEvent(QMouseEvent* e)
{
    qDebug(">> CConnectButton.mouseReleaseEvent");
    if (m_bPressed)
    {
        qDebug(">> signal CConnectButton.click()");
        Q_EMIT click(m_bConnect);

        m_bPressed = false;
    }
    QLabel::mouseReleaseEvent(e);
}


// helper methods
void CConnectButton::OnConnect() Q_DECL_NOEXCEPT
{
    // @TODO: km 20161211 - change button image and text..
/*
    "<html><head/><body><p><span style=\"color:#e9f2fa;\">QUICK CONNECT</span></p></body></html>"
*/
    (m_bHovered) ? OnEnter() : OnLeave(); // update button image
}
void CConnectButton::OnEnter() Q_DECL_NOEXCEPT
{
    QLabel* label = parent()->findChild<QLabel*>("connectImage");

    if (m_bConnect)
    {
        label->setPixmap(QPixmap(QString::fromUtf8(":/CBlinkVPN/Resources/Assets/Disconnect_btn_hover.png")));
    }
    else
    {
        label->setPixmap(QPixmap(QString::fromUtf8(":/CBlinkVPN/Resources/Assets/Quickconnect_btn_hover.png")));
    }

    m_bHovered = true;
}
void CConnectButton::OnLeave() Q_DECL_NOEXCEPT
{
    QLabel* label = parent()->findChild<QLabel*>("connectImage");

    if (m_bConnect)
    {
        label->setPixmap(QPixmap(QString::fromUtf8(":/CBlinkVPN/Resources/Assets/Disconnect_btn.png")));
    }
    else
    {
        label->setPixmap(QPixmap(QString::fromUtf8(":/CBlinkVPN/Resources/Assets/Quickconnect_btn.png")));
    }

    m_bHovered = false;
}
