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
#include "math.h"

#define DEFAULT_PLOT_TITLE_MACRO "plotter"
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
        Float,
        CSV,
        MAD
    }ProtocolType_e;
    //定义数据模型
    typedef double OneData_t;
    typedef QVector<OneData_t> RowData_t;
    typedef QVector<RowData_t> DataPool_t;
    //定义数据包和数据包流
    typedef QByteArray          Pack_t;
    //定义数据组和数据集合
    typedef struct{
        QString name;
        DataPool_t dataPool;
    }DataPoolGroup_t;
    typedef QVector<DataPoolGroup_t> DataPoolSets;

public:
    explicit DataProtocol(QObject *parent = nullptr);
    ~DataProtocol();
    int32_t hasParsedBuff();//判断数据池剩余大小
    int32_t popOneRowData(QString &outName, QVector<double> &outRowData);//弹出一行数据，没有数据则为空
    void setProtocolType(ProtocolType_e type, bool clearbuff=true);
    ProtocolType_e getProtocolType();
    QString getDefaultPlotterTitle();
    int32_t setDefaultPlotterTitle(QString title);
public slots:
    void clearBuff(const QString &name);
    void appendData(const QByteArray &data);
    void parseData(bool enableSumCheck=false);
private:
    QString defaultPlotTitle = DEFAULT_PLOT_TITLE_MACRO;
    int32_t hasParsedBuffCnt = 0;
    QMutex dataPoolLock;
    QMutex tempDataPoolLock;
    //从pack中提取合法数据行
    RowData_t extractRowData(const Pack_t &pack, QString &outName, RowData_t &outRowData);
    //将合法数据行添加进数据池
    void addToDataPool(QString &name, RowData_t &rowData, bool enableSumCheck);
    //数据池
    DataPoolSets dataPoolSets;
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
    QByteArray& packetDecorationOfCSV(QByteArray& onePack);
};

#endif // DATAPROTOCOL_H
