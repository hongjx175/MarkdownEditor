#include "mdeditor.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MDeditor w;
    w.show();
    return a.exec();
}
