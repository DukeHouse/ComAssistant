/**
 * @brief   TCP、UDP通信文件
 * @file    network_comm.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-3月-28
 * @note    集成了TCP Server、TCP Client、UDP Server等通信功能于一起的文件
 *          同一时间下只有一种模式能工作
 *          通过connect的时候指定不同的模式
 *          通过write进行发送数据
 *          通过read信号来读取数据
 */
#ifndef NETWORK_COMM_H
#define NETWORK_COMM_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QVector>
#include <QDebug>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QThread>
#include <QNetworkProxy>
#include <QApplication>

#define DEFAULT_IP              "127.0.0.1"
#define DEFAULT_PORT            1234
#define DEFAULT_TIME_OUT        1000

//网络错误类型
#define NET_ERR_CONNECT         (0)
#define NET_ERR_DISCONNECT      (1)
#define NET_ERR_WRITE           (2)
#define NET_ERR_READ            (3)
#define NET_ERR_MULTI_SOCKET    (4)
#define NET_ERR_ACCEPT_ERR      (5)
#define NET_ERR_SOCKET_ERR      (6)

//网络消息类型
#define NET_MSG_NEW_CONNECT     (0)
#define NET_MSG_DISCONNECT      (1)

//网络工作模式workmode
#define NET_UNKNOWN             (0)
#define TCP_CLIENT              (1)
#define TCP_SERVER              (2)
#define UDP_CLIENT              (3)
#define UDP_SERVER              (4)

class NetworkComm : public QObject
{
    Q_OBJECT
public:
    explicit NetworkComm(QObject *parent = nullptr);
    ~NetworkComm();
    QString getLocalIP(void);
    void setRemoteUdpAddr(QString ip, quint16 port);
    //获取收发统计值
    int64_t getTxCnt();
    int64_t getRxCnt();
    int64_t getTotalTxCnt();
    int64_t getTotalRxCnt();
    QString getTxRxString();
    QString getTxRxString_with_color();
    //重置收发统计
    void resetCnt();
    void resetTxCnt();
    void resetRxCnt();
    bool isOpen();

public slots:
    void init(void);
    int64_t write(const QByteArray &data);
    int32_t disconnect();
    int32_t connect(qint32 mode = TCP_CLIENT, QString ip = DEFAULT_IP, quint16 port = DEFAULT_PORT);

signals:
    void readBytes(const QByteArray &data);
    void bytesWritten(qint64);
    void error(qint32 errorCode, QString errorDetail);
    void message(qint32 type, QString msg);

private:
    int32_t disconnectServerSocket();
    QString numberStringAddSeprator(QString str);
    QString numberStringAddWarningColor(int64_t theCnt, QString theStr);
    QTcpSocket* socket = nullptr;
    QTcpServer* server = nullptr;
    QTcpSocket* serverSocket = nullptr;
    QUdpSocket* udpSocket = nullptr;
    QHostAddress remoteUdpIp;
    uint16_t remoteUdpPort;
    qint32 workMode = NET_UNKNOWN;
    int64_t TxCnt = 0;
    int64_t RxCnt = 0;
    int64_t totalTxCnt = 0;
    int64_t totalRxCnt = 0;
    bool socketConnectedFlag = false;

private slots:
    void readData(void);
    void clientDisconnect(void);
    void serverNewConnect(void);
    void serverAcceptError(QAbstractSocket::SocketError error);
    void socketConnected(void);
    void abstractSocketError(QAbstractSocket::SocketError error);
};

#endif // NETWORK_COMM_H
