#include "file_unpacker.h"

FileUnpacker::FileUnpacker(QObject *parent)
{
    Q_UNUSED(parent)
}

void FileUnpacker::continue_thread()
{
//    thread_pause = false;
}

void FileUnpacker::pause_thread()
{
//    thread_pause = true;
}

void FileUnpacker::stop_thread()
{
//    thread_stop = true;
}

/**
 * @brief     解析文件
 * @note      只设置了文件路径，并测试能否访问
 * @param[in] 文件路径
 * @param[in] 解包大小
 * @param[in] 是否交互式解析（交互式解析会等待对方应答后才进行下一个解包操作）
 * @return    错误码
 */
int32_t FileUnpacker::unpack_file(QString path, bool deleteIfUnpackSuccess, uint32_t unpack_size)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
    {
        return -1;
    }
    file.close();

    if(!unpack_size)
    {
        return -1;
    }
    if(status != IDLE)
    {
        return -1;
    }

    filePath = path;
    deleteIfSucess = deleteIfUnpackSuccess;
    pack_size = unpack_size;
    result = UnpackResult_e::FAILED;
    status = PREPARE;
    return 0;
}

/**
 * @brief     解包应答
 * @note      告诉本文件已经完成上一个包的解析，可以发送下一个包
 */
void FileUnpacker::unpack_ack()
{
    wait_unpack_ack = false;
}

/**
 * @brief     中止解析文件
 */
void FileUnpacker::abort_unpack_file()
{
    abort_unpack = true;
}

/**
 * @brief     准备数据
 * @note      把数据读入缓冲中
 * @return    错误码
 */
int32_t FileUnpacker::__prepare()
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        result = UnpackResult_e::OPEN_FAILED;
        return -1;
    }

    fileBuff = file.readAll();
    buff_index = 0;
    return 0;
}

/**
 * @brief     解包数据
 * @note      把文件拆成若干包发出去，该函数是交互式的，必须要给ack才能继续执行
 * @return    错误码
 */
int32_t FileUnpacker::__unpacking()
{
    int32_t pack_cnt = 0;
    int32_t total_pack_cnt = 0;
    QByteArray tmp;

    abort_unpack = false;
    wait_unpack_ack = false;
    total_pack_cnt = fileBuff.size() / pack_size
                    + ((fileBuff.size() % pack_size) > 0 ? 1 : 0);

    while(!thread_stop)
    {
        if(thread_pause)
        {
            msleep(100);
            continue;
        }
        if(abort_unpack)
        {
            result = UnpackResult_e::ABORTED;
            return -1;
        }
        if(wait_unpack_ack)
        {
            msleep(1);
            continue;
        }
        wait_unpack_ack = true;
        tmp = fileBuff.mid(buff_index, pack_size);
        buff_index += pack_size;
        pack_cnt++;
        emit newPack(tmp, pack_cnt, total_pack_cnt);
        if(pack_cnt == total_pack_cnt)
            break;
    }
    result = UnpackResult_e::SUCCESS;
    return 0;
}

/**
 * @brief     解包数据
 * @note      把文件拆成若干包发出去，该函数是交互式的，必须要给ack才能继续执行
 * @return    错误码
 */
QString FileUnpacker::__updateResultDetails()
{
    QString details;
    switch(result)
    {
    case FAILED:
        details = "unpack failed.";
        break;
    case OPEN_FAILED:
        details = "file open failed.";
        break;
    case ABORTED:
        details = "unpack aborted.";
        break;
    case SUCCESS:
        details = "unpack success.";
        break;
    default:
        details = "unknown unpack result.";
        break;
    }
    return details;
}

//线程主函数
void FileUnpacker::run()
{
    while(!thread_stop)
    {
        if(thread_pause)
        {
            msleep(100);
            continue;
        }
        switch(status)
        {
        case IDLE:
            msleep(100);
            break;
        case PREPARE:
            if(__prepare())
                status = FINISH;
            else
                status = UNPACKING;
            break;
        case UNPACKING:
            __unpacking();
            status = FINISH;
            break;
        case FINISH:
            if(result == SUCCESS)
            {
                if(deleteIfSucess)
                {
                    QFile file(filePath);
                    file.remove();
                }
                emit unpackResult(true, __updateResultDetails());
            }
            else
            {
                emit unpackResult(false, __updateResultDetails());
            }
            status = IDLE;
            break;
        default:
            break;
        }
        msleep(1);
    }
    quit();
}
