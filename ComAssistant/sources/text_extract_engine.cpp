#include "text_extract_engine.h"

TextExtractEngine::TextExtractEngine(QObject *parent) : QObject(parent)
{

}

TextExtractEngine::~TextExtractEngine()
{
//    qDebug() << "good bye TextExtractEngine:" << QThread::currentThreadId();
}

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

bool inline TextExtractEngine::parseNameAndDataFromPack(QByteArray& pack)
{

    if(pack.isEmpty())
        return false;

    QByteArray name;
    QByteArray data;
    bool find_name = 0;
    QRegularExpression reg;
    QRegularExpressionMatch match;
    QString pattern;
    int scanIndex = 0;

    //匹配{name:data}风格的数据中的name。
    scanIndex = 0;
    pattern = PACK_PREFIX_REG + PACK_NAME_REG + PACK_SEPARATE_REG;
    reg.setPattern(pattern);
    reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
    do {
            match = reg.match(pack, scanIndex);
            if(match.hasMatch()) {
                scanIndex = match.capturedEnd();
                name.clear();
                name.append(match.captured(0).toLocal8Bit());
                name.remove(0, PACK_PREFIX.size());
                name.remove(name.size() - PACK_SEPARATE.size(), PACK_SEPARATE.size());
                find_name = 1;
                break;
            }else{
//                qDebug()<<"no match";
                scanIndex++;
            }
    } while(scanIndex < pack.size());

    if(find_name == false)
        return false;

    //匹配{name:data}风格的数据中的data。
    scanIndex = 0;
    pattern = PACK_SEPARATE_REG + PACK_DATA_REG + PACK_SUFFIX_REG;
    reg.setPattern(pattern);
    reg.setPatternOptions(QRegularExpression::InvertedGreedinessOption);//设置为非贪婪模式匹配
    do {
            match = reg.match(pack, scanIndex);
            if(match.hasMatch()) {
                scanIndex = match.capturedEnd();
                data.clear();
                data.append(match.captured(0).toLocal8Bit());
                data.remove(0, PACK_SEPARATE.size());
                data.remove(data.size() - PACK_SUFFIX.size(), PACK_SUFFIX.size());
                break;
            }else{
//                qDebug()<<"no match";
                scanIndex++;
            }
    } while(scanIndex < pack.size());

    appendPackDataToTextGroups(name, data, pack);

//    qDebug()<<"name:"<<name<<"data:"<<data;
    return true;
}

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
                //剔除回车换行
                while (onePack.indexOf('\r') != -1) {
                    onePack.remove(onePack.indexOf('\r'), 1);
                }
                while (onePack.indexOf('\n') != -1) {
                    onePack.remove(onePack.indexOf('\n'), 1);
                }
                parseNameAndDataFromPack(onePack);
            }else{
//                qDebug()<<"no match";
                scanIndex++;
            }
    } while(scanIndex < buffer.size());

    restBuffer = buffer.mid(lastScannedIndex);
}

void TextExtractEngine::appendData(const QString &newData)
{
    rawData.buff.append(newData);
//    qDebug()<<"------------------";
//    qDebug()<<"ThreadID"<<QThread::currentThreadId();
//    qDebug()<<"inData:"<<newData;
//    qDebug()<<"rawData:"<<rawData.buff;
    parsePacksFromBuffer(rawData.buff, rawData.buff);
//    qDebug()<<"leftData"<<rawData.buff;
}

//清除指定名称的数据
void TextExtractEngine::clearData(const QString &name)
{
//    qDebug()<<"clearData";
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
}

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
