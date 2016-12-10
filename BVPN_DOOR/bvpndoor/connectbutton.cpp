#include "stdafx.h"
#include "connectbutton.h"


CConnectButton::CConnectButton(QWidget* parent)
    : QLabel(parent)
    , m_bConnect(false)
    , m_bPressed(false)
{
}

// slots
void CConnectButton::connected(bool b)
{
    qDebug("connect=%i", b);
    m_bConnect = b;

    OnConnect();
}

// overridables
void CConnectButton::enterEvent(QEvent* e)
{
    qDebug(">> enterEvent");
    OnEnter();

    QLabel::enterEvent(e);
}

void CConnectButton::leaveEvent(QEvent* e)
{
    qDebug(">> leaveEvent");
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
    qDebug(">> mousePressEvent");
    m_bPressed = true;

    QLabel::mousePressEvent(e);
}
void CConnectButton::mouseReleaseEvent(QMouseEvent* e)
{
    qDebug(">> mouseReleaseEvent");
    if (m_bPressed)
    {
        qDebug(">> signal CConnectButton.click()");
        Q_EMIT click();
        m_bPressed = false;
    }
    QLabel::mouseReleaseEvent(e);
}


// helper methods
void CConnectButton::OnConnect() Q_DECL_NOEXCEPT
{
    // @TODO: km 20161210 - change button image..
    ;
}
void CConnectButton::OnEnter() Q_DECL_NOEXCEPT
{
    QLabel* label = parent()->findChild<QLabel*>("connectImage"); // QStringLiteral("connectImage")
    label->setPixmap(QPixmap(QString::fromUtf8(":/CBlinkVPN/Resources/Assets/Quickconnect_btn_hover.png")));

    // @TODO: km 20161210 - change button image..

}
void CConnectButton::OnLeave() Q_DECL_NOEXCEPT
{
    QLabel* label = parent()->findChild<QLabel*>("connectImage"); // QStringLiteral("connectImage")
    label->setPixmap(QPixmap(QString::fromUtf8(":/CBlinkVPN/Resources/Assets/Quickconnect_btn.png")));

    // @TODO: km 20161210 - change button image..

}
