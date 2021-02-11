/**
 * @brief   HTTP网络请求处理文件
 * @file    http.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-2月-11
 * @note    检查更新、下载信息、检查版本等功能
 */
#ifndef HTTP_H
#define HTTP_H

//网络类
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QNetworkInterface>
//Json类
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include <QMainWindow>
#include <QtDebug>
#include <QTimer>
#include <QDesktopServices>
#include <QMessageBox>
#include <QMutex>

#include "config.h"

#include <QObject>

namespace Ui {
class MainWindow;
}

class HTTP: public QObject
{
    Q_OBJECT
    #define TIMEOUT_SECOND  5
public:
    typedef enum{
        Idle,
        GetVersion, //获取版本号
        BackStageGetVersion,//后台检查更新
        BackStageGetVersion_MyServer, //从私人服务器获取版本号
        DownloadFile, //下载文件，暂未使用
        PostStatic, //上传统计
        DownloadMSGs, //信息发布系统
    }HttpFunction_e;
    HTTP(QWidget *parentWidget);

    static QString getHostMacAddress();
    bool postUsageStatistic(void);
    bool getRemoteVersion(void);
    bool getRemoteVersion_my_server(void);
    bool downloadMessages(void);
    void addTask(HttpFunction_e name);
    QStringList getMsgList();

private:
    int32_t parseReleaseInfo(QString &inputStr, QString &remoteVersion, QString &remoteNote, QString &publishedTime);

    QMutex httpLock;
    QWidget *parent;
    QTimer secTimer;
    //http访问
    QNetworkAccessManager *m_NetManger;
    QNetworkReply* m_Reply = nullptr;
    QVector<HttpFunction_e> httpTaskVector;
    //由于共用m_Reply指针，因此httpTask实际只能运行完一个后才能运行下一个，
    //所以httpTimeout也有流控作用
    //把m_Replay改为向量后同时向外发包则问题在于不知道网络回的包属于哪一个task,也不太好搞
    int httpTimeout = 0;
    QStringList msgList;//远端下载的信息列表
    HttpFunction_e cur_task = Idle;
private slots:
    void httpTimeoutHandle();
    void httpFinishedSlot(QNetworkReply *reply);
};

#endif // HTTP_H
