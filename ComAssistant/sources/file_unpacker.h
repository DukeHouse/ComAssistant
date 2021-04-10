/**
 * @brief   文件解包器，设置路径后自动将文件解包并通过信号发到绑定的槽上
 * @file    file_unpacker.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-4月-11
 * @note    每发完一个包必须收到应答才会发送下一个包
 */
#ifndef FILEUNPACKER_H
#define FILEUNPACKER_H

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QFile>

typedef enum {
    IDLE,
    PREPARE,
    UNPACKING,
    FINISH
}UnpackStatus_e;

typedef enum {
    FAILED,
    OPEN_FAILED,
    ABORTED,
    SUCCESS,
}UnpackResult_e;

class FileUnpacker : public QThread
{
    Q_OBJECT

public:
    FileUnpacker(QObject *parent = nullptr);
    void pause_thread();
    void continue_thread();
    void stop_thread();
    int32_t unpack_file(QString path,
                    bool deleteIfUnpackSuccess = false,
                    uint32_t unpack_size = 4096);
    void abort_unpack_file();
    void unpack_ack();
signals:
    void newPack(const QByteArray &pack, qint32 current_cnt, qint32 total_cnt);
    void unpackResult(bool success, QString details);
private:
    int32_t __prepare();
    int32_t __unpacking();
    QString __updateResultDetails();
    UnpackStatus_e status = UnpackStatus_e::IDLE;

    bool wait_unpack_ack = false;
    bool abort_unpack = false;
    bool deleteIfSucess = false;

    UnpackResult_e result;

    bool thread_stop = false;
    bool thread_pause = false;

    int32_t pack_size = 0;
    int32_t buff_index = 0;
    QString filePath;
    QByteArray fileBuff;
protected:
    void run();  //线程任务
};

#endif // FILEUNPACKER_H
