#include "dataprotocol.h"

DataProtocol::DataProtocol(QObject *parent) : QObject(parent)
{
    MAXDATA_AS_END.append(static_cast<char>(0x00));
    MAXDATA_AS_END.append(static_cast<char>(0x00));
    MAXDATA_AS_END.append(static_cast<char>(0x80));
    MAXDATA_AS_END.append(static_cast<char>(0x7F));
    QString empty;
    clearBuff(empty);
}

DataProtocol::~DataProtocol()
{
}

/**
 * @brief     设置协议类型
 * @param[in] 协议类型
 * @param[in] 是否清空缓冲
 */
void DataProtocol::setProtocolType(ProtocolType_e type, bool clearbuff)
{
    protocolType = type;
    if(clearbuff)
    {
        QString empty;
        clearBuff(empty);
    }
}

/**
 * @brief     获取协议类型
 * @return    返回的协议类型
 */
DataProtocol::ProtocolType_e DataProtocol::getProtocolType()
{
    return protocolType;
}

/**
 * @brief     获取默认标题
 * @return    默认标题
 */
QString DataProtocol::getDefaultPlotterTitle()
{
    return defaultPlotTitle;
}

/**
 * @brief     设置默认标题
 * @param[in] 新的标题
 * @return    错误码
 */
int32_t DataProtocol::setDefaultPlotterTitle(QString title)
{
    defaultPlotTitle = title;
    return 0;
}

/**
 * @brief     往协议（解析器）中追加数据
 * @param[in] 新的数据
 */
void DataProtocol::appendData(const QByteArray &data)
{
    QByteArray tmp;
    if(protocolType != Float)
    {
        //剔除正则匹配无法很好支持的字符：中文换空格、'\0'删除
        foreach(char ch, data)
        {
            if(ch & 0x80 || ch == '\0')
            {
                if(ch & 0x80)
                    ch = ' ';
                else
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

/**
 * @brief     解析协议（解析器）中的数据
 * @param[in] 是否需要和校验
 */
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

/**
 * @brief     ASCII协议是否存在错误字符串
 * @param[in] 被检测的字符串
 * @return    检测结果
 */
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

/**
 * @brief      CSV协议的数据包修饰
 * @note       见函数内部注释
 * @param[in]  要修饰的数据包
 * @param[out] 修饰后的数据包
 */
QByteArray& DataProtocol::packetDecorationOfCSV(QByteArray& onePack)
{
    //剔除\r\n并把连续空格替换为单个空格
    onePack = onePack.simplified();

    //把", "或者" ,"替换为","
    while(onePack.indexOf(", ") != -1)
    {
        onePack.replace(onePack.indexOf(", "), 2, ",");
    }
    while(onePack.indexOf(" ,") != -1)
    {
        onePack.replace(onePack.indexOf(" ,"), 2, ",");
    }
    //把剩余空格替换为逗号
    while(onePack.indexOf(" ") != -1)
    {
        onePack.replace(onePack.indexOf(" "), 1, ",");
    }
    //连续逗号之间补0
    while(onePack.indexOf(",,") != -1)
    {
        onePack.insert(onePack.indexOf(",,") + 1,'0');
    }
    //以逗号开头或结尾则在开头或结尾补0
    if(onePack.startsWith(','))
    {
        onePack.push_front('0');
    }
    if(onePack.endsWith(','))
    {
        onePack.remove(onePack.size() - 1, 1);
    }
    return onePack;
}

/**
 * @brief      从缓冲中解析数据包
 * @note       根据不同协议使用不同解析方法，解析出数据包后会马上解析数据值
 * @param[in]  输入缓冲
 * @param[out] 剩余缓冲
 * @param[in]  缓冲锁
 * @param[in]  是否使能和校验
 */
inline void DataProtocol::parsePacksFromBuffer(QByteArray& buffer, QByteArray& restBuffer,
                                               QMutex &bufferLock, bool enableSumCheck)
{
    //坑：如果一串数据前面的字符都匹配上了，最后几个字符没匹配上会消耗大量的时间
    //或者说在大量匹配数据中穿插一组连续的不匹配数据，此时由于需要scanIndex遍历也会异常耗时
    //主要耗时点为match函数和scanIndex+1遍历，导致卡顿。刻意制造这种数据会使处理效率剧烈降低
    //目前解决办法是限制scanIndex遍历长度为512
    if(buffer.isEmpty())
        return;
    if(protocolType == Ascii)
    {
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
                    if(onePack.startsWith("{:"))
                    {
                        onePack.insert(1, defaultPlotTitle);
                    }
                    if(!onePack.isEmpty()){
                        RowData_t data;
                        QString name;
                        extractRowData(onePack, name, data);
                        addToDataPool(name, data, enableSumCheck);
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
    else if(protocolType == Float)
    {
        QByteArray tmpArray = buffer;
        while (tmpArray.indexOf(MAXDATA_AS_END) != -1)
        {
            QByteArray before = tmpArray.mid(0,tmpArray.indexOf(MAXDATA_AS_END));
            tmpArray = tmpArray.mid(tmpArray.indexOf(MAXDATA_AS_END)+MAXDATA_AS_END.size());
            if(before.size() % 4 == 0)
            {
                RowData_t data;
                QString name;
                extractRowData(before, name, data);
                addToDataPool(name, data, enableSumCheck);
            }
            else
                qDebug()<<"丢弃数据（长度不是4的倍数）："<<before.toHex().toUpper();
        }
        restBuffer = tmpArray;
    }
    else if(protocolType == CSV)
    {
        bufferLock.lock();
        //删除\0
        while (buffer.indexOf('\0') != -1)
        {
            buffer.remove(buffer.indexOf('\0'), 1);
        }
        //替换所有非数字和符号的数据为空格
        for(int32_t i = 0; i < buffer.size(); i++)
        {
            if(buffer[i] < '0' || buffer[i] > '9')
            {
                if(buffer[i] != '-'
                && buffer[i] != '+'
                && buffer[i] != ','
                && buffer[i] != '.'
                && buffer[i] != '\r'
                && buffer[i] != '\n')
                {
                    buffer[i] = ' ';
                }
            }
        }
        bufferLock.unlock();
        QRegularExpression reg;
        QRegularExpressionMatch match;
        int scanIndex = 0;
        int lastScannedIndex = 0;
        /* 逗号分隔符格式
         * 符号有0到1个，整数必须有，小数部分有0到1个
         * 以上作为一个组，组前可以有空格，组后可以有逗号
         * 末尾要有换行符
        */
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
                        qDebug() << "hasErrorStr_CSV" << onePack;
                        continue;
                    }
                    onePack = packetDecorationOfCSV(onePack);
                    if(!onePack.isEmpty())
                    {
                        RowData_t data;
                        QString name;
                        extractRowData(onePack, name, data);
                        addToDataPool(name, data, enableSumCheck);
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
    else if(protocolType == MAD)
    {
        QRegularExpression reg;
        QRegularExpressionMatch match;
        int scanIndex = 0;
        int lastScannedIndex = 0;
        //逗号分隔符格式
        reg.setPattern(".*\n");
        reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
        do {
                QByteArray onePack;
                match = reg.match(buffer, scanIndex);
                if(match.hasMatch())
                {
                    scanIndex = match.capturedEnd();
                    lastScannedIndex = scanIndex;
                    onePack.clear();
                    onePack.append(match.captured(0).toLocal8Bit());
                    if(!onePack.isEmpty())
                    {
                        RowData_t data;
                        QString name;
                        extractRowData(onePack, name, data);
                        addToDataPool(name, data, enableSumCheck);
                    }
//                    qDebug()<<"match"<<match.captured(0);
                }
                else
                {
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

/**
 * @brief     清空缓冲
 * @param[in] 要清空的缓冲名称
 */
void DataProtocol::clearBuff(const QString &name)
{

    if(name.isEmpty())
    {
        hasParsedBuffCnt = 0;
        tempDataPoolLock.lock();
        tempDataPool.clear();
        tempDataPoolLock.unlock();
        dataPoolLock.lock();
        for(int32_t i = 0; i < dataPoolSets.size(); i++)
        {
            dataPoolSets[i].dataPool.clear();
            // dataPoolSets.remove(i);//这里暂时是只清Pool不移除对象
        }
        dataPoolLock.unlock();
        return;
    }

    dataPoolLock.lock();
    for(int32_t i = 0; i < dataPoolSets.size(); i++)
    {
        if(dataPoolSets.at(i).name == name)
        {
            hasParsedBuffCnt -= dataPoolSets[i].dataPool.size();
            dataPoolSets[i].dataPool.clear();
            // dataPoolSets.remove(i);
            break;
        }
    }
    dataPoolLock.unlock();
}

/**
 * @brief     是否存在已解析完成的数据（在数据池中）
 * @return    返回结果
 */
int32_t DataProtocol::hasParsedBuff()
{
    return hasParsedBuffCnt;
}

/**
 * @brief      从数据池中弹出一行数据
 * @note
 * @param[in]  要弹出的数据所属的名称
 * @param[out] 弹出的数据
 * @return     无用
 */
int32_t DataProtocol::popOneRowData(QString &name, QVector<double> &rowData)
{
    dataPoolLock.lock();
    for(int32_t i = 0; i < dataPoolSets.size(); i++)
    {
        if(dataPoolSets.at(i).dataPool.size())
        {
            name = dataPoolSets.at(i).name;
            rowData = dataPoolSets[i].dataPool.at(0);
            dataPoolSets[i].dataPool.pop_front();
            hasParsedBuffCnt--;
            break;
        }
    }
    dataPoolLock.unlock();
    return 0;
}

/**
 * @brief      （从数据包中）提取（解析）一行数据
 * @note
 * @param[in]  要被提取的数据包
 * @param[out] 提取出的数据包的名称
 * @param[out] 提取出的数据包的数据
 * @return     提取出的数据包的数据
 */
inline DataProtocol::RowData_t
DataProtocol::extractRowData(const Pack_t &pack, QString &outName, RowData_t &outRowData)
{
    QString namePack = defaultPlotTitle;
    RowData_t rowData;
    Pack_t dataPack;

    if(pack.isEmpty())
    {
        outName = namePack;
        outRowData = rowData;
        return rowData;
    }

    if(protocolType == Ascii)
    {
        //把数据部分提取出来
        namePack = pack.mid(pack.indexOf('{') + 1, pack.indexOf(':') - 1);
        dataPack = pack.mid(pack.indexOf(':'));

        QRegularExpression reg;
        QRegularExpressionMatch match;
        int index = 0;
        reg.setPattern("[\\+-]?\\d+\\.?\\d*");//匹配实数 符号出现0、1次，数字至少1次，小数点0、1次，小数不出现或出现多次
        do {
                match = reg.match(dataPack, index);
                if(match.hasMatch())
                {
                    index = match.capturedEnd();
                    rowData << match.captured(0).toDouble();
    //                qDebug()<<match.captured(0).toDouble();
                }
                else
                {
    //                qDebug()<<"no match";
                    break;
                }
        } while(index < dataPack.length());
    }
    else if(protocolType == Float)
    {
        dataPack = pack;
        while (dataPack.size() > 0)
        {
            float tmp;
            if(packToFloat(dataPack, tmp))
            {
                dataPack = dataPack.mid(4);
                rowData << static_cast<double>(tmp);
//                qDebug("tmp:%f",tmp);
            }
            else
            {
//                qDebug()<<"get max";
            }
        }
    }
    else if(protocolType == CSV)
    {
        QByteArrayList numberList = pack.split(',');
        foreach(QByteArray num, numberList)
        {
            rowData << num.toDouble();
        }
    }
    else if(protocolType == MAD)
    {
        //把数据部分提取出来
        Pack_t dataPack = pack;

        QRegularExpression reg;
        QRegularExpressionMatch match;
        int index = 0;
        reg.setPattern("[\\+-]?\\d+\\.?\\d*");//匹配实数 符号出现0、1次，数字至少1次，小数点0、1次，小数不出现或出现多次
        do {
                match = reg.match(dataPack, index);
                if(match.hasMatch())
                {
                    index = match.capturedEnd();
                    rowData << match.captured(0).toDouble();
    //                qDebug()<<match.captured(0).toDouble();
                }
                else
                {
    //                qDebug()<<"no match";
                    break;
                }
        } while(index < dataPack.length());
    }

    outName = namePack;
    outRowData = rowData;

    return rowData;
}

/**
 * @brief     把（提取出的）数据加入数据池中
 * @param[in] 数据所属的名称
 * @param[in] 数据
 * @param[in] 是否使能和校验
 */
inline void DataProtocol::addToDataPool(QString &name, RowData_t &rowData, bool enableSumCheck=false)
{
    #define SUPER_MIN_VALUE 0.000001
    if(rowData.size() <= 0 || name.isEmpty())
        return;

    if(enableSumCheck)
    {
        //和校验模式 必须 要有至少两个数据
        if(rowData.size() <= 1)
            return;
        float lastValue = rowData.at(rowData.size() - 1); //要用float型的，double太过精确导致数据不一致
        float sum = 0;
        for(int i = 0; i < rowData.size()-1; i++)
            sum += rowData.at(i);
        if(fabs(sum-lastValue) > SUPER_MIN_VALUE)
        {
            qDebug() << "addToDataPool sum check error" << rowData;
            return;
        }
        rowData.pop_back();
    }
    //把数据加入到对应name的数据池中
    uint8_t finded = 0;
    dataPoolLock.lock();
    for(int32_t i = 0; i < dataPoolSets.size(); i++)
    {
        if(dataPoolSets.at(i).name == name)
        {
            dataPoolSets[i].dataPool << rowData;
            hasParsedBuffCnt ++;
            finded = 1;
            break;
        }
    }
    dataPoolLock.unlock();

    if(!finded)
    {
        dataPoolLock.lock();
        if(dataPoolSets.size() < 15)
        {
            DataPoolGroup_t dataPool;
            dataPool.name = name;
            dataPool.dataPool << rowData;
            hasParsedBuffCnt ++;
            dataPoolSets.append(dataPool);
        }
        else
        {
            qDebug() << "dataPoolSets size out of limits. new name:" << name;
        }
        dataPoolLock.unlock();
    }
}

/**
 * @brief      字节数组强转Float型数据
 * @note
 * @param[in]  要转换的字节数组
 * @param[out] 转换结果
 * @return     是否转换成功
 */
bool DataProtocol::byteArrayToFloat(const QByteArray& array, float& result)
{
    if(array.size() < 4)
        return false;

    char num[4];
    for(int i = 0; i < 4; i++)
        num[i] = array.at(i);//
//    qDebug("%.2f", *(reinterpret_cast<float*>(num)));

    result = *(reinterpret_cast<float*>(num));
    return true;
}

/**
 * @brief      数据包转换为Float型数据
 * @note       若一个数据包中有多个float型数据，则返回第一个
 * @param[in]  要转换的数据包
 * @param[out] 转换结果
 * @return     是否转换成功
 */
bool DataProtocol::packToFloat(const Pack_t& pack , float& result)
{
    return byteArrayToFloat(pack, result);
}
