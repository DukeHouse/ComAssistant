/**
 * @brief   分窗文本提取引擎
 * @file    text_extract_engine.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-2月-11
 * @note    根据ASCII协议解析数据流并放入不同的缓冲中
 */
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
#include <QMutex>
#include "common.h"

/*
 * TODO:
 * 1. Vector显示到textPlainEdit控件效率如何？要不要改为QByteArray
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
    #define MAX_EXTRACT_LENGTH 512
    enum SaveDataResult{
        UNKNOW_NAME = -2,
        OPEN_FAILED = -1,
        SAVE_OK = 0,
    };

    explicit TextExtractEngine(QObject *parent = nullptr);
    ~TextExtractEngine();
    void setLevel2NameSupport(bool enable);//二级名字过滤开关
    bool getLevel2NameSupport(void);

public slots:
    void appendData(const QByteArray &newData);
    void parseData();
    void appendAndParseData(const QByteArray &newData);
    void clearData(const QString &name);
    qint32 saveData(const QString &path, const QString &name, const bool& savePackBuff);
signals:
    void textGroupsUpdate(const QString &name, const QByteArray &data);
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
    const QString PACK_DATA_REG     = "[\\w`~!@#\\$%\\^&*\\(\\)=+-\\[\\]:;'\"<>,\\.\\?/\\\\| \\s\\{\\}]+";
    const QString PACK_SUFFIX_REG   = "\\}";
    const QString PACK_TAIL_REG     = "\r?\n";

    void inline appendPackDataToTextGroups(QByteArray &name, QByteArray &data,  QByteArray& pack);
    bool inline parseNameAndDataFromPack(QByteArray &pack, bool enableLevel2NameSupport);
    void parsePacksFromBuffer(QByteArray &buffer, QByteArray &restBuffer, QMutex &bufferLock);
    QVector<textGroup_t> textGroups;  //classified text groups: one tab page one group
    rawData_t rawData;
    QMutex dataLock;
    bool enableLevel2NameSupport = 0;
};

#endif // TEXT_EXTRACT_ENGINE_H
