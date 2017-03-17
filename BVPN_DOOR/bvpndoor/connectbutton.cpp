#include "stdafx.h"
#include "connectbutton.h"


CConnectButton::CConnectButton(QWidget* parent)
    : QLabel(parent)
    , m_bConnect(false)
    , m_clrConnect("#e9f2fa")
    , m_clrDisconnect("#f7f7f7")
{
}

// slots
void CConnectButton::connected(bool b)
{
    qDebug() << ">> slot connect:" << b;
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
void CConnectButton::mouseReleaseEvent(QMouseEvent* e)
{
    qDebug(">> CConnectButton.mouseReleaseEvent");
    if ((e->button() == Qt::LeftButton) && rect().contains(e->pos()) )
    {
        qDebug(">> signal CConnectButton.click()");
        Q_EMIT click(!m_bConnect);

#ifdef _TESTING_
        m_bConnect = !m_bConnect; OnConnect();
#endif
    }
    QLabel::mouseReleaseEvent(e);
}


// helper methods
void CConnectButton::OnConnect() Q_DECL_NOEXCEPT
{
    m_bConnect ? updateLabelText(m_clrDisconnect.name(), tr("DISCONNECT"))
               : updateLabelText(m_clrConnect.name(), tr("QUICK CONNECT"));

    OnEnter(); // update hovered button image
}

void CConnectButton::OnEnter() Q_DECL_NOEXCEPT
{
    m_bConnect ? updateLabelImage("Disconnect_btn_hover.png")
               : updateLabelImage("Quickconnect_btn_hover.png");
}

void CConnectButton::OnLeave() Q_DECL_NOEXCEPT
{
    m_bConnect ? updateLabelImage("Disconnect_btn.png")
               : updateLabelImage("Quickconnect_btn.png");
}

void CConnectButton::updateLabelImage(const QString& s) Q_DECL_NOEXCEPT
{
    QLabel* l = static_cast<QLabel*>(buddy());
    Q_ASSERT(l != Q_NULLPTR);

    l->setPixmap(QPixmap(":/CBlinkVPN/Resources/Assets/" + s));
}

void CConnectButton::updateLabelText(const QString& c, const QString& t) Q_DECL_NOEXCEPT
{
    setText(QString("<html><head/><body><p><span style=\"color:%1;\">%2</span></p></body></html>").arg(c, t));
}
