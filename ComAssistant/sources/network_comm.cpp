#include "network_comm.h"

NetworkComm::NetworkComm(QObject *parent) : QObject(parent)
{
}

NetworkComm::~NetworkComm()
{
    disconnect();
}

/**
 * @brief     初始化
 * @note      应通过信号-槽形式调用确保在新线程中new这些对象
 */
void NetworkComm::init(void)
{
    socket = new QTcpSocket(this);
    socket->setProxy(QNetworkProxy::NoProxy);   // 设置不使用代理

    server = new QTcpServer(this);
    server->setMaxPendingConnections(1);        // 设置最大连接数量
    server->setProxy(QNetworkProxy::NoProxy);

    udpSocket = new QUdpSocket(this);
    udpSocket->setProxy(QNetworkProxy::NoProxy);

//    qDebug() << "Network ThreadID" << QThread::currentThreadId() << QThread::currentThread();
}

//增加千位分隔符
QString NetworkComm::numberStringAddSeprator(QString str)
{
    if(str.size() > 3 && str.size() <= 6)
    {
        str.insert(str.size() - 3, ',');
    }
    else if(str.size() > 6 && str.size() <= 9)
    {
        str.insert(str.size() - 6, ',');
        str.insert(str.size() - 3, ',');
    }
    else if(str.size() > 10)
    {
        str.insert(str.size() - 9, ',');
        str.insert(str.size() - 6, ',');
        str.insert(str.size() - 3, ',');
    }
    return str;
}
/**
 * @brief     字符串增加颜色信息
 * @param[in] 计数值
 * @param[in] 字符串
 * @return    带颜色信息的数字字符串
 */
QString NetworkComm::numberStringAddWarningColor(int64_t theCnt, QString theStr)
{
    #define ONE_MB      (1000*1000)
    #define WARNING_0   (ONE_MB*5)
    #define WARNING_1   (ONE_MB*10)
    #define WARNING_2   (ONE_MB*15)
    if(theCnt < WARNING_0)
    {
        // no code here.
    }
    else if(theCnt < WARNING_1)
    {
        theStr = "<font color=#ff7d46>" + theStr + "</font>";
    }
    else if(theCnt < WARNING_2)
    {
        theStr = "<font color=#FF5A5A>" + theStr + "</font>";
    }
    else
    {
        theStr = "<font color=#FF0000>" + theStr + "</font>";
    }
    return theStr;
}
/*
*/
QString NetworkComm::getTxRxString()
{
    QString txStr, rxStr, result;

    txStr = QString::number(getTxCnt());
    txStr = numberStringAddSeprator(txStr);//加分隔符

    rxStr = QString::number(getRxCnt());
    rxStr = numberStringAddSeprator(rxStr);

    return "T:" + txStr + " R:" + rxStr;
}
QString NetworkComm::getTxRxString_with_color()
{

    QString txStr, rxStr, result;

    // Tx
    txStr = QString::number(getTxCnt());
    txStr = numberStringAddSeprator(txStr);
//    txStr = numberStringAddWarningColor(getTxCnt(), txStr);//发送无需变色

    // Rx
    rxStr = QString::number(getRxCnt());
    rxStr = numberStringAddSeprator(rxStr);
    rxStr = numberStringAddWarningColor(getRxCnt(), rxStr);

    result = "T:" + txStr + " R:" + rxStr;

    return result;
}

int64_t NetworkComm::getTotalTxCnt()
{
    return totalTxCnt;
}

/*
*/
int64_t NetworkComm::getTotalRxCnt()
{
    return totalRxCnt;
}

int64_t NetworkComm::getTxCnt(void)
{
    return TxCnt;
}

int64_t NetworkComm::getRxCnt(void)
{
    return RxCnt;
}

/*
 * Function: reset tx/rx cnt statistics
*/
void NetworkComm::resetCnt()
{
    TxCnt = 0;
    RxCnt = 0;
}

void NetworkComm::resetTxCnt(void)
{
    TxCnt = 0;
}

void NetworkComm::resetRxCnt(void)
{
    RxCnt = 0;
}

/**
 * @brief     网络是否连接
 */
bool NetworkComm::isOpen(void)
{
    if(workMode == NET_UNKNOWN)
        return false;
    else
        return true;
}

/**
 * @brief     获取本机IP
 * @return    返回本机IP
 */
QString NetworkComm::getLocalIP(void)
{
    QString mIpAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            mIpAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (mIpAddress.isEmpty())
        mIpAddress = QHostAddress(QHostAddress::LocalHost).toString();
    return mIpAddress;
}

/**
 * @brief     设置远端UDP地址
 * @note      常用于UDP模式下发送数据前设置接收者
 * @param[in] 远端IP
 * @param[in] 远端端口
 */
void NetworkComm::setRemoteUdpAddr(QString ip, quint16 port)
{
    remoteUdpIp = QHostAddress(ip);
    remoteUdpPort = port;
}

/**
 * @brief     TCP Client模式下socket连接成功槽
 */
void NetworkComm::socketConnected(void)
{
    socketConnectedFlag = true;
}

/**
 * @brief     主动连接
 * @note      不同模式下，目标地址和目标端口含义都有点区别
 * @param[in] 模式
 * @param[in] 目标地址
 * @param[in] 目标端口
 * @return    连接成功。部分情况下，连接失败信息也会通过信号方式传递
 */
int32_t NetworkComm::connect(qint32 mode, QString ip, quint16 port)
{
    QString errorDetail;
    // 存在已建立连接
    if(workMode != NET_UNKNOWN)
    {
        goto connect_failed;
    }

    // 建立连接
    switch (mode)
    {
    case TCP_CLIENT:
        if(!socket)
            goto connect_failed;
        QObject::connect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
        socket->connectToHost(QHostAddress(ip), port);
        if(!socket->waitForConnected(DEFAULT_TIME_OUT))
        {
            //官方说connectToHost在windows下有可能失败，所以通过eventloop和connected信号综合判断
            qApp->processEvents();
            if(!socketConnectedFlag)
            {
                errorDetail = socket->errorString();
                goto connect_failed;
            }
        }
        QObject::connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
                         this, SLOT(abstractSocketError(QAbstractSocket::SocketError)));
        QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readData()));
        QObject::connect(socket, SIGNAL(disconnected()), this, SLOT(clientDisconnect()));
        break;
    case TCP_SERVER:
        if(!server)
            goto connect_failed;
        if(ip != getLocalIP() && ip != DEFAULT_IP)
        {
            errorDetail = "uncorrect ip: " + ip;
            goto connect_failed;
        }
        if(!server->listen(QHostAddress(ip), port))
        {
            errorDetail = server->errorString();
            goto connect_failed;
        }
        QObject::connect(server, SIGNAL(newConnection()), this, SLOT(serverNewConnect()));
        QObject::connect(server, SIGNAL(acceptError(QAbstractSocket::SocketError)),
                         this, SLOT(serverAcceptError(QAbstractSocket::SocketError)));
        break;
    case UDP_SERVER:
        //监听这个地址，同时从这个地址发数据出去
        if(!udpSocket->bind(QHostAddress(ip), port))
        {
            errorDetail = udpSocket->errorString();
            goto connect_failed;
        }
        QObject::connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readData()));
        QObject::connect(udpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                         this, SLOT(abstractSocketError(QAbstractSocket::SocketError)));
        break;
    case UDP_CLIENT:
        if(!udpSocket->open(QIODevice::ReadWrite))
        {
            errorDetail = udpSocket->errorString();
            goto connect_failed;
        }
        //只设置远端地址，发送端口未指定，因为指定后无法在本机进行回环通信
        remoteUdpIp = QHostAddress(ip);
        remoteUdpPort = port;
        QObject::connect(udpSocket, SIGNAL(readyRead()), this, SLOT(readData()));
        QObject::connect(udpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                         this, SLOT(abstractSocketError(QAbstractSocket::SocketError)));
        break;
    default:
        errorDetail = "unknown connection mode";
        goto connect_failed;
    }
    workMode = mode;
    return 0;
connect_failed:
    workMode = NET_UNKNOWN;
    emit error(NET_ERR_CONNECT, errorDetail);
    qDebug() << __FILE__ << __FUNCTION__ << "failed";
    return -1;
}

/**
 * @brief     TCP Server模式下有客户端请求断开连接的槽
 * @note      重置socket
 * @return    重置成功标记
 */
int32_t NetworkComm::disconnectServerSocket()
{
    //空指针则说明没有连接，无需进行断连操作
    if(!serverSocket)
        return 0;
    //先断开信号避免响应
    QObject::disconnect(serverSocket, SIGNAL(readyRead()), this, SLOT(readData()));
    QObject::disconnect(serverSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnect()));
    //断开前记录要断开的地址
    QString peerAddress = serverSocket->peerAddress().toString();
    QString peerPort = QString::number(serverSocket->peerPort());
    if(serverSocket->state() == QAbstractSocket::ConnectedState)
    {
        serverSocket->disconnectFromHost();
        if(serverSocket->state() == QAbstractSocket::ConnectedState &&
            !serverSocket->waitForDisconnected(DEFAULT_TIME_OUT))
        {
            return -1;
        }
    }
    QObject::disconnect(serverSocket, SIGNAL(error(QAbstractSocket::SocketError)),  //TODO:能不能一直链接信号？
                     this, SLOT(abstractSocketError(QAbstractSocket::SocketError)));
    serverSocket->close();
    emit message(NET_MSG_DISCONNECT,
                 peerAddress + ":" + peerPort);
    serverSocket = nullptr;
    return 0;
}

/**
 * @brief     主动断开连接
 * @return    断开连接成功标记
 */
int32_t NetworkComm::disconnect()
{
    if(workMode == NET_UNKNOWN)
    {
        return 0;
    }

    QString errorDetail;
    // 取消连接
    switch (workMode)
    {
    case TCP_CLIENT:
        if(!socket)
            goto failed;
        //先断开断开连接的信号槽，主动的断开连接不触发断连信号
        QObject::disconnect(socket, SIGNAL(disconnected()), this, SLOT(clientDisconnect()));
        socket->disconnectFromHost();
        if (socket->state() == QAbstractSocket::ConnectedState &&
            !socket->waitForDisconnected(DEFAULT_TIME_OUT))
        {
            errorDetail = socket->errorString();
            goto failed;
        }
        QObject::disconnect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
        QObject::disconnect(socket, SIGNAL(readyRead()), this, SLOT(readData()));
        QObject::disconnect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
                         this, SLOT(abstractSocketError(QAbstractSocket::SocketError)));
        socket->close();
        socketConnectedFlag = false;
        break;
    case TCP_SERVER:
        if(disconnectServerSocket())
        {
            errorDetail = serverSocket->errorString();
            goto failed;
        }
        if(!server)
            goto failed;
        QObject::disconnect(server, SIGNAL(newConnection()), this, SLOT(serverNewConnect()));
        QObject::disconnect(server, SIGNAL(acceptError(QAbstractSocket::SocketError)),
                         this, SLOT(serverAcceptError(QAbstractSocket::SocketError)));
        server->close();
        break;
    case UDP_SERVER:
        udpSocket->close();
        QObject::disconnect(udpSocket, SIGNAL(readyRead()), this, SLOT(readData()));
        QObject::disconnect(udpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                         this, SLOT(abstractSocketError(QAbstractSocket::SocketError)));
        break;
    case UDP_CLIENT:
        udpSocket->close();
        QObject::disconnect(udpSocket, SIGNAL(readyRead()), this, SLOT(readData()));
        QObject::disconnect(udpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
                         this, SLOT(abstractSocketError(QAbstractSocket::SocketError)));
        break;
    default:
        goto failed;
    }

    // 重置工作模式
    workMode = NET_UNKNOWN;
    return 0;
failed:
    workMode = NET_UNKNOWN;
    emit error(NET_ERR_DISCONNECT, errorDetail);
    qDebug() << __FILE__ << __FUNCTION__ << "failed";
    return -1;
}

/**
 * @brief     发送数据
 * @param[in] 发送的数据
 * @return    发送的数据大小，-1表示失败
 */
int64_t NetworkComm::write(const QByteArray &data)
{
    QString errorDetails;
    int64_t ret = -1;
    switch (workMode)
    {
    case TCP_CLIENT:
        if(!socket)
            return ret;
        ret = socket->write(data);
        errorDetails = socket->errorString();
        break;
    case TCP_SERVER:
        if(!serverSocket)
            return ret;
        ret = serverSocket->write(data);
        errorDetails = serverSocket->errorString();
        break;
    case UDP_SERVER:
        ret = udpSocket->writeDatagram(data, remoteUdpIp, remoteUdpPort);
        errorDetails = udpSocket->errorString();
        break;
    case UDP_CLIENT:
        ret = udpSocket->writeDatagram(data, remoteUdpIp, remoteUdpPort);
        errorDetails = udpSocket->errorString();
        break;
    default:
        break;
    }
    if(ret > 0)
    {
        TxCnt += ret;
        totalTxCnt += ret;
        emit bytesWritten(ret);
    }
    else
    {
        emit error(NET_ERR_WRITE, errorDetails);
    }
    return ret;
}

/**
 * @brief     读取数据
 * @note      该函数为内部函数
 * @return
 */
void NetworkComm::readData(void)
{
    QString errorDetails;
    QHostAddress recvUdpIP;
    quint16 recvUdpPort;
    int64_t ret = 0;
    QByteArray array;
    switch (workMode)
    {
    case TCP_CLIENT:
        if(!socket)
            return;
        array.resize(socket->bytesAvailable());
        socket->read(array.data(), array.size());
        array = socket->readAll();
        errorDetails = socket->errorString();
        break;
    case TCP_SERVER:
        if(!serverSocket)
            return;
        array.resize(serverSocket->bytesAvailable());
        serverSocket->read(array.data(), array.size());
        errorDetails = serverSocket->errorString();
        break;
    case UDP_SERVER:
        ret = udpSocket->bytesAvailable();
        array.resize(ret);
        udpSocket->readDatagram(array.data(), array.size(), &recvUdpIP, &recvUdpPort);
        if(recvUdpPort == 0 && recvUdpIP.isNull())
        {
            qDebug() << "error remote udp addr" << recvUdpIP << recvUdpPort << "at" << __FILE__ << __FUNCTION__;
            emit error(NET_ERR_READ,
                       "Error recv UDP addr:" + recvUdpIP.toString() + QString::number(recvUdpPort));
            return;
        }
        if(recvUdpPort != remoteUdpPort || recvUdpIP != remoteUdpIp)
        {
            remoteUdpIp = recvUdpIP;
            remoteUdpPort = recvUdpPort;
            emit message(NET_MSG_NEW_CONNECT,
                         recvUdpIP.toString() + ":" + QString::number(recvUdpPort));
        }
        errorDetails = udpSocket->errorString();
        break;
    case UDP_CLIENT:
        ret = udpSocket->bytesAvailable();
        array.resize(ret);
        udpSocket->readDatagram(array.data(), array.size());
        errorDetails = udpSocket->errorString();
        break;
    default:
        break;
    }
    ret = array.size();

    if(ret > 0)
    {
        RxCnt += ret;
        totalRxCnt += ret;
        emit readBytes(array);//不知道为什么不能模仿串口使用readReady并配合readAll使用
    }
    else
    {
        emit error(NET_ERR_READ, errorDetails);
    }
    return;
}

/**
 * @brief     客户端断开连接的槽
 * @note      客户端可以指自己，也可以指远端
 */
void NetworkComm::clientDisconnect(void)
{
    QString errorDetails;
    switch (workMode)
    {
    case TCP_CLIENT:
        if(!socket)
            goto over;
        //在该模式下主要是指被动关闭，如对方服务器主动断开连接
        QObject::disconnect(socket, SIGNAL(connected()), this, SLOT(socketConnected()));
        QObject::disconnect(socket, SIGNAL(readyRead()), this, SLOT(readData()));
        QObject::disconnect(socket, SIGNAL(disconnected()), this, SLOT(clientDisconnect()));
        QObject::disconnect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
                         this, SLOT(abstractSocketError(QAbstractSocket::SocketError)));
        socket->close();
        socketConnectedFlag = false;
        workMode = NET_UNKNOWN;
        emit error(NET_ERR_DISCONNECT, errorDetails);
        break;
    case TCP_SERVER:
        if(disconnectServerSocket())
        {
            return;
        }
        emit message(NET_MSG_DISCONNECT, errorDetails);
        break;
    case UDP_SERVER:
        break;
    case UDP_CLIENT:
        break;
    default:
        break;
    }
over:
    return;
}

/**
 * @brief     服务器有新的连接请求
 * @note      目前只支持单一连接，后续连接的socket会主动拒绝掉
 */
void NetworkComm::serverNewConnect(void)
{
    if(serverSocket)
    {
        qDebug() << "Connection refused, multi-client is not supported";
        QTcpSocket *nextSocket = server->nextPendingConnection();
        nextSocket->disconnectFromHost();
        nextSocket->close();
        emit error(NET_ERR_MULTI_SOCKET,
                "Connection refused, multi-client is not supported");
        return;
    }

    serverSocket = server->nextPendingConnection();
    if(!serverSocket)
    {
        qDebug() << "null server->nextPendingConnection() at" << __FUNCTION__;
        return;
    }
    QObject::connect(serverSocket, SIGNAL(readyRead()), this, SLOT(readData()));
    QObject::connect(serverSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnect()));
    emit message(NET_MSG_NEW_CONNECT,
                 serverSocket->peerAddress().toString()
                 + ":" + QString::number(serverSocket->peerPort()));
}

/**
 * @brief     TCP Server模式下的错误
 */
void NetworkComm::serverAcceptError(QAbstractSocket::SocketError err)
{
    qDebug() << __FUNCTION__ << err << server->errorString();
    emit error(NET_ERR_ACCEPT_ERR, "code:" + QString(err) + " server_err_str:" + server->errorString());
}

/**
 * @brief     抽象socket错误
 */
void NetworkComm::abstractSocketError(QAbstractSocket::SocketError err)
{
    QString errorStr = __FUNCTION__;
    switch (workMode) {
    case NET_UNKNOWN:
        errorStr += QString(" mode:NET_UNKNOWN code:") + QString(err);
        break;
    case TCP_CLIENT:
        errorStr += QString(" mode:TCP_CLIENT code:") + QString(err) + " socket_err_str:" + socket->errorString();
        break;
    case TCP_SERVER:
        errorStr += QString(" mode:TCP_SERVER code:") + QString(err) + " server_err_str:" + server->errorString();
        if(serverSocket)
        {
            errorStr += " serverSocket_err_str:" + serverSocket->errorString();
        }
        break;
    case UDP_CLIENT:
        errorStr += QString(" mode:UDP_CLIENT code:") + QString(err) + " udp_err_str:" + udpSocket->errorString();
        break;
    case UDP_SERVER:
        errorStr += QString(" mode:UDP_SERVER code:") + QString(err) + " udp_err_str:" + udpSocket->errorString();
        break;
    default:
        errorStr += QString(" mode:unknown code:") + QString(err);
        break;
    }
    qDebug() << errorStr;
    emit error(NET_ERR_SOCKET_ERR, errorStr);
}
