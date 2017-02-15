#pragma once

#include "ui_bvpn.h"


class CBlinkVPN : public QWidget
{
    Q_OBJECT

public:
    CBlinkVPN(QWidget* parent = Q_NULLPTR);

protected:
    // helper methods
    bool setupFonts(void) Q_DECL_NOEXCEPT;
    long getEmbeddedFontId(const QString&) Q_DECL_NOEXCEPT;
    bool setupEmbeddedFont(const QString&, int, int, bool = false) Q_DECL_NOEXCEPT;

private:
    Ui::BlinkVPNClass ui;
};
