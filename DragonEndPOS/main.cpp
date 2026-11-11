#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFile>
#include <QStyleFactory>
#include <QDebug>

int main(int argc, char *argv[])
{
    //starts the application, applies the theme and translations, and runs the main window
    QApplication a(argc, argv);

    a.setStyle(QStyleFactory::create("Fusion"));

    QFile themeFile(":/theme/dragons_end.qss");

    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        a.setStyleSheet(
            QString::fromUtf8(themeFile.readAll())
        );
    }
    else {
        qWarning() << "Could not load the theme:"
                   << themeFile.errorString();
    }

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "DragonEndPOS_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}
