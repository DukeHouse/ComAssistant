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

//把包数据根据名字加到文本组集合中，如果集合中没有对应名字，则在集合中新增一个文本组
void inline TextExtractEngine::appendPackDataToTextGroups(QByteArray& name, QByteArray& data, QByteArray& pack)
{
    int32_t nameIndex = 0;

    //按名字寻找文本组
    QVector<textGroup_t>::iterator it = textGroups.begin();
    for(; it != textGroups.end(); it++){
        if(it->name == name){
            it->dataBuff.append(data + "\n");
            it->packBuff.append(pack + "\n");
            emit textGroupsUpdate(name, data);
            return;
        }
        nameIndex++;
    }

    textGroup_t newTextGroup;
    newTextGroup.name = name;
    newTextGroup.dataBuff.append(data + "\n");
    newTextGroup.packBuff.append(pack + "\n");
    textGroups.append(newTextGroup);

    emit textGroupsUpdate(name, data);
    return;
}

//从包数据中提取名字和数据，若名字提取失败则不再提取数据
bool inline TextExtractEngine::parseNameAndDataFromPack(QByteArray& pack)
{
    if(pack.isEmpty())
        return false;

    QByteArray name;
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
    data = data.mid(0, data.lastIndexOf(PACK_SUFFIX));

    appendPackDataToTextGroups(name, data, pack);

//    qDebug()<<"name:"<<name<<"data:"<<data;
    return true;
}

//从缓存中提取所有包，每提取出一个包就解析一个
void TextExtractEngine::parsePacksFromBuffer(QByteArray& buffer, QByteArray& restBuffer)
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
                parseNameAndDataFromPack(onePack);
            }else{
//                qDebug()<<"no match";
                scanIndex++;
            }
    } while(scanIndex < buffer.size());

    restBuffer = buffer.mid(lastScannedIndex);
}

void TextExtractEngine::appendData(const QByteArray &newData)
{
    QByteArray tmp = newData;
    //正则匹配无法处理\0要删去
    while(tmp.indexOf('\0') != -1)
    {
        tmp.remove(tmp.indexOf('\0'), 1);
    }
    rawData.buff.append(tmp);
}

void TextExtractEngine::parseData()
{
    #define MAX_EXTRACT_LENGTH 2048
    parsePacksFromBuffer(rawData.buff, rawData.buff);
    //在解析完成后剔除前面已扫描过的数据
    if(rawData.buff.size() > MAX_EXTRACT_LENGTH)
    {
        rawData.buff = rawData.buff.mid(rawData.buff.size() - MAX_EXTRACT_LENGTH);
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
