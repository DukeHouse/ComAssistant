#include "http.h"

HTTP::HTTP(QWidget *parentWidget)
{
    parent = parentWidget;

    //http访问
    m_NetManger = new QNetworkAccessManager(this);
    connect(m_NetManger, SIGNAL(finished(QNetworkReply*)), this, SLOT(httpFinishedSlot(QNetworkReply*)));

    //提交使用统计任务
    httpLock.lock();
    httpTaskVector.push_back(PostStatic);
    httpTaskVector.push_back(DownloadMSGs);
    httpTaskVector.push_back(BackStageGetVersion);
    httpLock.unlock();

    connect(&secTimer, SIGNAL(timeout()), this, SLOT(httpTimeoutHandle()));
    secTimer.start(1000);
}

//获取MAC地址
QString HTTP::getHostMacAddress()
{
    QList<QNetworkInterface> nets = QNetworkInterface::allInterfaces();// 获取所有网络接口列表
    int nCnt = nets.count();
    QString strMacAddr = "";
    for(int i = 0; i < nCnt; i ++)
    {
        // 如果此网络接口被激活并且正在运行并且不是回环地址，则就是我们需要找的Mac地址
        if(nets[i].flags().testFlag(QNetworkInterface::IsUp) && nets[i].flags().testFlag(QNetworkInterface::IsRunning) && !nets[i].flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            strMacAddr = nets[i].hardwareAddress();
            break;
        }
    }
    return strMacAddr;
}

/*
 * Function:上传使用统计
 * Web端代码：static.php
<?php
    $val = "";
    $fileName = $_GET["filename"];//从URL中获取文件名称,格式http://www.inhowe.com/test.php?filename=a.txt
    @$data = fopen($fileName,'a+');//添加不覆盖，首先会判断这个文件是否存在，如果不存在，则会创建该文件
    //应该以键值对的形式提交信息
    if($_POST){
        $val.='|POST|';
        foreach($_POST as $key =>$value){
            $val .= '|'.$key.":".$value;
        }
    }else{
        $val.='|GET|';
        foreach($_GET as $key =>$value){
                $val .= '|'.$key.":".$value;
        }
    }
    $val.= "\n";
    fwrite($data,$val);//写入文本中
    fclose($data);
?>
*/
bool HTTP::postUsageStatistic(void)
{
    //旧请求未完成时不执行。
    if(httpTimeout > 0)
        return false;

//    ui->statusBar->showMessage("正在提交使用统计...", 1000);

    //准备上传数据
    //为了不改动服务器端程序，不要打乱已有的顺序
    QString sendData = "Version=#VERSION#&StartTime=#STARTTIME#&RunTime=#RUNTIME#&TxCnt=#TXCNT#&RxCnt=#RXCNT#";
    sendData.replace("#VERSION#",Config::getVersion());
    QString current_date_str = Config::getStartTime();
    sendData.replace("#STARTTIME#",current_date_str);
    sendData.replace("#RUNTIME#",Config::getLastRunTime());
    sendData.replace("#TXCNT#",Config::getLastTxCnt());
    sendData.replace("#RXCNT#",Config::getLastRxCnt());

    //以本机MAC地址作为上传的文件名
    QString tmp = "http://www.inhowe.com/ComAssistant/static.php?filename=#FILENAME#.txt";
    tmp.replace("#FILENAME#",getHostMacAddress());
    QUrl url(tmp);

    //request请求
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded;charset=utf-8"); //以表单键值对的形式提交
    request.setHeader(QNetworkRequest::ContentLengthHeader, sendData.size());
//    m_NetManger = new QNetworkAccessManager;//重复使用所以不重复定义
    m_Reply = m_NetManger->post(request, sendData.toLocal8Bit());
    return true;
}

/**
 * @brief     功能上已经和getRemoteVersion_my_server完全一致了
 */
bool HTTP::getRemoteVersion(void)
{
    //旧请求未完成时不执行。
    if(httpTimeout > 0)
        return false;

//    ui->statusBar->showMessage("正在检查更新，请稍候……", 1000);

    //发起http请求远端的发布版本号
    QUrl url("http://www.inhowe.com/ComAssistant/Request/latest");
    QNetworkRequest request;
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    request.setUrl(url);
    m_Reply = m_NetManger->get(request);
    return true;
}

bool HTTP::getRemoteVersion_my_server(void)
{
    //旧请求未完成时不执行。
    if(httpTimeout>0)
        return false;

    //发起http请求远端的发布版本号
    QUrl url("http://www.inhowe.com/ComAssistant/Request/latest");
    QNetworkRequest request;
    request.setUrl(url);
    m_Reply = m_NetManger->get(request);
    return true;
}

bool HTTP::downloadMessages(void)
{
    //旧请求未完成时不执行。
    if(httpTimeout > 0)
        return false;

    //下载远端信息
    QUrl url(NEW_MSG_ADDR);
    QNetworkRequest request;
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    request.setUrl(url);
    m_Reply = m_NetManger->get(request);
    return true;
}

void HTTP::addTask(HttpFunction_e name)
{
    //最好不要重复添加任务,否则似乎会崩溃,八成是m_Reply指针共用的原因，这边安全起见加几个大锁
    httpLock.lock();
    for(int32_t i = 0; i < httpTaskVector.size(); i++)
    {
        if(httpTaskVector.at(i) == name)
        {
            httpLock.unlock();
            return;
        }
    }
    httpTaskVector.append(name);
    httpLock.unlock();
}

QStringList HTTP::getMsgList()
{
    return msgList;
}

bool HTTP::find_new_version()
{
    return findNewVersion;
}

QString HTTP::get_new_version_details()
{
    return newVersionDetails;
}

void HTTP::httpTimeoutHandle()
{
    //http超时放弃(要放在“处理http任务请求队列”前面)
    if(httpTimeout > 0){
        httpTimeout--;
        if(httpTimeout == 0){
            m_Reply->abort();
//            m_Reply->deleteLater();
            qDebug() << "http request timed out: " << cur_task;
        }
    }

    //处理http任务请求队列
    if(httpTaskVector.size() > 0 && httpTimeout == 0){
        switch(httpTaskVector.at(0)){
        case GetVersion:
            getRemoteVersion();break;
        case BackStageGetVersion:
            getRemoteVersion();break;
        case DownloadFile:
            break;
        case PostStatic:
            postUsageStatistic();break;
        case DownloadMSGs:
            downloadMessages();break;
        case BackStageGetVersion_MyServer:
            getRemoteVersion_my_server();break;
        default:
            break;
        }
        httpLock.lock();
        cur_task = httpTaskVector.at(0);
        httpTaskVector.pop_front();
        httpLock.unlock();
        httpTimeout = TIMEOUT_SECOND;
    }
}

//解析发布信息
int32_t HTTP::parseReleaseInfo(QString &inputStr, QString &remoteVersion, QString &remoteNote, QString &publishedTime)
{
    QJsonParseError jsonError;
    QJsonDocument document = QJsonDocument::fromJson(inputStr.toUtf8(), &jsonError); //转化为JSON文档
    if(document.isNull() || jsonError.error != QJsonParseError::NoError || !document.isObject()){
//        QMessageBox::information(nullptr, "提示", "版本数据解析异常");
        qDebug() << "版本数据解析异常";
        return -1;
    }
    QJsonObject object = document.object();
    if(object.contains("tag_name"))
    {
        QJsonValue value = object.value("tag_name");
        if(value.isString())
        {
            remoteVersion = value.toString();
        }
    }
    if(object.contains("body"))
    {
        QJsonValue value = object.value("body");
        if(value.isString())
        {
            remoteNote = value.toString();
        }
    }
    if(object.contains("published_at"))
    {
        QJsonValue value = object.value("published_at");
        if(value.isString())
        {
            publishedTime = value.toString();
        }
    }
    return 0;
}

void HTTP::httpFinishedSlot(QNetworkReply *reply)
{
    //立刻清零，防止超时delete
    httpTimeout = 0;
    static uint32_t GetVersion_failed = 0;  //GetVersion失败次数

    reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = reply->readAll();
        QString string = QString::fromUtf8(bytes);

        if(cur_task == GetVersion ||
           cur_task == BackStageGetVersion ||
           cur_task == BackStageGetVersion_MyServer )
        {
            QString remoteVersion;
            QString remoteNote;
            QString publishedTime;
            //提取版本号
            if(parseReleaseInfo(string, remoteVersion, remoteNote, publishedTime))
            {
                return;
            }

            QString localVersion;
            localVersion = Config::getVersion();

            //版本号比较
            if(Config::versionCompare(localVersion, remoteVersion) > 0)
            {
                findNewVersion = true;
                newVersionDetails = tr("当前版本号：") + localVersion + "\n"
                                    + tr("远端版本号：") + remoteVersion + "\n"
                                    + tr("发布时间：") + publishedTime + "\n"
                                    + tr("更新内容：") + "\n" + remoteNote + "\n";
                if(cur_task == GetVersion || GetVersion_failed > 0)
                {
                    GetVersion_failed = 0;
                    QMessageBox::Button button;
                    button = QMessageBox::information(nullptr, tr("提示"),
                                                      newVersionDetails + "\n"
                                                      + tr("点击Ok将打开外链。")
                                                      , QMessageBox::Ok | QMessageBox::No);
                    if(button == QMessageBox::Ok)
                    {
                        QDesktopServices::openUrl(QUrl(NEW_VERSION_ADDR));
                    }
                }
                parent->setWindowTitle(tr("纸飞机串口助手") + " - " + localVersion + " " + tr("新版本：V") + remoteVersion);
            }
            else
            {
                if(cur_task == GetVersion || GetVersion_failed > 0)
                {
                    GetVersion_failed = 0;
                    QMessageBox::information(nullptr, tr("提示"),
                                             tr("当前版本号：") + localVersion + "\n" +
                                             tr("远端版本号：") + remoteVersion + "\n" +
                                             tr("已经是最新版本。"));
                }
            }
        }
        else if(cur_task == PostStatic)
        {
            if(!string.isEmpty())
                qDebug()<<"PostStatic:"<<string;
        }
        else if(cur_task == DownloadMSGs)
        {
            int32_t hasIllegalStr = 0;
            //把下载的且合法的远端信息添加进变量
            if(string.indexOf("301 Moved Permanently") != -1)
            {
                hasIllegalStr++;
            }
            if(string.indexOf("<head") != -1 ||
               string.indexOf("<body") != -1 ||
               string.indexOf("<html") != -1)
            {
                hasIllegalStr++;
            }
            if(string.indexOf("<h1") != -1 ||
               string.indexOf("<h2") != -1 ||
               string.indexOf("<h3") != -1)
            {
                hasIllegalStr++;
            }
            if(!hasIllegalStr)
            {
                msgList = string.split('\n',QString::SkipEmptyParts);
            }
            else
            {
                qDebug() << "illegal msg string:" << string;
            }
        }else
        {
            qDebug()<<"http state error" << string;
        }
    }
    else
    {
        //只有GetVersion是主动更新，因此获取不到版本号时才弹失败提示
        if(cur_task == BackStageGetVersion){
            httpLock.lock();
            httpTaskVector.push_back(BackStageGetVersion_MyServer);
            httpLock.unlock();
        }else if(cur_task == GetVersion){
            qDebug() << "change version server";
            GetVersion_failed++;
            httpLock.lock();
            httpTaskVector.push_back(BackStageGetVersion_MyServer);
            httpLock.unlock();
        }else if(cur_task == BackStageGetVersion_MyServer){
            //两个服务器都失败才弹提示
            if(GetVersion_failed){
                QMessageBox::information(nullptr, tr("提示"),
                                         tr("当前版本号：") + Config::getVersion() + "\n" +
                                         tr("检查更新失败。"));
            }
        }else if(cur_task == PostStatic){

        }
        qDebug()<< "http reply err" << reply->errorString();
        reply->abort();
    }

    cur_task = Idle;
    reply->deleteLater();
}
