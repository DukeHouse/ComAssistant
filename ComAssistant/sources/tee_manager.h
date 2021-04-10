/**
 * @brief   数据分窗管理
 * @file    tee_manager.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-2月-13
 * @note
 */
#ifndef TEE_MANAGER_H
#define TEE_MANAGER_H

#include <QObject>
#include <QDebug>
#include <QString>
#include <QPlainTextEdit>
#include <QMap>
#include <QVector>
#include "common.h"
#include <QMutex>

/**
 * @brief     数据分窗管理类
 * @note      可以集中管理分窗窗口和缓冲
 */
class TeeManager : public QObject
{
    Q_OBJECT
public:

public:
    explicit TeeManager(QObject *parent = nullptr);
    ~TeeManager();
    int32_t addTeeBrowser(QString teeBrowserTitle, QPlainTextEdit* teeBrowser);
    int32_t removeTeeBrowser(QString teeBrowserTitle);
    QPlainTextEdit* selectTeeBrowser(QString teeBrowserTitle);
    int32_t appendTeeBrowserBuffer(QString teeBrowserTitle, const QByteArray &buffer);
    int32_t clearTeeBrowserBuffer(QString teeBrowserTitle);
    int32_t clearAllTeeBrowserBuffer();
    int32_t updateTeeBrowserText(QString teeBrowserTitle);
    int32_t updateAllTeeBrowserText();
    int32_t updateAllTeeBrowserFont(QFont font);
    int32_t updateAllTeeBrowserBackground(QColor itsColor);
    QVector<QPlainTextEdit*> getAllTeeBrowsers();
public slots:
private:
    typedef struct TeeBrowserObj_s
    {
        QPlainTextEdit *browser;
        QByteArray buffer;
        QMutex bufferLock;
    }TeeBrowserObj_t;

    QMap<QString, TeeBrowserObj_t*> teeBrowserMap;
};

#endif // TEE_MANAGER_H
