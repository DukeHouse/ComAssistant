/**
 * @brief   自定义串口对象
 * @file    myserialport.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-2月-11
 * @note    对自带串口类进行了简化打包，并加入统计功能
 */
#ifndef MYSERIALPORT_H
#define MYSERIALPORT_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QList>
#include <QString>
#include <QByteArray>
#include <QtDebug>

#include <config.h>

class mySerialPort : public QSerialPort
{
public:
    mySerialPort();
    ~mySerialPort();
    //保留父类的open函数并重载open函数
    using QSerialPort::open;
    bool open(QString PortName,int BaudRate);
    int64_t write(const QByteArray& data);
    QByteArray readAll();
    //刷新串口
    QList<QString> refreshSerialPort();
    //端口数量变化检测。应周期性执行
    bool portAmountChanged();
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
    //更多设置，任何一个设置失败均会返回false
    bool moreSetting(StopBits stopbits=OneStop, Parity parity=NoParity,
                     FlowControl flowcontrol=NoFlowControl, DataBits databits=Data8);
private:
    QString numberStringAddSeprator(QString str);
    QString numberStringAddWarningColor(int64_t theCnt, QString theStr);
    int64_t TxCnt;
    int64_t RxCnt;
    DataBits databits;
    StopBits stopbits;
    FlowControl flowcontrol;
    Parity paritybit;
    int64_t totalTxCnt;
    int64_t totalRxCnt;
};

#endif // MYSERIALPORT_H
