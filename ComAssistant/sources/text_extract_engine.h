#ifndef TEXT_EXTRACT_ENGINE_H
#define TEXT_EXTRACT_ENGINE_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QDebug>
#include <QThread>
#include <QString>
#include <QByteArray>
#include <QRegularExpression>

/*
 * TODO:
 * 1. clear支持指定按name清除
 * 2. 支持按name保存为文件功能
 * 3. Vector显示到textPlainEdit控件效率如何？要不要改为QByteArray
 * 4. 绘图器解析是不是有缺陷？
 */

class textGroup_t
{
public:
    QString     name;
    QVector<QByteArray>  textBuff;
    uint32_t     showIndex;
};

class rawData_t
{
public:
    QByteArray  buff;
    uint32_t    parseIndex;
};

class TextExtractEngine : public QObject
{
    Q_OBJECT
public:
    explicit TextExtractEngine(QObject *parent = nullptr);
    ~TextExtractEngine();

public slots:
    void appendData(const QString &newData);
    void clearData(void);

signals:
    void textGroupsUpdate(const QByteArray &name, const QByteArray &data);

private:
    //数据包的前缀、分隔符、后缀
    const QString PACK_PREFIX   = "{";
    const QString PACK_SEPARATE = ":";
    const QString PACK_SUFFIX   = "}";
    //数据包的前缀、名字、分隔符、数据、后缀、尾巴的正则表达式
    const QString PACK_PREFIX_REG   = "\\{";
    const QString PACK_NAME_REG     = "\\w+";
    const QString PACK_SEPARATE_REG = ":";
    const QString PACK_DATA_REG     = "[\\w`~!@#\\$%\\^&*\\(\\)=+-\\[\\]:;'\"<>,\\.\\?/\\\\| ]+";
    const QString PACK_SUFFIX_REG   = "\\}";
    const QString PACK_TAIL_REG     = "\r?\n";

    void inline appendPackDataToTextGroups(QByteArray &name, QByteArray &data, QVector<textGroup_t> &textGroups);
    bool inline parseNameAndDataFromPack(QByteArray &pack);
    void parsePacksFromBuffer(QByteArray &buffer, QByteArray &restBuffer);
    QVector<textGroup_t> textGroups;  //classified text groups: one tab page one group
    rawData_t rawData;
};

#endif // TEXT_EXTRACT_ENGINE_H
