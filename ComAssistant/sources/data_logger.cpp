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

// 只支持逗号分隔符且buff里的包必须是完整的，
bool Data_Logger::appendToXlsx(const QByteArray &buff, QString path)
{
    QXlsx::Document xlsx(path);
    QString buffStr = buff;
    QStringList buffList = buffStr.split('\n');
    int32_t rowCnt = xlsx.dimension().lastRow();
    //空xlsx文件判断
    if(rowCnt == -2)
        rowCnt = 0;
    for(int32_t i = 0; i < buffList.size(); i++)
    {
        if(buffList.at(i).isEmpty())
        {
            continue;
        }
        QStringList dataList = buffList.at(i).split(',');
        for(int32_t col = 0; col < dataList.size(); col++)
        {
            if(rowCnt == 0)//表头
            {
                xlsx.write(rowCnt + 1, col + 1, dataList.at(col));
            }
            else//数据
            {
                xlsx.write(rowCnt + 1, col + 1, dataList.at(col).toDouble());
            }
        }
        rowCnt++;
    }
    return xlsx.save();
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
        if(it->file->fileName().endsWith("xlsx"))
        {
            if(!appendToXlsx(it->buff, it->file->fileName()))
            {
                qDebug() << it->file->fileName() << " saved failed.";
            }
            it->buff.clear();
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
