#include "tee_manager.h"

TeeManager::TeeManager(QObject *parent)
: QObject(parent)
{
}

TeeManager::~TeeManager()
{
}

/**
 * @brief     添加一个Tee窗口到管理器中
 * @param[in] 窗口标题
 * @param[in] 窗口
 * @return    错误码
 */
int32_t TeeManager::addTeeBrowser(QString teeBrowserTitle, QPlainTextEdit* teeBrowser)
{
    //TODO:重复添加会怎样？？？？
    if(!teeBrowser)
    {
        qDebug() << "teeBrowser is null at" << __FUNCTION__;
        return -1;
    }

    if(teeBrowserTitle == MAIN_TAB_NAME ||
       teeBrowserTitle == REGMATCH_TAB_NAME ||
       teeBrowserTitle.isEmpty())
    {
        qDebug() << "invalid teeBrowserTitle[" << teeBrowserTitle
                 << "] at" << __FUNCTION__;
        return -1;
    }

    TeeBrowserObj_t *obj = new TeeBrowserObj_t();
    obj->browser = teeBrowser;
    obj->buffer.clear();
    teeBrowserMap.insert(teeBrowserTitle, obj);
    return 0;
}

/**
 * @brief     从管理器中移除一个Tee窗口
 * @note      只是从管理器中移除并清空缓冲，窗口资源没有被释放
 * @param[in] 窗口标题
 * @return    错误码
 */
int32_t TeeManager::removeTeeBrowser(QString teeBrowserTitle)
{
    if(teeBrowserTitle == MAIN_TAB_NAME ||
       teeBrowserTitle == REGMATCH_TAB_NAME ||
       teeBrowserTitle.isEmpty())
    {
        qDebug() << "invalid teeBrowserTitle[" << teeBrowserTitle
                 << "] at" << __FUNCTION__;
        return -1;
    }

    TeeBrowserObj_t* obj = nullptr;
    obj = teeBrowserMap.value(teeBrowserTitle);
    if(obj)
    {
        clearTeeBrowserBuffer(teeBrowserTitle);
        teeBrowserMap.remove(teeBrowserTitle);
        delete obj;
        return 0;
    }

    qDebug() << "obj of" << teeBrowserTitle << "is null at" << __FUNCTION__;
    return -1;
}

/**
 * @brief     从管理器中选择一个Tee窗口
 * @param[in] 窗口标题
 * @return    选择的窗口
 */
QPlainTextEdit* TeeManager::selectTeeBrowser(QString teeBrowserTitle)
{
    if(teeBrowserTitle == MAIN_TAB_NAME ||
       teeBrowserTitle == REGMATCH_TAB_NAME ||
       teeBrowserTitle.isEmpty())
    {
        qDebug() << "invalid teeBrowserTitle[" << teeBrowserTitle
                 << "] at" << __FUNCTION__;
        return nullptr;
    }

    TeeBrowserObj_t* obj = nullptr;
    obj = teeBrowserMap.value(teeBrowserTitle);
    if(obj)
    {
        return obj->browser;
    }
    return nullptr;
}

/**
 * @brief     往窗口缓冲中增加数据
 * @param[in] 窗口标题
 * @param[in] 要缓冲的数据
 * @return    错误码
 */
int32_t TeeManager::appendTeeBrowserBuffer(QString teeBrowserTitle, const QByteArray &buffer)
{
    if(buffer.isEmpty())
    {
        return 0;
    }

    if(teeBrowserTitle == MAIN_TAB_NAME ||
       teeBrowserTitle == REGMATCH_TAB_NAME ||
       teeBrowserTitle.isEmpty())
    {
        qDebug() << "invalid teeBrowserTitle[" << teeBrowserTitle
                 << "] at" << __FUNCTION__;
        return -1;
    }

    TeeBrowserObj_t *obj = nullptr;
    obj = teeBrowserMap.value(teeBrowserTitle);
    if(!obj || obj->browser == nullptr)
    {
        return -1;
    }

    obj->bufferLock.lock();
    obj->buffer.append(buffer);
    obj->buffer.append("\n");
    obj->bufferLock.unlock();

    return 0;
}

/**
 * @brief     清除窗口缓冲
 * @param[in] 窗口标题
 * @return    错误码
 */
int32_t TeeManager::clearTeeBrowserBuffer(QString teeBrowserTitle)
{
    if(teeBrowserTitle.isEmpty())
    {
        qDebug() << "invalid teeBrowserTitle[" << teeBrowserTitle
                 << "] at" << __FUNCTION__;
        return -1;
    }

    TeeBrowserObj_t* obj = nullptr;
    obj = teeBrowserMap.value(teeBrowserTitle);

    if(!obj)
    {
        qDebug() << "obj is null at" << __FUNCTION__;
        return -1;
    }

    if(!obj->browser)
    {
        qDebug() << "browser is null at" << __FUNCTION__;
        return -1;
    }

    obj->buffer.clear();

    return 0;
}

/**
 * @brief     清除所有窗口缓冲
 * @return    错误码
 */
int32_t TeeManager::clearAllTeeBrowserBuffer()
{
    QMap<QString, TeeBrowserObj_t*>::iterator it;
    for(it = teeBrowserMap.begin(); it != teeBrowserMap.end(); it++)
    {
        if(clearTeeBrowserBuffer(it.key()))
        {
            qDebug() << "error happend at" << __FUNCTION__;
        }
    }
    return 0;
}

/**
 * @brief     更新指定窗口上屏
 * @param[in] 窗口标题
 * @return    错误码
 */
int32_t TeeManager::updateTeeBrowserText(QString teeBrowserTitle)
{
    if(teeBrowserTitle.isEmpty())
    {
        qDebug() << "invalid teeBrowserTitle[" << teeBrowserTitle
                 << "] at" << __FUNCTION__;
        return -1;
    }

    TeeBrowserObj_t* obj = nullptr;
    obj = teeBrowserMap.value(teeBrowserTitle);

    if(!obj)
    {
        qDebug() << "obj is null at" << __FUNCTION__;
        return -1;
    }

    if(!obj->browser)
    {
        qDebug() << "browser is null at" << __FUNCTION__;
        return -1;
    }

    if(obj->buffer.isEmpty())
    {
        return 0;
    }

    obj->bufferLock.lock();
    if(obj->buffer.endsWith('\n'))
    {
        obj->browser->appendPlainText(obj->buffer.mid(0, obj->buffer.size() - 1));
    }
    else
    {
        qDebug() << "buffer is not end with LF at" << __FUNCTION__
                 << "buffer:" << obj->buffer
                 << "name:" << teeBrowserTitle;
    }
    obj->buffer.clear();
    obj->bufferLock.unlock();
    return 0;
}

/**
 * @brief     更新所有窗口上屏
 * @return    错误码
 */
int32_t TeeManager::updateAllTeeBrowserText()
{
    QMap<QString, TeeBrowserObj_t*>::iterator it;
    for(it = teeBrowserMap.begin(); it != teeBrowserMap.end(); it++)
    {
        if(updateTeeBrowserText(it.key()))
        {
            qDebug() << "error happend at" << __FUNCTION__;
        }
    }
    return 0;
}

/**
 * @brief     更新窗口字体
 * @param[in] 字体
 * @return    错误码
 */
int32_t TeeManager::updateAllTeeBrowserFont(QFont font)
{
    TeeBrowserObj_t* obj = nullptr;
    QMap<QString, TeeBrowserObj_t*>::iterator it;
    for(it = teeBrowserMap.begin(); it != teeBrowserMap.end(); it++)
    {
        obj = nullptr;
        obj = it.value();
        if(obj)
            obj->browser->setFont(font);
    }
    return 0;
}

/**
 * @brief     更新窗口风格
 * @param[in] 风格字符串
 * @return    错误码
 */
int32_t TeeManager::updateAllTeeBrowserBackground(QColor itsColor)
{
    int32_t r, g, b;
    r = g = b = 0xFF;
    itsColor.setRed(r);
    itsColor.setGreen(g);
    itsColor.setBlue(b);

    QString background = "QPlainTextEdit { background-color: rgb(RGBR,RGBG,RGBB);}";
    background.replace("RGBR", QString::number(r));
    background.replace("RGBG", QString::number(g));
    background.replace("RGBB", QString::number(b));

    TeeBrowserObj_t* obj = nullptr;
    QMap<QString, TeeBrowserObj_t*>::iterator it;
    for(it = teeBrowserMap.begin(); it != teeBrowserMap.end(); it++)
    {
        obj = nullptr;
        obj = it.value();
        if(obj)
            obj->browser->setStyleSheet(background);
    }
    return 0;
}

/**
 * @brief     获取所有窗口指针
 * @return    窗口指针列表
 */
QVector<QPlainTextEdit*> TeeManager::getAllTeeBrowsers()
{
    QVector<QPlainTextEdit*> list;
    QMap<QString, TeeBrowserObj_t*>::iterator it;
    for(it = teeBrowserMap.begin(); it != teeBrowserMap.end(); it++)
    {
        list << it.value()->browser;
    }
    return list;
}
