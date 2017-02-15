#include "stdafx.h"
#include "bvpn.h"


CBlinkVPN::CBlinkVPN(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    setupFonts();

}

bool CBlinkVPN::setupFonts(void) Q_DECL_NOEXCEPT
{
    int id(0);

    id = getEmbeddedFontId("Roboto Bold");
    setupEmbeddedFont("loginLabel", id, 9);
    setupEmbeddedFont("connectButton", id, 11);

    return true;
}

long CBlinkVPN::getEmbeddedFontId(const QString& alias) Q_DECL_NOEXCEPT
{
    return QFontDatabase::addApplicationFont(":/CBlinkVPN/" + alias);
}

bool CBlinkVPN::setupEmbeddedFont(const QString& wname, int id, int fsize, bool bold) Q_DECL_NOEXCEPT
{
    QWidget* w = findChild<QWidget*>(wname);
    if (w && (id != -1))
    {
        QFont font;

        font.setFamily(QFontDatabase::applicationFontFamilies(id).at(0));
        font.setPointSize(fsize);
        font.setStyleStrategy(QFont::PreferAntialias);
        font.setBold(bold);

        w->setFont(font);
        return true;
    }
    return false;
}
