/********************************************************************************
** Form generated from reading UI file 'bvpn.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BVPN_H
#define UI_BVPN_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>
#include "connectbutton.h"
#include "loginbutton.h"

QT_BEGIN_NAMESPACE

class Ui_BlinkVPNClass
{
public:
    QLabel *loginLabel;
    CLoginButton *loginButton;
    QLabel *logoImage;
    QLabel *connectFlag;
    QLabel *connectImage;
    CConnectButton *connectButton;

    void setupUi(QWidget *BlinkVPNClass)
    {
        if (BlinkVPNClass->objectName().isEmpty())
            BlinkVPNClass->setObjectName(QStringLiteral("BlinkVPNClass"));
        BlinkVPNClass->setWindowModality(Qt::NonModal);
        BlinkVPNClass->resize(296, 400);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(BlinkVPNClass->sizePolicy().hasHeightForWidth());
        BlinkVPNClass->setSizePolicy(sizePolicy);
        BlinkVPNClass->setMinimumSize(QSize(296, 400));
        BlinkVPNClass->setMaximumSize(QSize(296, 400));
        QIcon icon;
        icon.addFile(QStringLiteral("Resources/qticon.ico"), QSize(), QIcon::Normal, QIcon::Off);
        BlinkVPNClass->setWindowIcon(icon);
        BlinkVPNClass->setStyleSheet(QLatin1String("CBlinkVPN{ background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 rgb(56, 66, 90), stop:1 rgb(58, 68, 92)); }\n"
""));
        BlinkVPNClass->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
        loginLabel = new QLabel(BlinkVPNClass);
        loginLabel->setObjectName(QStringLiteral("loginLabel"));
        loginLabel->setGeometry(QRect(19, 16, 221, 20));
        sizePolicy.setHeightForWidth(loginLabel->sizePolicy().hasHeightForWidth());
        loginLabel->setSizePolicy(sizePolicy);
        QFont font;
        font.setFamily(QStringLiteral("Roboto"));
        font.setPointSize(9);
        font.setBold(true);
        font.setWeight(75);
        loginLabel->setFont(font);
        loginLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        loginLabel->setTextInteractionFlags(Qt::NoTextInteraction);
        loginButton = new CLoginButton(BlinkVPNClass);
        loginButton->setObjectName(QStringLiteral("loginButton"));
        loginButton->setGeometry(QRect(256, 16, 22, 20));
        loginButton->setCursor(QCursor(Qt::PointingHandCursor));
        loginButton->setMouseTracking(true);
        loginButton->setFocusPolicy(Qt::TabFocus);
        loginButton->setPixmap(QPixmap(QString::fromUtf8(":/CBlinkVPN/Resources/Assets/Login_normal.png")));
        loginButton->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        loginButton->setMargin(0);
        logoImage = new QLabel(BlinkVPNClass);
        logoImage->setObjectName(QStringLiteral("logoImage"));
        logoImage->setGeometry(QRect(85, 123, 126, 114));
        sizePolicy.setHeightForWidth(logoImage->sizePolicy().hasHeightForWidth());
        logoImage->setSizePolicy(sizePolicy);
        logoImage->setPixmap(QPixmap(QString::fromUtf8(":/CBlinkVPN/Resources/Assets/Logo.png")));
        logoImage->setAlignment(Qt::AlignCenter);
        logoImage->setTextInteractionFlags(Qt::NoTextInteraction);
        connectFlag = new QLabel(BlinkVPNClass);
        connectFlag->setObjectName(QStringLiteral("connectFlag"));
        connectFlag->setGeometry(QRect(51, 321, 29, 29));
        sizePolicy.setHeightForWidth(connectFlag->sizePolicy().hasHeightForWidth());
        connectFlag->setSizePolicy(sizePolicy);
        connectFlag->setPixmap(QPixmap(QString::fromUtf8(":/CBlinkVPN/Resources/Assets/SWE_flag.png")));
        connectFlag->setAlignment(Qt::AlignCenter);
        connectFlag->setTextInteractionFlags(Qt::NoTextInteraction);
        connectImage = new QLabel(BlinkVPNClass);
        connectImage->setObjectName(QStringLiteral("connectImage"));
        connectImage->setGeometry(QRect(46, 316, 203, 39));
        sizePolicy.setHeightForWidth(connectImage->sizePolicy().hasHeightForWidth());
        connectImage->setSizePolicy(sizePolicy);
        connectImage->setPixmap(QPixmap(QString::fromUtf8(":/CBlinkVPN/Resources/Assets/Quickconnect_btn.png")));
        connectImage->setAlignment(Qt::AlignCenter);
        connectImage->setTextInteractionFlags(Qt::NoTextInteraction);
        connectButton = new CConnectButton(BlinkVPNClass);
        connectButton->setObjectName(QStringLiteral("connectButton"));
        connectButton->setGeometry(QRect(50, 316, 195, 39));
        sizePolicy.setHeightForWidth(connectButton->sizePolicy().hasHeightForWidth());
        connectButton->setSizePolicy(sizePolicy);
        QFont font1;
        font1.setFamily(QStringLiteral("Roboto"));
        font1.setPointSize(11);
        font1.setBold(true);
        font1.setWeight(75);
        connectButton->setFont(font1);
        connectButton->setCursor(QCursor(Qt::PointingHandCursor));
        connectButton->setFocusPolicy(Qt::TabFocus);
        connectButton->setAlignment(Qt::AlignCenter);
        connectButton->setTextInteractionFlags(Qt::NoTextInteraction);
        QWidget::setTabOrder(loginButton, connectImage);

        retranslateUi(BlinkVPNClass);

        QMetaObject::connectSlotsByName(BlinkVPNClass);
    } // setupUi

    void retranslateUi(QWidget *BlinkVPNClass)
    {
        BlinkVPNClass->setWindowTitle(QApplication::translate("BlinkVPNClass", "BlinkVPN", 0));
        loginLabel->setText(QApplication::translate("BlinkVPNClass", "<html><head/><body><p><span style=\" color:#667391;\">Login</span></p></body></html>", 0));
        loginButton->setText(QString());
        logoImage->setText(QString());
        connectFlag->setText(QString());
        connectImage->setText(QString());
        connectButton->setText(QApplication::translate("BlinkVPNClass", "<html><head/><body><p><span style=\" color:#e9f2fa;\">QUICK CONNECT</span></p></body></html>", 0));
    } // retranslateUi

};

namespace Ui {
    class BlinkVPNClass: public Ui_BlinkVPNClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BVPN_H
