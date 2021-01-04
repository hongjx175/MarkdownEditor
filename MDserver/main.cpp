#include "mdserver.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MDserver w;
    w.show();
    return a.exec();
}
