#ifndef DATAPROTOCOL_H
#define DATAPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QtDebug>
#include <QString>
#include <QRegularExpression>
#include <QVector>
#include <float.h>
#include <QElapsedTimer>
#include <QThread>
#include <QMutex>

//可视协议：{保留:数据1,数据2,...}
//透传协议: float数据转为小端模式传输，以00 00 80 7F结尾

//pack包定义：满足可视协议的数据包为pack包，典型特征为符号{和：和}和数据

class DataProtocol : public QObject
{
    Q_OBJECT
public:
    #define MAX_EXTRACT_LENGTH 512

    typedef enum{
        Ascii,
        Float
    }ProtocolType_e;
    //定义数据模型
    typedef double OneData_t;
    typedef QVector<OneData_t> RowData_t;
    typedef QVector<RowData_t> DataPool_t;
    //定义数据包和数据包流
    typedef QByteArray          Pack_t;

public:
    explicit DataProtocol(QObject *parent = nullptr);
    ~DataProtocol();
    void clearBuff();
    int parsedBuffSize();//判断数据池剩余大小
    QVector<double> popOneRowData();//弹出一行数据，没有数据则为空
    void setProtocolType(ProtocolType_e type, bool clearbuff=true);
    ProtocolType_e getProtocolType();
public slots:
    void appendData(const QByteArray &data);
    void parseData(bool enableSumCheck=false);
private:
    QMutex dataPoolLock;
    QMutex tempDataPoolLock;
    //从pack中提取合法数据行
    RowData_t extractRowData(const Pack_t& pack);
    //将合法数据行添加进数据池
    void addToDataPool(RowData_t &rowData, bool enableSumCheck);
    //数据池
    DataPool_t dataPool;
    QByteArray tempDataPool;
    //协议类型
    ProtocolType_e protocolType = Ascii;
    //最大常数
    QByteArray MAXDATA_AS_END;
    //数组前4个字节转float
    bool byteArrayToFloat(const QByteArray& array, float& result);
    //进一步筛除错误数据
    int32_t hasErrorStr_Ascii(QByteArray &input);
    //调用byteArrayToFloat
    bool packToFloat(const Pack_t& pack , float& result);
    void parsePacksFromBuffer(QByteArray& buffer, QByteArray& restBuffer,
                              QMutex &bufferLock, bool enableSumCheck);
};

#endif // DATAPROTOCOL_H
