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
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

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
    QByteArray  dataBuff;
    QByteArray  packBuff;
};

class rawData_t
{
public:
    QByteArray  buff;
};

class TextExtractEngine : public QObject
{
    Q_OBJECT
public:
    enum SaveDataResult{
        UNKNOW_NAME = -2,
        OPEN_FAILED = -1,
        SAVE_OK = 0,
    };

    explicit TextExtractEngine(QObject *parent = nullptr);
    ~TextExtractEngine();

public slots:
    void appendData(const QString &newData);
    void clearData(const QString &name);
    qint32 saveData(const QString &path, const QString &name, const bool& savePackBuff);
    void parseData();
signals:
    void textGroupsUpdate(const QByteArray &name, const QByteArray &data);
    void saveDataResult(const qint32& result, const QString &path, const qint32 fileSize);

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

    void inline appendPackDataToTextGroups(QByteArray &name, QByteArray &data,  QByteArray& pack);
    bool inline parseNameAndDataFromPack(QByteArray &pack);
    void parsePacksFromBuffer(QByteArray &buffer, QByteArray &restBuffer);
    bool needParse = false;
    QVector<textGroup_t> textGroups;  //classified text groups: one tab page one group
    rawData_t rawData;
};

#endif // TEXT_EXTRACT_ENGINE_H
