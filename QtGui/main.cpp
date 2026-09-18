#include "QtGui.h"

#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");
    QApplication a(argc, argv);

    QtGui w;
    w.setWindowTitle("Feira das Profissões - Controle Arduino");
    w.setWindowIcon(QIcon(":/Assets/assets/x-ray.png"));
    w.show();
    return a.exec();
}
