/**
 * @brief   数据记录仪处理文件
 * @file    data_logger.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-2月-11
 * @note    以类型为索引把数据放入不同缓冲中，同时周期写入文件
 */
#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <QObject>
#include <QVector>
#include <QDebug>
#include <QThread>
#include <QFile>
#include <QMap>

#include "xlsxdocument.h"

//type
#define     RECOVERY_LOG    0   ///< 存储类型：恢复数据
#define     RAW_DATA_LOG    1   ///< 存储类型：原始数据
#define     GRAPH_DATA_LOG  2   ///< 存储类型：图像数据

/**
 * @brief     文件单元
 */
typedef struct file_unit_s
{
    uint8_t     type;   ///< 存储类型
    QFile       *file;  ///< 文件对象
    QByteArray  buff;   ///< 存储缓冲
}file_unit_t;

/**
 * @brief     数据记录器类
 * @note      初始化后，周期的append和flush即可
 */
class Data_Logger:public QObject
{
    Q_OBJECT
public:
    explicit Data_Logger(QObject *parent = nullptr);
    ~Data_Logger();
public slots:
    /**
     * @brief     初始化（新增）一个记录器
     * @note      记录器是按类型划分的
     * @param[in] 记录类型
     * @param[in] 记录路径
     */
    void init_logger(uint8_t type, QString path);
    /**
     * @brief     追加数据到指定类型的记录器中
     * @param[in] 记录器类型
     * @param[in] 要记录的数据
     */
    void append_data_logger_buff(uint8_t type, const QByteArray &data);
    /**
     * @brief     立即把指定记录器的缓冲写入文件
     * @param[in] 记录器类型
     */
    void logger_buff_flush(uint8_t type);
    /**
     * @brief     清空记录器
     * @param[in] 记录器类型
     * @return
     */
    void clear_logger(uint8_t type);
private:
    QVector<file_unit_t> file_group;    ///< 文件组（记录器组）
    /**
     * @brief     追加到XLSX文件
     * @note      适用于曲线数据的记录
     * @param[in] 追加的数据
     * @param[in] 文件路径
     */
    bool appendToXlsx(const QByteArray &buff, QString path);
signals:
};

#endif // DATA_LOGGER_H
