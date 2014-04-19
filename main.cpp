#include "dialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Dialog *w;
    QStringList argument = QApplication::arguments();
    if(argument.count()>1)
    {
        w = new Dialog(argument.at(1));
    }
    else
    {
        w = new Dialog();
    }

    w->show();

    return a.exec();
}
