#include "reg_match_engine.h"

RegMatchEngine::RegMatchEngine(QObject *parent)
    : QObject(parent)
{
    // this run in main thread.
//    qDebug()<<"hello thread";
}

RegMatchEngine::~RegMatchEngine()
{
    // this run in main thread.
//    qDebug()<<"goodbye thread";
}

/**
 * @brief     更新匹配字符串
 * @note
 * @param[in] 新的匹配字符串
 * @param[in] 是否清空缓冲
 * @return
*/
void RegMatchEngine::updateRegMatch(QString newStr, bool clearFlag)
{
    if(newStr != RegMatchStr)
    {
        if(clearFlag)
            clearData();
        RegMatchStr = newStr;
    }
}

//好像没啥用
void RegMatchEngine::updateCodec(QString codec)
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName(codec.toLocal8Bit()));
}

/**
 * @brief 从数据流缓冲中提取所有包
 * @note
 * @param[in] 输入缓冲
 * @param[in] 剩余未解析缓冲
 * @param[in] 缓冲锁
 * @return
*/
void RegMatchEngine::parsePacksFromBuffer(QByteArray& buffer, QByteArray& restBuffer, QMutex &bufferLock)
{
    parsingFlag = true;

    if(buffer.isEmpty() || buffer == "\n")
        return;
    if(RegMatchStr.isEmpty())
        return;

    QString pattern = "\n.*" + RegMatchStr + ".*\n";
    QRegularExpression reg;
    QRegularExpressionMatch match;
    int scanIndex = 0;
    int lastScannedIndex = 0;
    reg.setPattern(pattern.toLocal8Bit());
    reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
    do {
            //若执行了清空函数则会重置parsingFlag，此时不再解析并清空buffer
            if(!parsingFlag)
            {
                qDebug() << __FUNCTION__ << "detect exit flag";
                clearData();
                return;
            }
            QByteArray onePack;
            match = reg.match(buffer, scanIndex);
            if(match.hasMatch()) {
                //-1是因为匹配借用了前面数据包结尾的换行符\n，所以保留最后的换行符供下一个数据包借用
                scanIndex = match.capturedEnd() - 1;
                lastScannedIndex = scanIndex;
                onePack = match.captured(0).toLocal8Bit();
                onePack.remove(0, 1); //remove first '\n', last
                matchedDataPool.append(onePack);
                onePack.remove(onePack.size()-1, 1);//remove last '\r\n'
                if(onePack.endsWith("\r"))
                {
                    onePack.remove(onePack.size()-1, 1);
                }
                emit dataUpdated(onePack);
            }else{
//                qDebug()<<"no match";
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

    parsingFlag = false;
}


/**
 * @brief 追加数据
 * @note 仅追加数据，不进行解析
 * @param[in] 新的数据
*/
void RegMatchEngine::appendData(const QByteArray &newData)
{
    QByteArray tmp;
    //剔除正则匹配无法很好支持的字符：中文、'\0'
    foreach(char ch, newData)
    {
        if(ch & 0x80 || ch == '\0')
        {
            continue;
        }
        tmp.append(ch);
    }
    dataLock.lock();
    //由于匹配借用了前面数据包结尾的换行符，因此首个数据包要在前面补\n
    if(rawDataBuff.isEmpty())
        rawDataBuff = "\n";
    rawDataBuff.append(tmp);
    dataLock.unlock();
}

/**
 * @brief 解析数据
 * @note 仅解析数据
*/
void RegMatchEngine::parseData()
{
    parsePacksFromBuffer(rawDataBuff, rawDataBuff, dataLock);
    //在解析完成后剔除前面已扫描过的数据
    if(rawDataBuff.size() > MAX_EXTRACT_LENGTH)
    {
        dataLock.lock();
        rawDataBuff = rawDataBuff.mid(rawDataBuff.size() - MAX_EXTRACT_LENGTH);
        dataLock.unlock();
    }
}

/**
 * @brief 追加和解析数据
 * @note 高频解析好像有性能问题不建议使用
 * @deprecated 高频解析好像有性能问题不建议使用
*/
void RegMatchEngine::appendAndParseData(const QByteArray &newData)
{
    appendData(newData);
    parseData();
}

/**
 * @brief 清空缓冲
 * @note 清空后会故意保留一个换行符
*/
void RegMatchEngine::clearData()
{
    //若正在解析，则重置解析标志，由解析函数负责清空缓冲
    if(parsingFlag)
    {
        parsingFlag = false;
        return;
    }

    rawDataBuff.clear();
    matchedDataPool.clear();
    //由于匹配借用了前面数据包结尾的换行符，因此这里也要保留最后的换行符供下一个数据包借用
    rawDataBuff = "\n";
}

/**
 * @brief 保存匹配到的数据到指定路径中
 * @param[in] 指定路径
*/
qint32 RegMatchEngine::saveData(const QString &path)
{
    //保存数据
    QFile file(path);
    //删除旧数据形式写文件
    if(file.open(QFile::WriteOnly|QFile::Truncate)){
        file.write(matchedDataPool);//不用DataStream写入非文本文件，它会额外添加4个字节作为文件头
        file.flush();
        file.close();
        emit saveDataResult(SAVE_OK, path, file.size());
        return SAVE_OK;
    }else{
        emit saveDataResult(OPEN_FAILED, path, 0);
        return OPEN_FAILED;
    }
    emit saveDataResult(UNKNOW_NAME, path, 0);
    return UNKNOW_NAME;
}
