/**
 * @brief   asciiMatch功能的实现
 * @file    reg_match_engine.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-2月-11
 * @note    负责从数据流中提取包含指定字符串的数据
 */
#ifndef REG_MATCH_ENGINE_H
#define REG_MATCH_ENGINE_H

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
#include <QPlainTextEdit>
#include <QTextCodec>

/**
 * @brief     正则匹配引擎
 * @note      按照设定的匹配模式匹配数据流
 */
class RegMatchEngine : public QObject
{
    Q_OBJECT
public:
    #define MAX_EXTRACT_LENGTH 512
    enum SaveDataResult{
        UNKNOW_NAME = -2,
        OPEN_FAILED = -1,
        SAVE_OK = 0,
    };

    explicit RegMatchEngine(QObject *parent = nullptr);
    ~RegMatchEngine();
    /**
     * @brief     设定匹配模式字符串
     * @note      不支持中文
     * @param[in] 新的匹配模式
     */
    void updateRegMatch(QString newStr, bool clearFlag = true);
    //已弃用
    void updateCodec(QString codec);

public slots:
    void appendData(const QByteArray &newData);
    void parseData();
    void appendAndParseData(const QByteArray &newData);
    void clearData();
    qint32 saveData(const QString &path);
signals:
    /**
     * @brief      数据已更新（解析出一组数据）
     * @note       高频接收会有高频信号，建议放到缓冲中周期上屏显示，直接上屏很耗资源
     * @param[out] 解析出的数据包
     */
    void dataUpdated(const QByteArray &packData);
    /**
     * @brief      保存数据的结果
     * @param[out] 结果
     * @param[out] 路径
     * @param[out] 文件大小
     */
    void saveDataResult(const qint32& result, const QString &path, const qint32 fileSize);

private:
    bool parsingFlag;
    QString RegMatchStr;
    void parsePacksFromBuffer(QByteArray &buffer, QByteArray &restBuffer, QMutex &bufferLock);
    QByteArray  rawDataBuff;
    QByteArray  matchedDataPool;
    QMutex dataLock;
};

#endif // REG_MATCH_ENGINE_H
