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
    void updateRegMatch(QString newStr);

public slots:
    void appendData(const QByteArray &newData);
    void parseData();
    void appendAndParseData(const QByteArray &newData);
    void clearData();
    qint32 saveData(const QString &path);
signals:
    void dataUpdated(const QString &packData);
    void saveDataResult(const qint32& result, const QString &path, const qint32 fileSize);

private:
    QString RegMatchStr;
    void parsePacksFromBuffer(QByteArray &buffer, QByteArray &restBuffer, QMutex &bufferLock);
    QByteArray  rawDataBuff;
    QByteArray  matchedDataPool;
    QMutex dataLock;
};

#endif // REG_MATCH_ENGINE_H
