#include "data_logger.h"

Data_Logger::Data_Logger(QObject *parent) : QObject(parent)
{
}

Data_Logger::~Data_Logger()
{
    //退出前把数据存下来
    QVector<file_unit_t>::iterator it = file_group.begin();
    for(; it != file_group.end(); it++){
        if(it->buff.size() == 0){
            continue;
        }
        if(!it->file->open(QFile::WriteOnly|QFile::Append))
        {
            qDebug() << it->file->fileName() << "open failed.";
            continue;
        }
        QTextStream stream(it->file);
        QByteArray temp = it->buff;
        it->buff.remove(0, temp.size());
        stream << temp;
        it->file->close();
    }
}

void Data_Logger::init_logger(uint8_t type, QString path)
{
    //初始化文件句柄，如果已经存在则flush并更新文件路径
    QVector<file_unit_t>::iterator it = file_group.begin();
    for(; it != file_group.end(); it++){
        if(it->type != type){
            continue;
        }
        if(it->buff.size() != 0)
        {
            logger_buff_flush(type);
        }
        it->file->setFileName(path);
        return;
    }
    file_group.append(file_unit_t());
    file_group.last().file = new QFile(path);
    file_group.last().type = type;
}

//只加数据不存储
void Data_Logger::append_data_logger_buff(uint8_t type, const QByteArray &data)
{
//    qDebug()<<"ThreadID"<<QThread::currentThreadId();
    QVector<file_unit_t>::iterator it = file_group.begin();
    for(; it != file_group.end(); it++){
        if(it->type == type){
            it->buff.append(data);
            return;
        }
    }
}

//存储数据
void Data_Logger::logger_buff_flush(uint8_t type)
{
//    qDebug()<<"ThreadID"<<QThread::currentThreadId();
    QVector<file_unit_t>::iterator it = file_group.begin();
    for(; it != file_group.end(); it++){
        if(it->type != type){
            continue;
        }
        if(it->buff.size() == 0)
        {
            return;
        }
        if(!it->file->open(QFile::WriteOnly|QFile::Append))
        {
            qDebug() << __FUNCTION__ << "file open failed.";
            return;
        }
        QTextStream stream(it->file);
        QByteArray temp = it->buff;
        it->buff.remove(0, temp.size());
        stream << temp;
        it->file->close();
        return;
    }
}

void Data_Logger::clear_logger(uint8_t type)
{
//    qDebug()<<"ThreadID"<<QThread::currentThreadId();
    QVector<file_unit_t>::iterator it = file_group.begin();
    for(; it != file_group.end(); it++){
        if(it->type != type){
            continue;
        }
        it->buff.clear();
        it->file->remove();
        return;
    }
}
