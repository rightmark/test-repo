#include "stdafx.h"
#include "connectbutton.h"


CConnectButton::CConnectButton(QWidget* parent)
    : QLabel(parent)
    , m_bConnect(false)
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
    if (e->button() == Qt::LeftButton) { m_bPressed = true; }

    QLabel::mousePressEvent(e);
}
void CConnectButton::mouseReleaseEvent(QMouseEvent* e)
{
    qDebug(">> CConnectButton.mouseReleaseEvent");
    if (m_bPressed && (e->button() == Qt::LeftButton))
    {
        qDebug(">> signal CConnectButton.click()");
        Q_EMIT click(m_bConnect);

#ifdef _TESTING_
        m_bConnect = !m_bConnect; OnConnect();
#endif

        m_bPressed = false;
    }
    QLabel::mouseReleaseEvent(e);
}


// helper methods
void CConnectButton::OnConnect() Q_DECL_NOEXCEPT
{
    m_bConnect ? UpdateLabelText(m_clrDisconnect.name(), tr("DISCONNECT"))
               : UpdateLabelText(m_clrConnect.name(), tr("QUICK CONNECT"));

    OnEnter(); // update hovered button image
}
void CConnectButton::OnEnter() Q_DECL_NOEXCEPT
{
    QLabel* label = parent()->findChild<QLabel*>(QString("connectImage"));

    if (m_bConnect)
    {
        label->setPixmap(QPixmap(QString(":/CBlinkVPN/Resources/Assets/Disconnect_btn_hover.png")));
    }
    else
    {
        label->setPixmap(QPixmap(QString(":/CBlinkVPN/Resources/Assets/Quickconnect_btn_hover.png")));
    }
}
void CConnectButton::OnLeave() Q_DECL_NOEXCEPT
{
    QLabel* label = parent()->findChild<QLabel*>(QString("connectImage"));

    if (m_bConnect)
    {
        label->setPixmap(QPixmap(QString(":/CBlinkVPN/Resources/Assets/Disconnect_btn.png")));
    }
    else
    {
        label->setPixmap(QPixmap(QString(":/CBlinkVPN/Resources/Assets/Quickconnect_btn.png")));
    }
}

void CConnectButton::UpdateLabelText(const QString& c, const QString& t) Q_DECL_NOEXCEPT
{
    setText(QString("<html><head/><body><p><span style=\"color:%1;\">%2</span></p></body></html>").arg(c, t));
}
