#include "stdafx.h"
#include "loginbutton.h"

CLoginButton::CLoginButton(QWidget* parent)
    : QLabel(parent)
    , m_bPressed(false)
    , m_clrNorm(102, 115, 145)
    , m_clrHovr(233, 242, 250)
    , m_clrUser(255, 184,  35)
{
}

// slots
void CLoginButton::identified(const QString& s)
{
    qDebug(">> identified=%s", s);
    m_UserName = s;
}

// overridables
void CLoginButton::enterEvent(QEvent* e)
{
    qDebug(">> CLoginButton.enterEvent");
    OnEnter();

    QLabel::enterEvent(e);
}

void CLoginButton::leaveEvent(QEvent* e)
{
    qDebug(">> CLoginButton.leaveEvent");
    OnLeave();

    QLabel::leaveEvent(e);
}
void CLoginButton::mouseMoveEvent(QMouseEvent* e)
{
    m_bPressed = false;

    QLabel::mouseMoveEvent(e);
}
void CLoginButton::mousePressEvent(QMouseEvent* e)
{
    qDebug(">> CLoginButton.mousePressEvent");
    m_bPressed = true;

    QLabel::mousePressEvent(e);
}
void CLoginButton::mouseReleaseEvent(QMouseEvent* e)
{
    qDebug(">> CLoginButton.mouseReleaseEvent");
    if (m_bPressed)
    {
        qDebug(">> signal CLoginButton.click()");
        Q_EMIT click(!m_UserName.isEmpty()); 

        m_bPressed = false;
    }
    QLabel::mouseReleaseEvent(e);
}


// helper methods
void CLoginButton::OnEnter() Q_DECL_NOEXCEPT
{
    setPixmap(QPixmap(QString::fromUtf8(":/CBlinkVPN/Resources/Assets/Login_hover.png")));
    QLabel* label = parent()->findChild<QLabel*>("loginLabel");

    if (m_UserName.isEmpty())
    {
        label->setText(QString("<html><head/><body><p><span style=\" color:%1;\">Login</span></p></body></html>").arg(m_clrHovr.name()));
    }
    else
    {
        label->setText(QString("<html><head/><body><p><span style=\"color:%1;\">Logged as %2</span></p></body></html>").arg(m_clrHovr.name(), m_UserName));
    }
}
void CLoginButton::OnLeave() Q_DECL_NOEXCEPT
{
    setPixmap(QPixmap(QString::fromUtf8(":/CBlinkVPN/Resources/Assets/Login_normal.png")));
    QLabel* label = parent()->findChild<QLabel*>("loginLabel");

    if (m_UserName.isEmpty())
    {
        label->setText(QString("<html><head/><body><p><span style=\" color:%1;\">Login</span></p></body></html>").arg(m_clrNorm.name()));
    }
    else
    {
        label->setText(QString("<html><head/><body><p><span style=\"color:%1;\">Logged as </span><span style=\"color:%2;\">%3</span></p></body></html>").arg(m_clrNorm.name(), m_clrUser.name(), m_UserName));
    }
}
