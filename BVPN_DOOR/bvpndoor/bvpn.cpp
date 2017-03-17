#include "stdafx.h"
#include "bvpn.h"


CBlinkVPN::CBlinkVPN(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    setupFonts();

}

// dummy methods
bool CBlinkVPN::Connect(bool b) Q_DECL_NOEXCEPT
{
    if (m_bConnect != b)
    {
        m_bConnect = b;
        return true;
    }
    return false;
}

bool CBlinkVPN::Login(const QString& s) Q_DECL_NOEXCEPT
{
    // @KLUDGE: the algorithm does not depend on VPN connection state. IRL it should.
    // anyways this is implementation dependent..
    bool b = !s.isEmpty();
    if (m_bLogin != b)
    {
        m_bLogin = b;
        return true;
    }
    return false;
}

bool CBlinkVPN::GetUserName(QString& s) Q_DECL_NOEXCEPT
{
    // @KLUDGE: should invoke Login Form to obtain the credentials..
    s = "User McName";
    return true;
}

void CBlinkVPN::connectRequest(bool bConnect)
{
    if (Connect(bConnect))
    {
        qDebug(">> signal CBlinkVPN.connectReply()");
        Q_EMIT connectReply(bConnect); // signal on successful connect/disconnect..
    }
}

void CBlinkVPN::loginRequest(bool bLogin)
{
    QString s;
    bool bRet = true;
    if (bLogin) { bRet = GetUserName(s); }

    if (bRet && Login(s))
    {
        qDebug(">> signal CBlinkVPN.loginReply()");
        Q_EMIT loginReply(s);
    }
}

// overridables
void CBlinkVPN::closeEvent(QCloseEvent* e)
{
    // @TODO: close provider here..

    QWidget::closeEvent(e);
}

// helper methods
bool CBlinkVPN::setupFonts(void) Q_DECL_NOEXCEPT
{
    int id = getEmbeddedFontId("Roboto Bold");
    setupEmbeddedFont("loginLabel", id, 9);
    setupEmbeddedFont("connectButton", id, 11);

    return true;
}

long CBlinkVPN::getEmbeddedFontId(const QString& alias) Q_DECL_NOEXCEPT
{
    return QFontDatabase::addApplicationFont(":/CBlinkVPN/" + alias);
}

bool CBlinkVPN::setupEmbeddedFont(const QString& wname, int id, int fsize, int space) Q_DECL_NOEXCEPT
{
    if (id != -1)
    {
        QWidget* w = findChild<QWidget*>(wname);
        Q_ASSERT(w != Q_NULLPTR);

        if (w)
        {
            QFont font;

            font.setFamily(QFontDatabase::applicationFontFamilies(id).at(0));
            font.setPointSize(fsize);
            font.setStyleStrategy(QFont::PreferAntialias);
            font.setHintingPreference(QFont::PreferFullHinting);
            font.setLetterSpacing(QFont::PercentageSpacing, space);

            w->setFont(font);
            return true;
        }
    }
    return false;
}

