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
void CLoginButton::identified(QString s)
{
    qDebug() << ">> slot identified:" << s;
    m_strUserName = s;

    OnEnter();
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
    if (e->button() == Qt::LeftButton) { m_bPressed = true; }

    QLabel::mousePressEvent(e);
}
void CLoginButton::mouseReleaseEvent(QMouseEvent* e)
{
    qDebug(">> CLoginButton.mouseReleaseEvent");
    if (m_bPressed && (e->button() == Qt::LeftButton))
    {
        qDebug(">> signal CLoginButton.click()");
        Q_EMIT click(m_strUserName.isEmpty()); 

#ifdef _TESTING_
        m_strUserName.isEmpty() ? m_strUserName = "User McName" : m_strUserName.clear(); OnEnter();
#endif

        m_bPressed = false;
    }
    QLabel::mouseReleaseEvent(e);
}


// helper methods
void CLoginButton::OnEnter() Q_DECL_NOEXCEPT
{
    if (m_strUserName.isEmpty())
    {
        setPixmap(QPixmap(QString(":/CBlinkVPN/Resources/Assets/Login_hover.png")));
        UpdateLoginText(m_clrHovr.name());
    }
    else
    {
        setPixmap(QPixmap(QString(":/CBlinkVPN/Resources/Assets/Logout_hover.png")));
        UpdateLogoutText(m_clrHovr.name());
    }
}
void CLoginButton::OnLeave() Q_DECL_NOEXCEPT
{
    if (m_strUserName.isEmpty())
    {
        setPixmap(QPixmap(QString(":/CBlinkVPN/Resources/Assets/Login_normal.png")));
        UpdateLoginText(m_clrNorm.name());
    }
    else
    {
        setPixmap(QPixmap(QString(":/CBlinkVPN/Resources/Assets/Logout_normal.png")));
        UpdateLogoutText(m_clrNorm.name(), m_clrUser.name());
    }
}

void CLoginButton::UpdateLoginText(const QString& c) Q_DECL_NOEXCEPT
{
    QLabel* label = parent()->findChild<QLabel*>(QString("loginLabel"));
    label->setText(QString("<html><head/><body><p><span style=\"color:%1;\">%2</span></p></body></html>").arg(c, tr("Login")));
}

void CLoginButton::UpdateLogoutText(const QString& c) Q_DECL_NOEXCEPT
{
    QLabel* label = parent()->findChild<QLabel*>(QString("loginLabel"));
    label->setText(QString("<html><head/><body><p><span style=\"color:%1;\">%3 %2</span></p></body></html>").arg(c, m_strUserName, tr("Logged as")));
}

void CLoginButton::UpdateLogoutText(const QString& c, const QString& u) Q_DECL_NOEXCEPT
{
    QLabel* label = parent()->findChild<QLabel*>(QString("loginLabel"));
    label->setText(QString("<html><head/><body><p><span style=\"color:%1;\">%4 </span><span style=\"color:%2;\">%3</span></p></body></html>").arg(c, u, m_strUserName, tr("Logged as")));
}
