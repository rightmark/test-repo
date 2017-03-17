#include "stdafx.h"
#include "loginbutton.h"

CLoginButton::CLoginButton(QWidget* parent)
    : QLabel(parent)
    , m_clrNorm("#667391")
    , m_clrHovr("#e9f2fa")
    , m_clrUser("#ffb823")
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

void CLoginButton::mouseReleaseEvent(QMouseEvent* e)
{
    qDebug(">> CLoginButton.mouseReleaseEvent");
    if ((e->button() == Qt::LeftButton) && rect().contains(e->pos()))
    {
        qDebug(">> signal CLoginButton.click()");
        Q_EMIT click(m_strUserName.isEmpty()); 

#ifdef _TESTING_
        m_strUserName.isEmpty() ? m_strUserName = "User McName" : m_strUserName.clear(); OnEnter();
#endif
    }
    QLabel::mouseReleaseEvent(e);
}


// helper methods
void CLoginButton::OnEnter() Q_DECL_NOEXCEPT
{
    if (m_strUserName.isEmpty())
    {
        updateLabelImage("Login_hover.png");
        updateLoginText(m_clrHovr.name());
    }
    else
    {
        updateLabelImage("Logout_hover.png");
        updateLogoutText(m_clrHovr.name());
    }
}
void CLoginButton::OnLeave() Q_DECL_NOEXCEPT
{
    if (m_strUserName.isEmpty())
    {
        updateLabelImage("Login_normal.png");
        updateLoginText(m_clrNorm.name());
    }
    else
    {
        updateLabelImage("Logout_normal.png");
        updateLogoutText(m_clrNorm.name(), m_clrUser.name());
    }
}

void CLoginButton::updateLabelImage(const QString& s) Q_DECL_NOEXCEPT
{
    setPixmap(QPixmap(":/CBlinkVPN/Resources/Assets/" + s));
}

void CLoginButton::updateLoginText(const QString& c) Q_DECL_NOEXCEPT
{
    QLabel* l = static_cast<QLabel*>(buddy());
    Q_ASSERT(l != Q_NULLPTR);

    QString s("<html><head/><body><p><span style=\"color:%1;\">%2</span></p></body></html>");
    l->setText(s.arg(c, tr("Login")));
}

void CLoginButton::updateLogoutText(const QString& c) Q_DECL_NOEXCEPT
{
    QLabel* l = static_cast<QLabel*>(buddy());
    Q_ASSERT(l != Q_NULLPTR);

    QString s("<html><head/><body><p><span style=\"color:%1;\">%3 %2</span></p></body></html>");
    l->setText(s.arg(c, m_strUserName, tr("Logged as")));
}

void CLoginButton::updateLogoutText(const QString& c, const QString& u) Q_DECL_NOEXCEPT
{
    QLabel* l = static_cast<QLabel*>(buddy());
    Q_ASSERT(l != Q_NULLPTR);

    QString s("<html><head/><body><p><span style=\"color:%1;\">%4 </span><span style=\"color:%2;\">%3</span></p></body></html>");
    l->setText(s.arg(c, u, m_strUserName, tr("Logged as")));
}
