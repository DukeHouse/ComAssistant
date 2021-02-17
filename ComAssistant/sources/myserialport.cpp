#include "myserialport.h"

mySerialPort::mySerialPort():TxCnt(0),RxCnt(0),totalTxCnt(0),totalRxCnt(0)
{
    moreSetting(Config::getStopBits(),Config::getParity(),Config::getFlowControl(),Config::getDataBits());
}

mySerialPort::~mySerialPort()
{
    if(mySerialPort::isOpen())
        mySerialPort::close();
}

/*
 * Function: reset tx/rx cnt statistics
*/
void mySerialPort::resetCnt()
{
    TxCnt = 0;
    RxCnt = 0;
}

/*
 * Function: reset tx cnt statistics
*/
void mySerialPort::resetTxCnt()
{
    TxCnt = 0;
}

/*
 * Function: reset rx cnt statistics
*/
void mySerialPort::resetRxCnt()
{
    RxCnt = 0;
}

/*
 * Function: refresh serial port
 * Parameter: none
 * Return: QList<QString>
*/
QList<QString> mySerialPort::refreshSerialPort()
{
    QList<QString> tmp;
    QSerialPort TmpSerial;

    //搜索串口
    foreach (const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        QString status;
        QString msg;
        status = info.isBusy() ? "BUSY  " : "IDLE  ";
        msg = info.portName() +
              "(" +
              status + info.description() +
              ")" +
              " " + info.manufacturer();
        tmp.append(msg);
    }

    //排序
    QList<QString> sorted;
    if(false == tmp.isEmpty()){
        QList<QString> tmp2;//用于存放COM0-COM9的条目
        QList<QString> tmp3;//用于存放COM10-COM99的条目
        for(int i = 0; i < tmp.size(); i++){
            //COM号在0-9
            if(tmp.at(i).indexOf("(") == 4){
                tmp2.append(tmp.at(i));
            }
            else if(tmp.at(i).indexOf("(") == 5){
                tmp3.append(tmp.at(i));
            }

        }
        tmp2.sort();
        tmp3.sort();
        sorted.append(tmp2);
        sorted.append(tmp3);
    }else{
        sorted = tmp;
    }

    return sorted;
}

/*
 *  如果当前设备处于占用状态，又被拔出后可能无法检测到数量变化，需要用错误处理槽函数先进行处理
*/
bool mySerialPort::portAmountChanged()
{
    static int lastPortAmount = 0;
    int currentPortAmount = QSerialPortInfo::availablePorts().size();
    if(lastPortAmount != currentPortAmount){
        lastPortAmount = currentPortAmount;
        return true;
    }else{
        return false;
    }
}

/*
 * Function: 获取发送统计
*/
int64_t mySerialPort::getTxCnt()
{
    return TxCnt;
}

/*
*/
int64_t mySerialPort::getRxCnt()
{
    return RxCnt;
}

/*
 * Function: 获取发送统计
*/
int64_t mySerialPort::getTotalTxCnt()
{
    return totalTxCnt;
}

/*
*/
int64_t mySerialPort::getTotalRxCnt()
{
    return totalRxCnt;
}

//增加千位分隔符
QString mySerialPort::numberStringAddSeprator(QString str)
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
QString mySerialPort::numberStringAddWarningColor(int64_t theCnt, QString theStr)
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
QString mySerialPort::getTxRxString()
{
    QString txStr, rxStr, result;

    txStr = QString::number(getTxCnt());
    txStr = numberStringAddSeprator(txStr);//加分隔符

    rxStr = QString::number(getRxCnt());
    rxStr = numberStringAddSeprator(rxStr);

    return "T:" + txStr + " R:" + rxStr;
}
QString mySerialPort::getTxRxString_with_color()
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
/*
*/
bool mySerialPort::open(QString PortName,int BaudRate)
{
    mySerialPort::setPortName(PortName);
    mySerialPort::setBaudRate(BaudRate);
//    mySerialPort::setDataBits(mySerialPort::Data8);
//    mySerialPort::setParity(mySerialPort::NoParity);
//    mySerialPort::setStopBits(mySerialPort::OneStop);
//    mySerialPort::setFlowControl(mySerialPort::NoFlowControl);

    return mySerialPort::open(mySerialPort::ReadWrite);
}

/*
 *
*/
int64_t mySerialPort::write(const QByteArray& data)
{
    int64_t tmp;

    tmp = QSerialPort::write(data);
    if(tmp != -1){
        TxCnt+=static_cast<int64_t>(data.size());
        totalTxCnt+=static_cast<int64_t>(data.size());
    }

    return tmp;
}

/*
 *
*/
QByteArray mySerialPort::readAll()
{
    QByteArray tmp;

    tmp = QSerialPort::readAll();
    if(!tmp.isEmpty()){
        RxCnt+=static_cast<int64_t>(tmp.size());
        totalRxCnt+=static_cast<int64_t>(tmp.size());
    }

    return tmp;
}

bool mySerialPort::moreSetting(StopBits sb, Parity pa, FlowControl fc, DataBits db)
{
    databits = db;
    stopbits = sb;
    flowcontrol = fc;
    paritybit = pa;

    return  mySerialPort::setDataBits(databits) &&
            mySerialPort::setParity(paritybit) &&
            mySerialPort::setStopBits(stopbits) &&
            mySerialPort::setFlowControl(flowcontrol);
}
