#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include <QDebug>
#include "config.h"

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)

    if(!g_log_record)
    {
        return;
    }

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
    current_date_time = QDateTime::currentDateTime().toString("hh:mm:ss");
    current_date = QString("%1").arg(current_date_time);
    QString message = QString("[%1][%2] %3").arg(text).arg(current_date).arg(msg);

    QString log_header = "-----------New Debug Log @";
    QFile file("ComAssistantDebug.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream text_stream(&file);
    //首次运行添加log头
    if(!first_run){
        text_stream << "\r\n"
                    << log_header
                    << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd")
                    << " V" + Config::getVersion()
                    << "\r\n";
    }
    text_stream << message << endl;
    file.flush();
    file.close();

    //文件大小限制(超出大小的将按log_header进行删除)
    if(!first_run && file.size() > 1024*1024)
    {
        QByteArray temp;
        file.open(QIODevice::ReadWrite);
        temp = file.readAll();
        //至少2个log记录
        if(temp.indexOf(log_header) != -1 && temp.indexOf(log_header, log_header.size()) != -1)
        {
            file.close();
            temp = temp.mid(log_header.size());
            temp = temp.mid(temp.indexOf(log_header));
            file.open(QIODevice::ReadWrite|QIODevice::Truncate);
        }
        else
        {
            temp.clear();
        }
        file.write(temp);
        file.flush();
        file.close();
    }
    first_run = 1;

    mutex.unlock();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //重定向qDebug到文件
#ifdef QT_NO_DEBUG
    g_log_record = Config::getLogRecord();
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

    // 不同意相关声明则退出(为啥qApp->quit()没效果?)
    if(!g_agree_statement)
    {
        return 0;
    }

    return a.exec();
}
