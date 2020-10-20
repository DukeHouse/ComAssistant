#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QDebug>

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    static QMutex mutex;
    static int8_t first_run = 0;
    mutex.lock();
    QString text;
    switch(type)
    {
    case QtDebugMsg:
        text = QString("Debug:   ");
        break;
    case QtWarningMsg:
        text = QString("Warning: ");
        break;
    case QtCriticalMsg:
        text = QString("Critical:");
        break;
    case QtFatalMsg:
        text = QString("Fatal:   ");
        break;
    default:
        text = QString("Debug:   ");
        break;
    }

//    QString context_info;
    QString current_date_time;
    QString current_date;
//    context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
    current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    current_date = QString("%1").arg(current_date_time);
    QString message = QString("[%1][%2] %3").arg(text).arg(current_date).arg(msg);

    QFile file("ComAssistantDebug.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream text_stream(&file);
    if(!first_run){
        text_stream << "\r\n"
                    << "-----------New Debug Log @"
                    << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd")
                    << "\r\n";
        first_run = 1;
    }
    text_stream << message << endl;
    file.flush();
    file.close();
    mutex.unlock();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //重定向qDebug到文件
#ifdef QT_NO_DEBUG
    qInstallMessageHandler(outputMessage);
#endif

    //国际化
    QTranslator translator;
    if (QLocale::system().name() != "zh_CN")
    {
        translator.load("en_US.qm");
        a.installTranslator(&translator);
    }

    MainWindow w;

//    w.show(); //在构造函数中调用

    return a.exec();
}
