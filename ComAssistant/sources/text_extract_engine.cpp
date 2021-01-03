#include "text_extract_engine.h"

TextExtractEngine::TextExtractEngine(QObject *parent) : QObject(parent)
{
    // this run in main thread.
//    qDebug()<<"hello thread";
}

TextExtractEngine::~TextExtractEngine()
{
    // this run in main thread.
//    qDebug()<<"goodbye thread";
}

void TextExtractEngine::setLevel2NameSupport(bool enable)
{
    enableLevel2NameSupport = enable;
}

bool TextExtractEngine::getLevel2NameSupport(void)
{
    return enableLevel2NameSupport;
}

//把包数据根据名字加到文本组集合中，如果集合中没有对应名字，则在集合中新增一个文本组
void inline TextExtractEngine::appendPackDataToTextGroups(QByteArray& name, QByteArray& data, QByteArray& pack)
{
    int32_t nameIndex = 0;

    //按名字寻找文本组
    QVector<textGroup_t>::iterator it = textGroups.begin();
    for(; it != textGroups.end(); it++){
        if(it->name == name){
            it->dataBuff.append(data + "\n");
            it->packBuff.append(pack);
            emit textGroupsUpdate(name, data);
            return;
        }
        nameIndex++;
    }

    textGroup_t newTextGroup;
    newTextGroup.name = name;
    newTextGroup.dataBuff.append(data + "\n");
    newTextGroup.packBuff.append(pack);
    textGroups.append(newTextGroup);

    emit textGroupsUpdate(name, data);
    return;
}

//从包数据中提取名字（支持二级名字（数据段中的第一组{name}））、数据、时间戳，若名字提取失败则不再提取数据
bool inline TextExtractEngine::parseNameAndDataFromPack(QByteArray& pack, bool enable_name2)
{

    if(pack.isEmpty())
        return false;

    QByteArray name;
    QByteArray name2;
    QByteArray data;

    //匹配{name:data}风格的数据中的name。
    name = pack;
    name = name.mid(0, name.indexOf(PACK_SEPARATE));
    name = name.remove(name.indexOf(PACK_PREFIX), PACK_PREFIX.size());
    if(name.isEmpty())
        return false;

    //匹配{name:data}风格的数据中的data。
    data = pack;
    data.remove(0, PACK_PREFIX.size()+name.size()+PACK_SEPARATE.size());
    //如果使能并发现二级名字
    if(enable_name2 && data.indexOf('{') != -1 && data.indexOf('}') != -1)
    {
        QByteArray pre; //INFO,ERR等前缀
        pre = data.mid(0, data.indexOf('{'));
        name2 = data.mid(data.indexOf('{'), data.indexOf('}') + 1 - pre.size());
        name += name2;
        data.remove(0, data.indexOf('}') + 1);
        data = pre + data;
    }
    data = data.mid(0, data.lastIndexOf(PACK_SUFFIX));

    appendPackDataToTextGroups(name, data, pack);

//    qDebug()<<"name:"<<name<<"data:"<<data;
    return true;
}

//从缓存中提取所有包，每提取出一个包就解析一个
void TextExtractEngine::parsePacksFromBuffer(QByteArray& buffer, QByteArray& restBuffer, QMutex &bufferLock)
{
    if(buffer.isEmpty())
        return;

//    buffer = "{name:val0";
//    buffer = "{name:val0{name:val1}\n";
//    buffer = "{name:val0{name:val1}\n{name:val2";

    QRegularExpression reg;
    QRegularExpressionMatch match;
    QString pattern;
    int scanIndex = 0;
    int lastScannedIndex = 0;
    //匹配{name:data}\r\n风格的数据。
    //name可以是字母、数字、下划线
    //data可以是键盘上除左右花括号以外的所有可见字符
    pattern = PACK_PREFIX_REG + PACK_NAME_REG + PACK_SEPARATE_REG + PACK_DATA_REG + PACK_SUFFIX_REG + PACK_TAIL_REG;
    reg.setPattern(pattern);
    reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
    do {
            QByteArray onePack;
            match = reg.match(buffer, scanIndex);
            if(match.hasMatch()) {
                scanIndex = match.capturedEnd();
                lastScannedIndex = scanIndex;
                onePack.clear();
                onePack.append(match.captured(0).toLocal8Bit());
                parseNameAndDataFromPack(onePack, enableLevel2NameSupport);
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
}

void TextExtractEngine::appendData(const QByteArray &newData)
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
    rawData.buff.append(tmp);
    dataLock.unlock();
}

void TextExtractEngine::parseData()
{
    parsePacksFromBuffer(rawData.buff, rawData.buff, dataLock);
    //在解析完成后剔除前面已扫描过的数据
    if(rawData.buff.size() > MAX_EXTRACT_LENGTH)
    {
        dataLock.lock();
        rawData.buff = rawData.buff.mid(rawData.buff.size() - MAX_EXTRACT_LENGTH);
        dataLock.unlock();
    }
}

void TextExtractEngine::appendAndParseData(const QByteArray &newData)
{
    appendData(newData);
    parseData();
//    qDebug()<<"ThreadID:"<<QThread::currentThreadId()<<"size:"<<rawData.buff.size();
}

//从文本组集合中清除指定名称的文本组
void TextExtractEngine::clearData(const QString &name)
{
    rawData.buff.clear();

    qint32 i = 0;
    QVector<textGroup_t>::iterator it = textGroups.begin();
    for(; it != textGroups.end(); it++){
        if(it->name == name){
            textGroups.remove(i);
            break;//删除后记得跳出来
        }
        i++;
    }

//    qDebug()<<"clearData"<<rawData.buff.size();
}

//保存指定名字的文本组到指定路径
qint32 TextExtractEngine::saveData(const QString &path, const QString &name, const bool& savePackBuff)
{
    QVector<textGroup_t>::iterator it = textGroups.begin();
    for(; it != textGroups.end(); it++){
        if(it->name == name){
            //保存数据
            QFile file(path);
            //删除旧数据形式写文件
            if(file.open(QFile::WriteOnly|QFile::Truncate)){
                if(savePackBuff)
                    file.write(it->packBuff);//不用DataStream写入非文本文件，它会额外添加4个字节作为文件头
                else
                    file.write(it->dataBuff);//不用DataStream写入非文本文件，它会额外添加4个字节作为文件头
                file.flush();
                file.close();
                emit saveDataResult(SAVE_OK, path, file.size());
                return SAVE_OK;
            }else{
                emit saveDataResult(OPEN_FAILED, path, 0);
                return OPEN_FAILED;
            }
        }
    }
    emit saveDataResult(UNKNOW_NAME, path, 0);
    return UNKNOW_NAME;
}
