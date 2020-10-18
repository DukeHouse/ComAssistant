#include "dataprotocol.h"

DataProtocol::DataProtocol(QObject *parent) : QObject(parent)
{
    MAXDATA_AS_END.append(static_cast<char>(0x00));
    MAXDATA_AS_END.append(static_cast<char>(0x00));
    MAXDATA_AS_END.append(static_cast<char>(0x80));
    MAXDATA_AS_END.append(static_cast<char>(0x7F));
    clearBuff();
}

DataProtocol::~DataProtocol()
{
}

void DataProtocol::setProtocolType(ProtocolType_e type, bool clearbuff)
{
    protocolType = type;
    if(clearbuff)
        clearBuff();
}

DataProtocol::ProtocolType_e DataProtocol::getProtocolType()
{
    return protocolType;
}

void DataProtocol::appendData(const QByteArray &data)
{
    tempDataPool.append(data);
}

void DataProtocol::parseData(bool enableSumCheck)
{
    //坑：如果一串数据前面的字符都匹配上了，最后几个字符没匹配上会引起正则匹配函数会消耗大量的时间，
    //导致卡顿，如果是偶尔误码这种问题影响不大，但是如果刻意制造这种数据并大量发送则会卡死，似乎无解，
    //或放到子线程中，但依然会消耗很多时间，只是不会卡住GUI而已
    #define MAX_EXTRACT_LENGTH 512
    parsePacksFromBuffer(tempDataPool, tempDataPool, enableSumCheck);
    //在解析完成后剔除前面已扫描过的数据
    if(tempDataPool.size() > MAX_EXTRACT_LENGTH)
    {
        tempDataPool = tempDataPool.mid(tempDataPool.size() - MAX_EXTRACT_LENGTH);
    }
}

//从缓存中提取所有包，每提取出一个包就解析一个
void DataProtocol::parsePacksFromBuffer(QByteArray& buffer, QByteArray& restBuffer, bool enableSumCheck)
{
    if(buffer.isEmpty())
        return;
    if(protocolType == Ascii){
        //先剔除\0,\r,\n等特殊数据
        buffer = buffer.trimmed();
        while (buffer.indexOf('\0')!=-1) {
            buffer.remove(buffer.indexOf('\0'), 1);
        }
        QRegularExpression reg;
        QRegularExpressionMatch match;
        int scanIndex = 0;
        int lastScannedIndex = 0;
        //匹配{}间的数据。
        //{:之间不允许再出现{:
        //:后，数据与逗号作为一个组，这个组至少出现一次，组中的逗号出现0次或1次，组开头允许有空白字符\\s
        //组中的数据：符号出现或者不出现，整数部分出现至少1次，小数点与小数作为整体，可不出现或者1次
        //换行符最多出现2次
        reg.setPattern("\\{[^{:]*:(\\s*([+-]?\\d+(\\.\\d+)?)?,?)+\\}");
        reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
        do {
                QByteArray onePack;
                match = reg.match(buffer, scanIndex);
                if(match.hasMatch()) {
                    scanIndex = match.capturedEnd();
                    lastScannedIndex = scanIndex;
                    //连续的逗号和分号逗号之间补0
                    onePack.clear();
                    onePack.append(match.captured(0).toLocal8Bit());
                    while(onePack.indexOf(",,")!=-1){
                        onePack.insert(onePack.indexOf(",,")+1,'0');
                    }
                    while(onePack.indexOf(":,")!=-1){
                        onePack.insert(onePack.indexOf(":,")+1,'0');
                    }
                    if(!onePack.isEmpty()){
                        while(onePack.indexOf('\r')!=-1)
                            onePack = onePack.replace("\r","");
                        while(onePack.indexOf('\n')!=-1)
                            onePack = onePack.replace("\n","");
                        RowData_t data = extractRowData(onePack);
                        addToDataPool(data, enableSumCheck);
                    }
//                    qDebug()<<"match"<<match.captured(0);
                }
                else{
//                    qDebug()<<"no match";
                    scanIndex++;
                }
        } while(scanIndex < buffer.size());

        restBuffer = buffer.mid(lastScannedIndex);

    }else if(protocolType == Float){
        QByteArray tmpArray = buffer;
        while (tmpArray.indexOf(MAXDATA_AS_END)!=-1) {
            QByteArray before = tmpArray.mid(0,tmpArray.indexOf(MAXDATA_AS_END));
            tmpArray = tmpArray.mid(tmpArray.indexOf(MAXDATA_AS_END)+MAXDATA_AS_END.size());
            if(before.size()%4==0){
                RowData_t data = extractRowData(before);
                addToDataPool(data, enableSumCheck);
            }
            else
                qDebug()<<"丢弃数据（长度不是4的倍数）："<<before.toHex().toUpper();
        }
        restBuffer = tmpArray;
    }
}

void DataProtocol::clearBuff()
{
    tempDataPool.clear();
    dataPool.clear();
}

int DataProtocol::parsedBuffSize()
{
    return dataPool.size();
}

QVector<double> DataProtocol::popOneRowData()
{
    QVector<double> tmp;
    if(dataPool.size()>0){
        tmp = dataPool.at(0);
        dataPool.pop_front();
    }
    return tmp;
}

inline DataProtocol::RowData_t DataProtocol::extractRowData(const Pack_t &pack)
{

    RowData_t rowData;
    Pack_t dataPack;

    if(pack.isEmpty())
        return rowData;

    if(protocolType == Ascii){
        //把数据部分提取出来
        Pack_t dataPack = pack.mid(pack.indexOf(':'));

        QRegularExpression reg;
        QRegularExpressionMatch match;
        int index = 0;
        reg.setPattern("[\\+-]?\\d+\\.?\\d*");//匹配实数 符号出现0、1次，数字至少1次，小数点0、1次，小数不出现或出现多次
        do {
                match = reg.match(dataPack, index);
                if(match.hasMatch()) {
                    index = match.capturedEnd();
                    rowData << match.captured(0).toDouble();
    //                qDebug()<<match.captured(0).toDouble();
                }
                else{
    //                qDebug()<<"no match";
                    break;
                }
        } while(index < dataPack.length());

    }else if(protocolType == Float){
        dataPack = pack;
        while (dataPack.size()>0) {
            float tmp;
            if(packToFloat(dataPack, tmp)){
                dataPack = dataPack.mid(4);
                rowData << static_cast<double>(tmp);
//                qDebug("tmp:%f",tmp);
            }else{
//                qDebug()<<"get max";
            }
        }
    }

    return rowData;
}

inline void DataProtocol::addToDataPool(RowData_t &rowData, bool enableSumCheck=false)
{
    #define SUPER_MIN_VALUE 0.00000001  //感觉这个值还是偏小了float好像达不到这个精度，大概精度在Keil调试模式下看到的float类型的精度
    if(rowData.size()<=0)
        return;

    if(enableSumCheck){
        //和校验模式 必须 要有至少两个数据
        if(rowData.size()<=1)
            return;
        float lastValue = rowData.at(rowData.size()-1); //要用float型的，double太过精确导致数据不一致
        float sum = 0;
        for(int i = 0; i < rowData.size()-1; i++)
            sum += rowData.at(i);
        if(abs(sum-lastValue)>SUPER_MIN_VALUE){
            qDebug()<<"addToDataPool sum check error"<<rowData;
            return;
        }
        rowData.pop_back();
    }
    dataPool << rowData;
}

bool DataProtocol::byteArrayToFloat(const QByteArray& array, float& result)
{
    if(array.size()<4)
        return false;

    char num[4];
    for(int i = 0; i<4; i++)
        num[i] = array.at(i);//
//    qDebug("%.2f", *(reinterpret_cast<float*>(num)));

    result = *(reinterpret_cast<float*>(num));
    return true;
}

bool DataProtocol::packToFloat(const Pack_t& pack , float& result)
{
    return byteArrayToFloat(pack, result);
}
