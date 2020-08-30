#include "text_extract_engine.h"

TextExtractEngine::TextExtractEngine(QObject *parent) : QObject(parent)
{

}

TextExtractEngine::~TextExtractEngine()
{
//    qDebug() << "good bye TextExtractEngine:" << QThread::currentThreadId();
}

void inline TextExtractEngine::appendPackDataToTextGroups(QByteArray& name, QByteArray& data, QVector<textGroup_t>& textGroups)
{
    int32_t nameIndex = 0;

    //search name from TextGroups
    QVector<textGroup_t>::iterator it = textGroups.begin();
    for(; it != textGroups.end(); it++){
        if(it->name == name){
            it->textBuff.append(data);
            emit textGroupsUpdate(name, data);
            return;
        }
        nameIndex++;
    }

    textGroup_t newTextGroup;
    newTextGroup.name = name;
    newTextGroup.textBuff.append(data);
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

    appendPackDataToTextGroups(name, data, textGroups);

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
//    qDebug()<<"rowData:"<<rawData.buff;
    parsePacksFromBuffer(rawData.buff, rawData.buff);
//    qDebug()<<"leftData"<<rawData.buff;
}

void TextExtractEngine::clearData(void)
{
    qDebug()<<"clearData";
    rawData.buff.clear();
    rawData.parseIndex = 0;
}
