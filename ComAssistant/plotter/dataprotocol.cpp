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
    QByteArray tmp;
    if(protocolType == Ascii)
    {
        //剔除正则匹配无法很好支持的字符：中文、'\0'
        foreach(char ch, data)
        {
            if(ch & 0x80 || ch == '\0')
            {
                continue;
            }
            tmp.append(ch);
        }
    }
    else
    {
        tmp = data;
    }
    tempDataPoolLock.lock();
    tempDataPool.append(tmp);
    tempDataPoolLock.unlock();
}

void DataProtocol::parseData(bool enableSumCheck)
{
    parsePacksFromBuffer(tempDataPool, tempDataPool, 
                         tempDataPoolLock, enableSumCheck);

    //在解析完成后剔除前面已扫描过的数据
    if(tempDataPool.size() > MAX_EXTRACT_LENGTH)
    {
        tempDataPoolLock.lock();
        tempDataPool = tempDataPool.mid(tempDataPool.size() - MAX_EXTRACT_LENGTH);
        tempDataPoolLock.unlock();
    }
}

inline int32_t DataProtocol::hasErrorStr_Ascii(QByteArray &input)
{
    int32_t err = 0;
    QRegularExpression reg;
    QRegularExpressionMatch match;
    //不应该出现1-2这样的数据，符号前面要么是逗号要么是空格
    reg.setPattern("\\d[+-]");
    reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
    match = reg.match(input, 0);
    if(match.hasMatch()) {
        err--;
    }
    if(err)
    {
        return err;
    }
    //不应该出现1.2.3这样的数据
    reg.setPattern("\\.\\d+\\.");
    reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
    match = reg.match(input, 0);
    if(match.hasMatch()) {
        err--;
    }
    return err;
}

//从缓存中提取所有包，每提取出一个包就解析一个
inline void DataProtocol::parsePacksFromBuffer(QByteArray& buffer, QByteArray& restBuffer,
                                               QMutex &bufferLock, bool enableSumCheck)
{
    //坑：如果一串数据前面的字符都匹配上了，最后几个字符没匹配上会消耗大量的时间
    //或者说在大量匹配数据中穿插一组连续的不匹配数据，此时由于需要scanIndex遍历也会异常耗时
    //主要耗时点为match函数和scanIndex+1遍历，导致卡顿。刻意制造这种数据会使处理效率剧烈降低
    //目前解决办法是限制scanIndex遍历长度为512
    if(buffer.isEmpty())
        return;
    if(protocolType == Ascii){
        bufferLock.lock();
        //先剔除\0,\r,\n等特殊数据
        buffer = buffer.trimmed();
        while (buffer.indexOf('\0')!=-1) {
            buffer.remove(buffer.indexOf('\0'), 1);
        }
        bufferLock.unlock();
        QRegularExpression reg;
        QRegularExpressionMatch match;
        int scanIndex = 0;
        int lastScannedIndex = 0;
        //匹配{}间的数据。
        //{:之间不允许再出现{:
        //:后，数据与逗号作为一个组，这个组至少出现一次，组中的逗号出现0次或1次，组开头允许有空白字符\\s
        //组中的数据：符号出现或者不出现，整数部分出现至少1次，小数点与小数作为整体，可不出现或者1次
        //换行符最多出现2次
        //少数匹配错误的数据由hasErrorStr_Ascii进一步筛选
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
                    if(hasErrorStr_Ascii(onePack) != 0)
                    {
                        qDebug()<<"hasErrorStr_Ascii"<<onePack;
                        continue;
                    }
                    //补0
                    while(onePack.indexOf(",,") != -1){
                        onePack.insert(onePack.indexOf(",,") + 1,'0');
                    }
                    while(onePack.indexOf(":,") != -1){
                        onePack.insert(onePack.indexOf(":,") + 1,'0');
                    }
                    if(!onePack.isEmpty()){
                        RowData_t data = extractRowData(onePack);
                        addToDataPool(data, enableSumCheck);
                    }
//                    qDebug()<<"match"<<match.captured(0);
                }
                else{
//                    qDebug()<<"no match";
                    if(buffer.size() - scanIndex > MAX_EXTRACT_LENGTH)
                    {
                        scanIndex += MAX_EXTRACT_LENGTH;
                    }
                    else
                    {
                        scanIndex++;
                    }
                }
        } while(scanIndex < buffer.size());

        bufferLock.lock();
        restBuffer = buffer.mid(lastScannedIndex);
        bufferLock.unlock();
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
    else if(protocolType == CSV)
    {
        bufferLock.lock();
        while (buffer.indexOf('\0')!=-1) {
            buffer.remove(buffer.indexOf('\0'), 1);
        }
        bufferLock.unlock();
        QRegularExpression reg;
        QRegularExpressionMatch match;
        int scanIndex = 0;
        int lastScannedIndex = 0;
        //逗号分隔符格式
        reg.setPattern("(\\s*([+-]?\\d+(\\.\\d+)?)?,?)+\r?\n");
        reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
        do {
                QByteArray onePack;
                match = reg.match(buffer, scanIndex);
                if(match.hasMatch()) {
                    scanIndex = match.capturedEnd();
                    lastScannedIndex = scanIndex;
                    onePack.clear();
                    onePack.append(match.captured(0).toLocal8Bit());
                    if(hasErrorStr_Ascii(onePack) != 0)
                    {
                        qDebug()<<"hasErrorStr_CSV"<<onePack;
                        continue;
                    }
                    //剔除\r\n
                    onePack = onePack.trimmed();
                    //补0
                    while(onePack.indexOf(",,") != -1){
                        onePack.insert(onePack.indexOf(",,") + 1,'0');
                    }
                    if(onePack.startsWith(','))
                    {
                        onePack.push_front('0');
                    }
                    if(onePack.endsWith(','))
                    {
                        onePack.remove(onePack.size() - 1, 1);
                    }
                    if(!onePack.isEmpty()){
                        RowData_t data = extractRowData(onePack);
                        addToDataPool(data, enableSumCheck);
                    }
//                    qDebug()<<"match"<<match.captured(0);
                }
                else{
//                    qDebug()<<"no match";
                    if(buffer.size() - scanIndex > MAX_EXTRACT_LENGTH)
                    {
                        scanIndex += MAX_EXTRACT_LENGTH;
                    }
                    else
                    {
                        scanIndex++;
                    }
                }
        } while(scanIndex < buffer.size());

        bufferLock.lock();
        restBuffer = buffer.mid(lastScannedIndex);
        bufferLock.unlock();
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
    dataPoolLock.lock();
    if(dataPool.size() > 0){
        tmp = dataPool.at(0);
        dataPool.pop_front();
    }
    dataPoolLock.unlock();
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
    else if(protocolType == CSV){
        QByteArrayList numberList = pack.split(',');
        foreach(QByteArray num, numberList)
        {
            rowData << num.toDouble();
        }
    }

    return rowData;
}

inline void DataProtocol::addToDataPool(RowData_t &rowData, bool enableSumCheck=false)
{
    #define SUPER_MIN_VALUE 0.000001
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
        if(abs(sum-lastValue) > SUPER_MIN_VALUE)
        {
            qDebug()<<"addToDataPool sum check error"<<rowData;
            return;
        }
        rowData.pop_back();
    }
    dataPoolLock.lock();
    dataPool << rowData;
    dataPoolLock.unlock();
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
