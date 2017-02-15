#include "stdafx.h"

#include <QtWidgets/QApplication>
#include "bvpn.h"


int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);

    try
    {
        CBlinkVPN w;
        w.show();
        return a.exec();
    }
    catch (const char* s)
    {
        QMessageBox::critical(0, "Error", QString(s));
        return -1;
    }

}
