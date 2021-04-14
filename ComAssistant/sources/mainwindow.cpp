#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHotkey>

#define BIRTHDAY_YEAR               "2020"
#define BIRTHDAY_DATE               "02-16"

#define RECOVERY_FILE_PATH          "ComAssistantRecovery.dat"
#define BACKUP_RECOVERY_FILE_PATH   "ComAssistantRecovery_back.dat"

#define UNPACK_SIZE_OF_RX           (4096)
#define UNPACK_SIZE_OF_TX           (256)

bool    g_agree_statement = false;  //同意相关声明标志
bool    g_log_record      = false;  //日志记录开关
bool    g_debugger        = false;  //调试开关

static int32_t  g_network_comm_mode = 0;    // 串口网络切换开关
static qint32   g_multiStr_cur_index = -1;  // -1 means closed this function
static QColor   g_background_color;
static QFont    g_font;
static bool     g_enableSumCheck;
static qint64   g_lastSecsSinceEpoch;
static QString  g_popupHotKeySequence;
static QHotkey  *g_popupHotkey = new QHotkey(nullptr);
static int32_t  g_theme_index = 0;

/**
 * @brief     注册全局快捷键
 * @param[in] 要注册的按键
 */
bool MainWindow::registPopupHotKey(QString keySequence)
{
    if(keySequence == g_popupHotKeySequence)
        return true;

    if(keySequence.isEmpty())
    {
        if(g_popupHotkey->resetShortcut())
        {
            g_popupHotKeySequence.clear();
            QMessageBox::information(this, tr("提示"), tr("已关闭全局弹出热键"));
            return true;
        }
        QMessageBox::information(this, tr("提示"), tr("全局弹出热键重置失败"));
        return false;
    }

    if(g_popupHotkey->setShortcut(keySequence,true))
    {
        connect(g_popupHotkey, &QHotkey::activated, [this](){
            this->show();
            this->raise();
            this->activateWindow();
            this->raise();
            QApplication::setActiveWindow(this);
            this->showNormal();
        });
        g_popupHotKeySequence = keySequence;
        return true;
    }
    else
    {
        g_popupHotKeySequence.clear();
        QMessageBox::information(this, tr("提示"),
                                 tr("全局弹出热键注册失败") +
                                 "\n[" + keySequence + "]");
        return false;
    }
}

/**
 * @brief     从文件读取配置
 */
void MainWindow::readConfig()
{
    //写入启动时间
    Config::setStartTime(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));

    //文本解析引擎
    on_actionTeeSupport_triggered(Config::getTeeSupport());
    on_actionTeeLevel2NameSupport_triggered(Config::getTeeLevel2NameSupport());

    //回车风格
    if(Config::getEnterStyle() == EnterStyle_e::WinStyle){
        ui->action_winLikeEnter->setChecked(true);
        ui->action_unixLikeEnter->setChecked(false);
    }else if (Config::getEnterStyle() == EnterStyle_e::UnixStyle) {
        ui->action_winLikeEnter->setChecked(false);
        ui->action_unixLikeEnter->setChecked(true);
    }else {
        ui->action_winLikeEnter->setChecked(true);
        ui->action_unixLikeEnter->setChecked(false);
        QMessageBox::warning(this, tr("警告"), tr("读取到未知的回车风格"));
    }

    //编码规则
    if(Config::getCodeRule() == CodeRule_e::UTF8){
        on_actionUTF8_triggered(true);
    }else if(Config::getCodeRule() == CodeRule_e::GBK){
        on_actionGBK_triggered(true);
    }

    //简洁模式
    on_actionSimpleMode_triggered
        (Config::getConfigBool(SECTION_GLOBAL, KEY_SIMPLE_MODE, false));

    //多字符串
    ui->actionMultiString->setChecked(Config::getMultiStringState());
    on_actionMultiString_triggered(Config::getMultiStringState());
    QStringList multi;
    multi = Config::getMultiString();
    while(multi.size()>0){
        ui->multiString->addItem(multi.at(0));
        multi.pop_front();
    }

    //tab页面和正则匹配字符串
    ui->regMatchEdit->setText(Config::getConfigString(SECTION_GLOBAL, KEY_REG_MATCH_STR, ""));
    QString activatedTabName = Config::getConfigString(SECTION_GLOBAL, KEY_ACTIVATED_TAB, "");
    for(int32_t i = 0; i < ui->tabWidget->count(); i++){
        if(ui->tabWidget->tabText(i) == activatedTabName){
            ui->tabWidget->setCurrentIndex(i);
            break;
        }
    }
    on_regMatchEdit_textChanged(ui->regMatchEdit->text());

    //文件对话框路径
    lastFileDialogPath = Config::getLastFileDialogPath();

    //加载高亮规则
    ui->actionKeyWordHighlight->setChecked(Config::getKeyWordHighlightState());
    on_actionKeyWordHighlight_triggered(Config::getKeyWordHighlightState());
//    ui->actionKeyWordHighlight->setVisible(false);
//    on_actionKeyWordHighlight_triggered(true);

    //时间戳
    ui->timeStampCheckBox->setChecked(Config::getTimeStampState());
    ui->timeStampTimeOut->setText(QString::number(Config::getTimeStampTimeOut()));
    if(!ui->timeStampCheckBox->isChecked())
        ui->timeStampTimeOut->setEnabled(false);
    //发送间隔
    ui->sendInterval->setText(QString::number(Config::getSendInterval()));
    //hex发送
    ui->hexSend->setChecked(Config::getHexSendState());
    //hex显示
    ui->hexDisplay->setChecked(Config::getHexShowState());
    //波特率
    ui->baudrateList->setCurrentText(QString::number(Config::getBaudrate()));
    //文本发送区，不能放在hex发送前面
    ui->textEdit->setText(Config::getTextSendArea());
    ui->textEdit->moveCursor(QTextCursor::End);
    //背景色
    updateUIPanelBackground(Config::getBackGroundColor());

    //绘图器开关
    ui->actionPlotterSwitch->setChecked(Config::getPlotterState());
    on_actionPlotterSwitch_triggered(Config::getPlotterState());

    //协议类型
    if(Config::getPlotterType()==ProtocolType_e::Ascii||
       Config::getPlotterType()==ProtocolType_e::Ascii_SumCheck){
        on_actionAscii_triggered(true);
        if(Config::getPlotterType()==ProtocolType_e::Ascii_SumCheck){
            on_actionSumCheck_triggered(true);
        }
    }
    else if(Config::getPlotterType()==ProtocolType_e::Float||
            Config::getPlotterType()==ProtocolType_e::Float_SumCheck){
        on_actionFloat_triggered(true);
        if(Config::getPlotterType()==ProtocolType_e::Float_SumCheck){
            on_actionSumCheck_triggered(true);
        }
    }
    else if(Config::getPlotterType()==ProtocolType_e::CSV||
            Config::getPlotterType()==ProtocolType_e::CSV_SumCheck){
        on_actionCSV_triggered(true);
        if(Config::getPlotterType()==ProtocolType_e::CSV_SumCheck){
            on_actionSumCheck_triggered(true);
        }
    }
    else if(Config::getPlotterType()==ProtocolType_e::MAD||
            Config::getPlotterType()==ProtocolType_e::MAD_SumCheck){
        on_actionMAD_triggered(true);
        if(Config::getPlotterType()==ProtocolType_e::MAD_SumCheck){
            on_actionSumCheck_triggered(true);
        }
    }

    //轴标签
    ui->customPlot->xAxis->setLabel(Config::getXAxisName());
    ui->customPlot->yAxis->setLabel(Config::getYAxisName());
    //图像名字集
    ui->customPlot->plotControl->setNameSet(
        Config::getPlotterGraphNames(ui->customPlot->plotControl->getMaxValidGraphNumber()));
    //线形
    ui->customPlot->plotControl->setupLineType(Config::getLineType());
    //OpenGL
    ui->customPlot->useOpenGL(Config::getOpengGLState());

    //数值显示器
    ui->actionValueDisplay->setChecked(Config::getValueDisplayState());
    on_actionValueDisplay_triggered(Config::getValueDisplayState());

    //网络通信相关
    QString addr;
    QString addrStrSet;
    QStringList addrStrList;
    QRegExp validAddrFormat;
    addrStrSet = Config::getConfigString(SECTION_NETWORK, KEY_UDPS_REMOTE_ADDR, "");
    addrStrList = addrStrSet.split(";");
    validAddrFormat.setPattern("\\d{1,3}.\\d{1,3}.\\d{1,3}.\\d{1,3}:\\d{1,5}");
    server_remoteAddr_backup_list.clear();
    for(int32_t i = 0; i < addrStrList.size(); i++)
    {
        addr = addrStrList.at(i);
        if(addr.contains(validAddrFormat))
            server_remoteAddr_backup_list << addr;
    }
    addrStrSet = Config::getConfigString(SECTION_NETWORK, KEY_CLIENT_TARGET_IP, "");
    addrStrList = addrStrSet.split(";");
    validAddrFormat.setPattern("\\d{1,3}.\\d{1,3}.\\d{1,3}.\\d{1,3}");
    for(int32_t i = 0; i < addrStrList.size(); i++)
    {
        addr = addrStrList.at(i);
        if(addr.contains(validAddrFormat))
            client_targetIP_backup_List << addr;
    }
    addrStrSet = Config::getConfigString(SECTION_NETWORK, KEY_CLIENT_TARGET_PORT, "");
    addrStrList = addrStrSet.split(";");
    validAddrFormat.setPattern("\\d{1,5}");
    ui->comboBox_targetPort->clear();
    for(int32_t i = 0; i < addrStrList.size(); i++)
    {
        addr = addrStrList.at(i);
        if(addr.contains(validAddrFormat)
           && ui->comboBox_targetPort->findText(addr) == -1)
        {
            ui->comboBox_targetPort->addItem(addr);
            if(i == 0)
                ui->comboBox_targetPort->setCurrentText(addr);
        }
    }
    if(!ui->comboBox_targetPort->count())
    {
        ui->comboBox_targetPort->addItem(QString::number(DEFAULT_PORT));
    }
    QString str = Config::getConfigString(SECTION_NETWORK, KEY_NETWORK_MODE, "TCP Server");
    if(str == "TCP Server"
       || str == "TCP Client"
       || str == "UDP Server"
       || str == "UDP Client" )
    {
        on_networkModeBox_activated(str);
    }
    else
    {
        on_networkModeBox_activated("TCP Server");
    }

#if SHOW_PLOTTER_SETTING
    //refreshYAxis
    on_actionAutoRefreshYAxis_triggered(Config::getRefreshYAxisState());
#else
    ui->plotterSetting->menuAction()->setVisible(false);
#endif

    //注册全局呼出快捷键
    registPopupHotKey(Config::getPopupHotKey());
}

/**
 * @brief     调整窗口布局
 */
void MainWindow::adjustLayout()
{
    int32_t length;
    QList<int32_t> lengthList;

    //splitter_io垂直间距调整
    if(!ui->actionSimpleMode->isChecked())
    {
        length = splitter_io->height();
        lengthList.clear();
        lengthList << static_cast<qint32>(length*0.8)
                   << static_cast<qint32>(length*0.2);
        splitter_io->setSizes(lengthList);
    }
    //splitter_output垂直间距调整
    length = splitter_output->height();
    lengthList.clear();
    lengthList << static_cast<qint32>(length*0.8)
               << static_cast<qint32>(length*0.2);
    splitter_output->setSizes(lengthList);

    #define FACT_COE    0.645
    if(ui->actionPlotterSwitch->isChecked() || ui->actionValueDisplay->isChecked())
    {
        //splitter_visulize水平间距调整
        length = ui->splitter_visulize->width();
        lengthList.clear();
        lengthList << static_cast<qint32>(length * FACT_COE)
                   << static_cast<qint32>(length * (1 - FACT_COE));
        ui->splitter_visulize->setSizes(lengthList);
    }
    if(ui->actionMultiString->isChecked())
    {
        //splitter_display水平间距调整
        length = ui->splitter_display->width();
        lengthList.clear();
        lengthList << static_cast<qint32>(length * FACT_COE)
                   << static_cast<qint32>(length * (1 - FACT_COE));
        ui->splitter_display->setSizes(lengthList);
    }
}


/**
 * @brief     配置窗口布局
 */
void MainWindow::layoutConfig()
{
    //输入输出分裂器
    splitter_output = new QSplitter(Qt::Vertical, this);
    splitter_output->addWidget(ui->splitter_visulize);
    splitter_output->addWidget(ui->splitter_display);
    splitter_io = new QSplitter(Qt::Vertical, this);
    splitter_io->addWidget(splitter_output);
    splitter_io->addWidget(ui->widget_input);

    //设置顶级布局
    central = new QVBoxLayout();
    central->addWidget(ui->widget_ctrl);
    central->addWidget(ui->widget_net_ctrl);
    central->addWidget(splitter_io);
    ui->centralWidget->setLayout(central);
}

/**
 * @brief     软件首次运行通知
 */
int32_t MainWindow::firstRunNotify()
{
    if(Config::getFirstRun())
    {
        Config::setFirstStartTime(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        //弹出声明
        //创建关于我对话框资源
        About_Me_Dialog* p = new About_Me_Dialog(this);
        p->updateTitle(tr("欢迎使用纸飞机调试助手！"));
        p->getVersionString(Config::getVersion());
        //设置close后自动销毁
        p->setAttribute(Qt::WA_DeleteOnClose);
        //阻塞式显示
        p->exec();
        QMessageBox::StandardButton button;
        button = QMessageBox::information(this, tr("提示"),
                                            tr("我已知悉并同意相关声明。"),
                                            QMessageBox::Ok, QMessageBox::Cancel);
        if(button == QMessageBox::Cancel)
        {
            g_agree_statement = false;
        }
        else
        {
            g_agree_statement = true;
            //弹出帮助文件
//            on_actionManual_triggered();
        }

        g_network_comm_mode = QMessageBox::information(this, tr("提示"),
                                          tr("请选择常用的工作模式，后续可在\"功能\"选项中进行重设。"),
                                          tr("串口调试"), tr("网络调试"));
        on_actionNetworkMode_triggered(g_network_comm_mode);
    }
    else
    {
        g_agree_statement = true;
        if(Config::versionCompare(Config::readVersion(), "0.5.3") > 0
        || Config::isEvalVersionFromIniFile())
        {
            QMessageBox::information(this,
                                    tr("提示"),
                                    tr("纸飞机调试助手现已支持多窗口绘图！") + "\n\n" +
                                    tr("请关注ASCII协议变化：") + "\n" +
                                    tr("请修改{:1,2,3}为{plotter:1,2,3}") + "\n" +
                                    tr("其中plotter可以是任意英文字符。") + "\n" +
                                    tr("如发送{plotter:1,2,3}\\n{vol:4,5,6}\\n将可以出现2个绘图窗口。") + "\n\n" +
                                    tr("详情请重新了解帮助文档，感谢阁下的使用！"),
                                    QMessageBox::Ok);
        }
    }
    return 0;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)

{
    ui->setupUi(this);

    //日志记录开关
    on_actionLogRecord_triggered(Config::getLogRecord());

    //槽
    connect(&cycleSendTimer, SIGNAL(timeout()), this, SLOT(cycleSendTimerSlot()));
    connect(&secTimer, SIGNAL(timeout()), this, SLOT(secTimerSlot()));
    connect(&printToTextBrowserTimer, SIGNAL(timeout()), this, SLOT(printToTextBrowserTimerSlot()));
    connect(&plotterShowTimer, SIGNAL(timeout()), this, SLOT(plotterShowTimerSlot()));
    connect(&parseTimer100hz, SIGNAL(timeout()), this, SLOT(parseTimer100hzSlot()));
    connect(&multiStrSeqSendTimer, SIGNAL(timeout()), this, SLOT(multiStrSeqSendTimerSlot()));
    connect(&serial, SIGNAL(readyRead()), this, SLOT(readSerialPort()));
    connect(&serial, SIGNAL(bytesWritten(qint64)), this, SLOT(serialBytesWritten(qint64)));
    connect(&serial, SIGNAL(error(QSerialPort::SerialPortError)),  this, SLOT(handleSerialError(QSerialPort::SerialPortError)));
    connect(this, SIGNAL(sendKeyToPlotter(QKeyEvent*, bool)), ui->customPlot, SLOT(recvKey(QKeyEvent*, bool)));

    connect(ui->textBrowser->verticalScrollBar(),SIGNAL(actionTriggered(int)),this,SLOT(verticalScrollBarActionTriggered(int)));

    //点击下拉框刷新串口
    connect(ui->comList, SIGNAL(leftPressed()), this, SLOT(on_refreshCom_clicked()));
    ui->refreshCom->setVisible(false);

    //状态栏标签
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setAlignment(Qt::AlignVCenter);
    progressBar->setVisible(false);
    statusRemoteMsgLabel = new QLabel(this);
    statusSpeedLabel = new QLabel(this);
    statusStatisticLabel = new QLabel(this);
    statusTimer = new QLabel(this);
    statusTimer->setText("Timer:" + formatTime(0));
    statusRemoteMsgLabel->setOpenExternalLinks(true);//可打开外链
    ui->statusBar->addPermanentWidget(statusRemoteMsgLabel);
    ui->statusBar->addPermanentWidget(progressBar);
    ui->statusBar->addPermanentWidget(statusTimer);
    ui->statusBar->addPermanentWidget(statusStatisticLabel);//显示永久信息
    ui->statusBar->addPermanentWidget(statusSpeedLabel);

    //设置波特率框和发送间隔框的合法输入范围
    ui->baudrateList->setValidator(new QIntValidator(0, 9999999, this));
    ui->sendInterval->setValidator(new QIntValidator(0, 99999, this));
    ui->timeStampTimeOut->setValidator(new QIntValidator(0, 99999, this));

    //文件解包器初始化
    fileUnpacker = new FileUnpacker(this);
    connect(fileUnpacker, SIGNAL(newPack(const QByteArray &, qint32, qint32)),
            this, SLOT(recvNewFilePack(const QByteArray &, qint32, qint32)));
    connect(fileUnpacker, SIGNAL(unpackResult(bool, QString)),
            this, SLOT(recvUnpackResult(bool, QString)));
    fileUnpacker->start();

    //fft
    fft_window = new FFT_Dialog(ui->actionFFTShow, this);

    //初始化绘图解析器
    plotProtocol = new DataProtocol();
    plotProtocol_thread = new QThread(this);
    plotProtocol->moveToThread(plotProtocol_thread);
    qRegisterMetaType<qint32>("qint32&");
    connect(plotProtocol_thread, SIGNAL(finished()), plotProtocol, SLOT(deleteLater()));
    connect(this, SIGNAL(protocol_appendData(const QByteArray &)),
            plotProtocol, SLOT(appendData(const QByteArray &)));
    connect(this, SIGNAL(protocol_parseData(bool)),
            plotProtocol, SLOT(parseData(bool)));
    connect(this, SIGNAL(protocol_clearBuff(const QString &)),
            plotProtocol, SLOT(clearBuff(const QString &)));
    plotProtocol->setDefaultPlotterTitle(Config::getConfigString(SECTION_GLOBAL, KEY_DEFAULT_PLOT_TITLE, DEFAULT_PLOT_TITLE_MACRO));
    plotProtocol_thread->start();

    //初始化绘图器(要放在绘图解析器后面初始化)
    ui->customPlot->init(ui->actionSavePlotData, ui->actionSavePlotAsPicture,
                         XAxis_Cnt,
                         Config::getRefreshYAxisState(),
                         fft_window,
                         plotProtocol,
                         plotProtocol->getDefaultPlotterTitle());
    ui->customPlot->setAutoRescaleYAxis(Config::getRefreshYAxisState());
    ui->tabWidget_plotter->setTabText(0, plotProtocol->getDefaultPlotterTitle());
    plotterManager.setDefaultPlotter(ui->customPlot);

    //初始化完成后ui->customPlot不再建议使用，建议通过plotterManager进行管理
    plotterManager.addPlotter(plotProtocol->getDefaultPlotterTitle(), ui->customPlot);

    //文本提取引擎初始化
    qDebug() << "Main threadID :" << QThread::currentThreadId();
    p_textExtractThread = new QThread(this);
    p_textExtract       = new TextExtractEngine();
    p_textExtract->moveToThread(p_textExtractThread);
    connect(p_textExtractThread, SIGNAL(finished()), p_textExtract, SLOT(deleteLater()));
    connect(this, SIGNAL(tee_appendData(const QByteArray &)), p_textExtract, SLOT(appendData(const QByteArray &)));
    connect(this, SIGNAL(tee_parseData()), p_textExtract, SLOT(parseData()));
    connect(this, SIGNAL(tee_clearData(const QString &)), p_textExtract, SLOT(clearData(const QString )));
    connect(this, SIGNAL(tee_saveData(const QString &, const QString &, const bool& )),
            p_textExtract, SLOT(saveData(const QString &, const QString &, const bool& )));
    connect(p_textExtract, SIGNAL(textGroupsUpdate(const QString &, const QByteArray &)),
            this, SLOT(tee_textGroupsUpdate(const QString &, const QByteArray &)));
    connect(p_textExtract, SIGNAL(saveDataResult(const qint32&, const QString &, const qint32 )),
            this, SLOT(tee_saveDataResult(const qint32&, const QString &, const qint32 )));
    p_textExtractThread->start();

    //正则匹配引擎初始化
    p_regMatchThread = new QThread(this);
    p_regMatch       = new RegMatchEngine(nullptr);
    p_regMatch->moveToThread(p_regMatchThread);
    connect(p_regMatchThread, SIGNAL(finished()), p_regMatch, SLOT(deleteLater()));
    connect(this, SIGNAL(regM_appendData(const QByteArray &)), p_regMatch, SLOT(appendData(const QByteArray &)));
    connect(this, SIGNAL(regM_parseData()), p_regMatch, SLOT(parseData()));
    connect(this, SIGNAL(regM_clearData()), p_regMatch, SLOT(clearData()));
    connect(this, SIGNAL(regM_saveData(const QString &)),
            p_regMatch, SLOT(saveData(const QString &)));
    connect(p_regMatch, SIGNAL(dataUpdated(const QByteArray &)),
            this, SLOT(regM_dataUpdated(const QByteArray &)));
    connect(p_regMatch, SIGNAL(saveDataResult(const qint32&, const QString &, const qint32 )),
            this, SLOT(regM_saveDataResult(const qint32&, const QString &, const qint32 )));
    p_regMatchThread->start();

    //数据记录器(设计成线程模式为了做一个缓冲收集一段时间数据批量写入以缓解高频接收时的硬盘写入压力
    //（虽然直接写好像硬盘也没看出使用率很高）
    p_logger_thread = new QThread(this);
    p_logger        = new Data_Logger();
    p_logger->moveToThread(p_logger_thread);
    qRegisterMetaType<uint8_t>("uint8_t");
    connect(p_logger_thread, SIGNAL(finished()), p_logger, SLOT(deleteLater()));
    connect(this, SIGNAL(logger_append(uint8_t , const QByteArray &)), p_logger, SLOT(append_data_logger_buff(uint8_t , const QByteArray &)));
    connect(this, SIGNAL(logger_flush(uint8_t)), p_logger, SLOT(logger_buff_flush(uint8_t)));
    p_logger_thread->start();
    p_logger->init_logger(RECOVERY_LOG, RECOVERY_FILE_PATH);    //恢复log必须开启，其他log按需开启

    //网络调试模块
    p_networkCommThread = new QThread(this);
    p_networkComm        = new NetworkComm(nullptr);
    p_networkComm->moveToThread(p_networkCommThread);
    connect(p_networkCommThread, SIGNAL(finished()), p_networkComm, SLOT(deleteLater()));
    connect(this, SIGNAL(initNetwork()), p_networkComm, SLOT(init()));
    connect(p_networkComm, SIGNAL(readBytes(const QByteArray&)), this, SLOT(readDataNetwork(const QByteArray&)));
    connect(p_networkComm, SIGNAL(bytesWritten(qint64)), this, SLOT(serialBytesWritten(qint64)));
    connect(this, SIGNAL(writeToNetwork(const QByteArray&)), p_networkComm, SLOT(write(const QByteArray&)));
    connect(this, SIGNAL(connectToNetwork(qint32, QString , quint16)), p_networkComm, SLOT(connect(qint32, QString , quint16)));
    connect(this, SIGNAL(disconnectFromNetwork()), p_networkComm, SLOT(disconnect()));
    connect(p_networkComm, SIGNAL(error(qint32, QString)), this, SLOT(errorNetwork(qint32, QString)));
    connect(p_networkComm, SIGNAL(message(qint32, QString)), this, SLOT(msgNetwork(qint32, QString)));
    p_networkCommThread->start();
    emit initNetwork();

    //串口网络模式切换
    g_network_comm_mode = Config::getConfigNumber(SECTION_GLOBAL, KEY_WORKMODE, 0);
    on_actionNetworkMode_triggered(g_network_comm_mode);

    //设置窗体布局
    layoutConfig();

    //绑定分裂器移动槽
    connect(ui->splitter_display, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMovedSlot(int, int)));
    connect(splitter_io, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMovedSlot(int, int)));
    connect(splitter_output, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMovedSlot(int, int)));

    //显示收发统计
    serial.resetCnt();
    p_networkComm->resetCnt();
    if(g_network_comm_mode)
    {
        statusStatisticLabel->setText(p_networkComm->getTxRxString_with_color());
    }
    else
    {
        statusStatisticLabel->setText(serial.getTxRxString_with_color());
    }

    //搜寻可用串口，并尝试打开
    if(!g_network_comm_mode)
    {
        refreshCom();
        tryOpenSerial();
    }

    //数值显示器初始化
    ui->valueDisplay->setColumnCount(2);
    ui->valueDisplay->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("名称")));
    ui->valueDisplay->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("值")));
    ui->valueDisplay->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->valueDisplay->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->valueDisplay->horizontalHeader()->setStretchLastSection(true);

    this->setAutoFillBackground(true);

    //设置主题
    setWindowTheme(Config::getConfigNumber(SECTION_GLOBAL, KEY_THEME_INDEX, 0));
    g_font = Config::getGUIFont();
    updateUIPanelFont(g_font);

    this->setWindowTitle(tr("纸飞机调试助手") + " - V"+Config::getVersion());

    //接受拖入文件的动作
    this->setAcceptDrops(true);

    //启动定时器
    secTimer.setTimerType(Qt::PreciseTimer);
    secTimer.start(1000);
    printToTextBrowserTimer.setTimerType(Qt::PreciseTimer);
    printToTextBrowserTimer.start(TEXT_SHOW_PERIOD);
    plotterShowTimer.setTimerType(Qt::PreciseTimer);
    plotterShowTimer.start(PLOTTER_SHOW_PERIOD);
    multiStrSeqSendTimer.setTimerType(Qt::PreciseTimer);
    multiStrSeqSendTimer.setSingleShot(true);
    parseTimer100hz.setTimerType(Qt::PreciseTimer);
    parseTimer100hz.start(10);

    //textBrowser右键内容初始化
    textBrowser_rightClickContext_init();

    //计时器
    g_lastSecsSinceEpoch = QDateTime::currentSecsSinceEpoch();

    //快速教程
    quickHelp();

    //显示界面
    this->show();

    //读取配置（所有资源加载完成后读取，有些配置需要根据可见性改变所以显示后读取）
    readConfig();

    //release模式下关闭调试功能
#ifdef QT_NO_DEBUG
    ui->actiondebug->setVisible(false);
#endif

    //调整窗体布局
    adjustLayout();

    //是否首次运行
    firstRunNotify();

    //http
    if(g_agree_statement)
        http = new HTTP(this);

    //debugger模式控制
    debuggerModeControl();

    if(readWriteAuthorityTest())
    {
        QMessageBox::warning(this, tr("警告"),
                       tr("纸飞机在当前路径下无法读写文件！") + "\n\n"
                       + tr("请移动纸飞机至非系统路径，否则将导致数据记录、数据恢复、配置保存、数据保存等功能无法使用！"));
    }

    readRecoveryFile();

}

/**
 * @brief     debugger模式设置
 */
void MainWindow::debuggerModeControl()
{
    ui->actionMAD->setVisible(false);
    ui->actionNetworkMode->setVisible(false);
    if(g_debugger)
    {
       ui->actionMAD->setVisible(true);
       ui->actiondebug->setVisible(true);
       ui->actionNetworkMode->setVisible(true);
    }
    else
    {
        QFile support_mode_file;
        do{
            support_mode_file.setFileName("work_in_serialport");
            if(support_mode_file.exists())
            {
                g_network_comm_mode = 0;
                on_actionNetworkMode_triggered(g_network_comm_mode);
                ui->actionNetworkMode->setVisible(false);
                break;
            }
            support_mode_file.setFileName("work_in_network");
            if(support_mode_file.exists())
            {
                g_network_comm_mode = 1;
                on_actionNetworkMode_triggered(g_network_comm_mode);
                ui->actionNetworkMode->setVisible(false);
                break;
            }
            ui->actionNetworkMode->setVisible(true);
        }while(0);
    }
}

/**
 * @brief     读写测试，判断是否有权限在该路径写入文件
 */
int32_t MainWindow::readWriteAuthorityTest()
{
    int32_t failed = 0;
    QString testData = "ComAssistantAccessTest";
    QFile testFile(testData);
    do{
        if(!testFile.open(QIODevice::ReadWrite))
        {
            failed++;
            break;
        }
        if(testFile.write(testData.toLocal8Bit()) != testData.size())
        {
            failed++;
            testFile.close();
            break;
        }
        if(!testFile.flush())
        {
            failed++;
            testFile.close();
            break;
        }
        testFile.close();
        if(!testFile.open(QIODevice::ReadWrite))
        {
            failed++;
            break;
        }
        if(testFile.readAll() != testData)
        {
            failed++;
            testFile.close();
            break;
        }
        testFile.close();
    }while(0);

    if(testFile.exists())
        testFile.remove();

    if(failed)
        return -1;
    return 0;
}

/**
 * @brief     快速帮助
 */
void MainWindow::quickHelp()
{
    QString helpText;
    if (QLocale::system().name() != "zh_CN")
    {
        helpText = "Data display area"
                "\n\n"
                "Quick Guide：\n\n"
                "# ASCII Protocol(text string): \n"
                "  \"{title:string}\\n\"\n"
                "  'title' can be any word. Dart will show data in windows according to title\n"
                "  'string' can be any text containt. It's the data in windows\n"
                "  '\\n' is wrap symbol\n"
                "  It's recommended to select one title for each task\n\n"
                "# Guide with C language\n"
                "  1.Define macro function to simplify work\n"
                "    #define PRINT(title, fmt, ...) printf(\"{\"#title\":\"fmt\"}\\n\", __VA_ARGS__)\n"
                "  2.To draw graph, use it like this: \n"
                "    PRINT(plotter, \"%f,%f\", data1, data2); It means 2 graphs displayed on \'plotter\' window.\n"
                "  3.To classify text, use it like this: \n"
                "    PRINT(monitor, \"voltage is %f V\", data1); It means these text displayed on \'monitor\' window.\n";
        ui->textBrowser->setPlaceholderText(helpText);
        helpText = "Show the string contained the key word"
                "\n\n"
                "(only support string which end with LF character)";
        ui->regMatchBrowser->setPlaceholderText(helpText);
        helpText = "input key word";
        ui->regMatchEdit->setPlaceholderText(helpText);
        return;
    }
    helpText = "数据显示区"
            "\n\n"
            "快速教程：\n\n"
            "# ASCII协议规则(字符串)：\n"
            "  \"{title:string}\\n\"\n"
            "  title为自定义英文标题，纸飞机将根据title对数据进行分窗显示。\n"
            "  string为自定义英文字符串，这是将被分窗显示的数据内容。\n"
            "  {和}为一组花括号，表示一组数据包。\n"
            "  \\n为换行符，不可省略。\n"
            "  常推荐一种任务使用一种标题。\n\n"
            "# C语言使用方法：\n"
            "  1.定义宏函数可简化后期工作：\n"
            "    #define PRINT(title, fmt, ...) printf(\"{\"#title\":\"fmt\"}\\n\", __VA_ARGS__)\n"
            "  2.若要绘图可这样使用：\n"
            "    PRINT(plotter, \"%f,%f\", data1, data2); 表示2条曲线显示在plotter窗口。\n"
            "  3.若要分窗显示可这样使用：\n"
            "    PRINT(monitor, \"voltage is %f V\", data1); 表示电压数据显示在monitor窗口。\n";
    ui->textBrowser->setPlaceholderText(helpText);
    helpText = "该窗口显示包含关键字符的字符串"
            "\n\n"
            "（字符串需以换行符\\n结尾）";
    ui->regMatchBrowser->setPlaceholderText(helpText);
    helpText = "输入要过滤的关键字符";
    ui->regMatchEdit->setPlaceholderText(helpText);
}

/**
 * @brief     关闭事件
 * @note      如果在收发时关闭软件则进行提醒防止误操作
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    if(rxSpeedKB || txSpeedKB)
    {
        QMessageBox::StandardButton button;
        button = QMessageBox::information(this,
                                          tr("提示"),
                                          tr("纸飞机正在收发数据，确认关闭纸飞机吗？"),
                                          QMessageBox::Ok,
                                          QMessageBox::Cancel);
        if(button == QMessageBox::Ok)
        {
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }
}

/**
 * @brief     使能这些可交互控件
 */
void MainWindow::openInteractiveUI()
{
    ui->networkSwitch->setEnabled(true);
    ui->comSwitch->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->multiString->setEnabled(true);
    ui->cycleSendCheck->setEnabled(true);
    ui->clearWindows->setText(tr("清  空"));
    ui->clearWindows_simple->setText(ui->clearWindows->text());
    ui->clearWindows_simple_net->setText(ui->clearWindows->text());
}

/**
 * @brief     失能这些可交互控件
 */
void MainWindow::closeInteractiveUI()
{
    ui->networkSwitch->setEnabled(false);
    ui->comSwitch->setEnabled(false);
    ui->sendButton->setEnabled(false);
    ui->multiString->setEnabled(false);
    cycleSendTimer.stop();
    ui->cycleSendCheck->setEnabled(false);
    ui->cycleSendCheck->setChecked(false);
    ui->clearWindows->setText(tr("中  止"));
    ui->clearWindows_simple->setText(ui->clearWindows->text());
    ui->clearWindows_simple_net->setText(ui->clearWindows->text());
}

/**
 * @brief     更新解析进度条
 * @param[in] 预处理字符串
 * @param[in] 进度值
 */
void MainWindow::updateProgressBar(QString preStr, double percent)
{
    if(percent > 100)
    {
        percent = 100;
    }
    QString percent_str = QString::number(percent, 'f', 1);
    QString str = preStr + percent_str + "%";
    progressBar->setValue(static_cast<int32_t>(percent));
    progressBar->setFormat(str);
    if(percent_str.startsWith("100"))
    {
        progressBar->setValue(0);
        progressBar->setVisible(false);
    }
    else
    {
        progressBar->setVisible(true);
    }
}

/**
 * @brief     文件分包
 * @note      所有传入参数可能被修改
 * @param[in] 输入文件数据
 * @param[in] 输出文件数据
 * @param[in] 分包大小
 * @param[in] 分包标志位
 * @return    成功标志位
 */
int32_t MainWindow::divideDataToPacks(QByteArray &input, QByteArrayList &output, int32_t pack_size, bool &divideFlag)
{
    closeInteractiveUI();

    int32_t pack_num = input.size() / pack_size;
    int32_t pack_cnt = 0;
    double percent = 0;
    while(input.size() > pack_size && divideFlag){
        output.append(input.mid(0, pack_size));
        input.remove(0, pack_size);
        percent = 100.0 * pack_cnt / pack_num;
        updateProgressBar(tr("文件分包进度："), percent);
        pack_cnt++;
        if(pack_cnt % 5 == 0)
            qApp->processEvents();
    }
    output.append(input); //一定会有一个元素
    input.clear();

    //是否撤销解析
    if(output.size() < 1 || divideFlag == false){
        openInteractiveUI();
        ui->statusBar->showMessage("", 2000);
        return -1;
    }
    return 0;
}


/**
 * @brief     读取恢复文件
 * @note      主要是启动时检测
 */
void MainWindow::readRecoveryFile()
{
    QFile recoveryFile(RECOVERY_FILE_PATH);
    if(recoveryFile.exists() && recoveryFile.size())
    {
        qint32 button;
        button = QMessageBox::information(this, tr("提示"),
                                          tr("检测到数据恢复文件，可能是上次未正确关闭纸飞机，或者多开纸飞机导致的。") + "\n\n" +
                                          tr("点击Reload重载上次数据。") + "\n" +
                                          tr("点击Discard丢弃上次数据。") + "\n" +
                                          tr("点击Backup备份并丢弃上次数据。") + "\n",
                                          "Reload", "Discard", "Backup");
        if(button == 0)
        {
            //读文件
            if(unpack_file(readFile, RECOVERY_FILE_PATH, true, UNPACK_SIZE_OF_RX))
            {
                return;
            }
        }
        else if(button == 1)
        {
            recoveryFile.remove();
        }
        else if(button == 2)
        {
            //重命名前确保没有含新名字的文件
            QFile backFile(BACKUP_RECOVERY_FILE_PATH);
            if(backFile.exists())
                backFile.remove();
            if(backFile.exists())
            {
                QMessageBox::information(this, tr("提示"),
                                        tr("旧备份恢复数据文件删除失败，请自行处理：") + BACKUP_RECOVERY_FILE_PATH);
                return;
            }
            //重命名
            recoveryFile.rename(BACKUP_RECOVERY_FILE_PATH);
            QMessageBox::information(this, tr("提示"),
                                     tr("数据恢复文件已另存到程序所在目录下文件：") + "\n" +
                                     BACKUP_RECOVERY_FILE_PATH + "\n" +
                                     tr("请自行处理。"));
        }
    }
}

/**
 * @brief     保存分窗显示文本数据的结果的槽
 * @param[in] 保存的结果
 * @param[in] 保存的路径
 * @param[in] 保存的文件大小
 * @return
 */
void MainWindow::tee_saveDataResult(const qint32& result, const QString &path, const qint32 fileSize)
{
    QString str;
    switch (result) {
    case TextExtractEngine::SAVE_OK:
        str = "Total saved " + QString::number(fileSize) + " Bytes in " + path;
        appendMsgLogToBrowser(str);
        lastFileDialogPath = path;
        lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/')+1);
        break;
    case TextExtractEngine::OPEN_FAILED:
        QMessageBox::information(this, tr("提示"), tr("文件打开失败。"));
        break;
    case TextExtractEngine::UNKNOW_NAME:
        QMessageBox::information(this, tr("提示"), tr("未知的窗口名称"));
        break;
    }
}

/**
 * @brief     正则匹配引擎数据更新
 * @note      该函数可能会高频被触发，不能放耗时代码
 * @param[in] 更新的数据
 */
void MainWindow::regM_dataUpdated(const QByteArray &packData)
{
    QByteArray newPackData = packData;
    if(ui->actionGBK->isChecked())
    {
        QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
        QTextCodec* gbk = QTextCodec::codecForName("gbk");

        //gbk -> utf8
        //1. gbk to unicode
        //2. unicode -> utf-8
        newPackData = utf8->fromUnicode(
                      gbk->toUnicode(newPackData)
                      );
    }
    regMatchBufferLock.lock();
    regMatchBuffer.append(newPackData);
    regMatchBuffer.append('\n');
    regMatchBufferLock.unlock();

    statisticRegParseCnt += packData.size();
}

/**
 * @brief     正则匹配引擎数据保存结果的槽
 * @note      参考 @ref tee_saveDataResult
 */
void MainWindow::regM_saveDataResult(const qint32& result, const QString &path, const qint32 fileSize)
{
    tee_saveDataResult(result, path, fileSize);
}

/**
 * @brief     向设备发送数据
 */
int32_t MainWindow::writeDataToDevice(const QByteArray &data)
{
    if(!deviceIsOpen())
    {
        return -1;
    }

    if(g_network_comm_mode)
    {
        emit writeToNetwork(data);
        return data.size();
    }
    else
    {
        return serial.write(data);
    }
}

/**
 * @brief     查询设备是否打开
 */
int32_t MainWindow::deviceIsOpen()
{
    if(g_network_comm_mode)
    {
        return p_networkComm->isOpen();
    }
    else
    {
        return serial.isOpen();
    }
}

/**
 * @brief     提醒设备是否打开
 */
int32_t MainWindow::remindDeviceIsOpen()
{
    if(g_network_comm_mode)
    {
        if(!p_networkComm->isOpen())
        {
            QMessageBox::information(this, tr("提示"), tr("网络未打开。"));
            return 0;
        }
    }
    else
    {
        if(!serial.isOpen())
        {
            QMessageBox::information(this, tr("提示"), tr("串口未打开。"));
            return 0;
        }
    }
    return 1;
}

/**
 * @brief     收到文件解析器发的包
 * @note      解析并更新进度
 */
void MainWindow::recvNewFilePack(const QByteArray &pack, qint32 current_cnt, qint32 total_cnt)
{
    if(readFile)
    {
        readFileBuff = pack;
        readSerialPort();
        readFileBuff.clear();
        updateProgressBar(tr("解析进度："), 100.0*current_cnt/total_cnt);
    }
    else if(sendFile)
    {
        updateProgressBar(tr("发送进度："), 100.0*current_cnt/total_cnt);
        writeDataToDevice(pack);
    }
    fileUnpacker->unpack_ack();
}

/**
 * @brief     收到文件解析器发的最终结果
 * @note      关闭文件解析模式
 */
void MainWindow::recvUnpackResult(bool success, QString details)
{
    if(readFile)
    {
        readFile = false;
    }
    else if(sendFile)
    {
        sendFile = false;
    }
    openInteractiveUI();
    progressBar->setValue(0);
    progressBar->setVisible(false);
    if(success || details.indexOf("aborted") != -1)
    {
        return;
    }
    QMessageBox::information(this, tr("提示"),
                            tr("文件解包失败！") + "\n" + details);
}

/**
 * @brief     分窗显示引擎数据更新的槽
 * @note      该槽可能会高频被触发，不要放耗时代码
 * @param[in] 更新的数据的名称
 * @param[in] 更新的数据
 * @return
 */
void MainWindow::tee_textGroupsUpdate(const QString &name, const QByteArray &data)
{
    //statistic
    statisticTeeParseCnt += data.size();

    if(data.size() == 0)
    {
        return;
    }

    if(teeManager.appendTeeBrowserBuffer(name, data))
    {
        if(name == MAIN_TAB_NAME || name == REGMATCH_TAB_NAME)
        {
            qDebug() << "name can not be" << name << "at" << __FUNCTION__;
            ui->statusBar->showMessage(name + tr("已被系统占用，请更换名称。"), 2000);
            return;
        }
        //新增textEdit，并设置字体、背景、高亮器等属性
        QPlainTextEdit *textEdit = nullptr;
        textEdit = new QPlainTextEdit(this);
        textEdit->setFont(g_font);

        qint32 r,g,b;
        g_background_color.getRgb(&r,&g,&b);
        QString str = "{ background-color: rgb(RGBR,RGBG,RGBB);}";
        str.replace("RGBR", QString::number(r));
        str.replace("RGBG", QString::number(g));
        str.replace("RGBB", QString::number(b));
        textEdit->setStyleSheet("QPlainTextEdit" + str);

        if(ui->actionKeyWordHighlight->isChecked())
            new Highlighter(textEdit->document());

        textEdit->setReadOnly(true);

        ui->tabWidget->addTab(textEdit, name);
        teeManager.addTeeBrowser(name, textEdit);
    }

    //再次尝试把数据放缓冲里
    if(teeManager.appendTeeBrowserBuffer(name, data))
    {
        qDebug() << "teeManager append buffer error at" << __FUNCTION__
                 << "name:" << name
                 << "data:" << data;
    }
}

/**
 * @brief     周期打印数据到TextBrowser控件上
 */
void MainWindow::printToTextBrowserTimerSlot()
{
    //更新收发统计(可能会占用一点点点loading)
    if(g_network_comm_mode)
    {
        statusStatisticLabel->setText(p_networkComm->getTxRxString_with_color());
    }
    else
    {
        statusStatisticLabel->setText(serial.getTxRxString_with_color());
    }

    //打印分窗文本数据
    teeManager.updateAllTeeBrowserText();
    //打印正则匹配数据
    if(!regMatchBuffer.isEmpty())
    {
        regMatchBufferLock.lock();
        regMatchBuffer.remove(regMatchBuffer.size() - 1, 1);
        ui->regMatchBrowser->appendPlainText(regMatchBuffer);
        ui->regMatchBrowser->moveCursor(QTextCursor::End);
        regMatchBuffer.clear();
        regMatchBufferLock.unlock();
    }

    checkBlankProblem();

    //characterCount=0时或者窗口大小改变时重算窗口并重新显示
    if(characterCount == 0 || windowSize != ui->textBrowser->size())
    {
        printToTextBrowser();
        return;
    }
    if(TryRefreshBrowserCnt == DO_NOT_REFRESH_BROWSER)
        return;

    if(!disableRefreshWindow)
    {
        checkScrollBarTooLarge();
        checkBlankProblem();
    }

    //打印数据
    printToTextBrowser();

    if(TryRefreshBrowserCnt)
        TryRefreshBrowserCnt--;
}

/**
 * @brief     格式化时间字符串
 * @param[in] 时间毫秒
 */
QString MainWindow::formatTime(qint32 ms)
{
    qint32 ss = 1000;
    qint32 mi = ss * 60;
    qint32 hh = mi * 60;
    qint32 dd = hh * 24;

    long day = ms / dd;
    long hour = (ms - day * dd) / hh;
    long minute = (ms - day * dd - hour * hh) / mi;
    long second = (ms - day * dd - hour * hh - minute * mi) / ss;
    long milliSecond = ms - day * dd - hour * hh - minute * mi - second * ss;

    QString hou = QString::number(hour, 10);
    QString min = QString::number(minute, 10);
    QString sec = QString::number(second, 10);
    QString msec = QString::number(milliSecond, 10);

    if(min.size() == 1)
        min.insert(0, '0');
    if(sec.size() == 1)
        sec.insert(0, '0');

    //qDebug() << "minute:" << min << "second" << sec << "ms" << msec <<endl;

    return hou + ":" + min + ":" + sec ;
}

/**
 * @brief     收发速度统计和显示
 */
void MainWindow::TxRxSpeedStatisticAndDisplay()
{
    double idealSpeed = 0;
    int32_t txLoad, rxLoad;

    //传输速度统计与显示
    rxSpeedKB = static_cast<double>(statisticRxByteCnt) / 1000.0;
    statisticRxByteCnt = 0;
    txSpeedKB = static_cast<double>(statisticTxByteCnt) / 1000.0;
    statisticTxByteCnt = 0;
    //负载率计算(公式中的1是起始位)(网络模式下不显示负载率)
    if(g_network_comm_mode)
    {
        idealSpeed = 1;
    }
    else
    {
        idealSpeed = (double)serial.baudRate()/(serial.stopBits()+serial.parity()+serial.dataBits()+1)/1000.0;
    }

    txLoad = 100 * txSpeedKB / idealSpeed;
    rxLoad = 100 * rxSpeedKB / idealSpeed;
    if(g_network_comm_mode)
    {
        txLoad = rxLoad = 0;
    }

    QString txSpeedStr;
    QString rxSpeedStr;
    #define HIGH_LOAD_WARNING    90
    if(txSpeedKB < 1)
    {
        txSpeedStr = " T:" + QString::number(static_cast<int32_t>(txSpeedKB * 1000.0)) + "B/s(" +
                     QString::number(txLoad) + "%)";
    }
    else if(txSpeedKB < 1000)
    {
        txSpeedStr = " T:" + QString::number(txSpeedKB, 'f', 2) + "KB/s(" +
                     QString::number(txLoad) + "%)";
    }
    else
    {
        //转MB
        txSpeedKB = txSpeedKB / 1000;
        txSpeedStr = " T:" + QString::number(txSpeedKB, 'f', 2) + "MB/s(" +
                     QString::number(txLoad) + "%)";
    }
    if(txLoad > HIGH_LOAD_WARNING)
    {
        //电脑串口有缓冲，因此会出现使用率大于100%的情况
        if(txLoad > 100)
        {
            txSpeedStr = "<font color=#FF0000>" + txSpeedStr + "</font>";
        }
        else
        {
            txSpeedStr = "<font color=#FF5A5A>" + txSpeedStr + "</font>";
        }
    }
    if(rxSpeedKB < 1)
    {
        rxSpeedStr = " R:" + QString::number(static_cast<int32_t>(rxSpeedKB * 1000.0)) + "B/s(" +
                     QString::number(rxLoad) + "%)";
    }
    else if(rxSpeedKB < 1000)
    {
        rxSpeedStr = " R:" + QString::number(rxSpeedKB, 'f', 2) + "KB/s(" +
                     QString::number(rxLoad) + "%)";
    }
    else
    {
        //转MB
        rxSpeedKB = rxSpeedKB / 1000;
        rxSpeedStr = " R:" + QString::number(rxSpeedKB, 'f', 2) + "MB/s(" +
                     QString::number(rxLoad) + "%)";
    }
    if(rxLoad > HIGH_LOAD_WARNING)
    {
        //电脑串口有缓冲，因此会出现使用率大于100%的情况
        if(rxLoad > 100)
        {
            rxSpeedStr = "<font color=#FF0000>" + rxSpeedStr + "</font>";
        }
        else
        {
            rxSpeedStr = "<font color=#FF5A5A>" + rxSpeedStr + "</font>";
        }
    }
    //网络模式下不显示负载率
    if(g_network_comm_mode)
    {
        if(rxSpeedStr.indexOf("(0%)"))
            rxSpeedStr.remove(rxSpeedStr.indexOf("(0%)"), 4);
        if(txSpeedStr.indexOf("(0%)"))
            txSpeedStr.remove(txSpeedStr.indexOf("(0%)"), 4);
    }
    statusSpeedLabel->setText(txSpeedStr + rxSpeedStr);
}

/**
 * @brief     秒定时器
 * @note      刷新一些更新频率低的数据，如收发统计、HTTP信息、周期备份数据供恢复等
 */
void MainWindow::secTimerSlot()
{
    static int64_t secCnt = 0;
    static int32_t msgIndex = 0;

    //readSerialPort每秒执行次数统计，用于流控
    diff_slotCnt = readSlotCnt - lastReadSlotCnt;
    lastReadSlotCnt = readSlotCnt;

    //收发速度显示与颜色控制
    TxRxSpeedStatisticAndDisplay();

    //显示远端下载的信息
    if(http)
    {
        if(http->getMsgList().size() > 0 && secCnt % 10 == 0)
        {
            statusRemoteMsgLabel->setText(http->getMsgList().at(msgIndex++));
            if(msgIndex == http->getMsgList().size())
                msgIndex = 0;
        }
    }

    if(ui->comSwitch->isChecked() || ui->networkSwitch->isChecked())
    {
        qint64 consumedTime = QDateTime::currentSecsSinceEpoch() - g_lastSecsSinceEpoch;
        statusTimer->setText("Timer:" + formatTime(consumedTime * 1000));
    }
    else
    {
        g_lastSecsSinceEpoch = QDateTime::currentSecsSinceEpoch();
    }

    //点击暂停刷新后能及时保持和更新statusBar信息
    if(disableRefreshWindow)
    {
        disableRefreshWindow_triggered(true);
    }

    //数据记录器定时保存数据
    emit logger_flush(RECOVERY_LOG);
    if(!rawDataRecordPath.isEmpty())
    {
        emit logger_flush(RAW_DATA_LOG);
    }
    if(!graphDataRecordPath.isEmpty())
    {
        emit logger_flush(GRAPH_DATA_LOG);
    }
    secCnt++;
    currentRunTime++;
}

/**
 * @brief     调试定时器
 * @note      一般用户接触不到这段代码
 */
#define DEBUG_TIMER_FRQ (128)
static double debugTimerSlotCnt = 0;
void MainWindow::debugTimerSlot()
{
    #define BYTE0(dwTemp)   static_cast<char>((*reinterpret_cast<char *>(&dwTemp)))
    #define BYTE1(dwTemp)   static_cast<char>((*(reinterpret_cast<char *>(&dwTemp) + 1)))
    #define BYTE2(dwTemp)   static_cast<char>((*(reinterpret_cast<char *>(&dwTemp) + 2)))
    #define BYTE3(dwTemp)   static_cast<char>((*(reinterpret_cast<char *>(&dwTemp) + 3)))

    float num1, num2, num3, num4;
    //正弦
    #define PI  (3.141592653)
    #define FRQ (6)
    num1 = static_cast<float>(1 + qSin(2 * PI * (FRQ-FRQ) * (debugTimerSlotCnt / DEBUG_TIMER_FRQ)));
    num2 = static_cast<float>(qSin(2 * PI * (FRQ+0) * (debugTimerSlotCnt / DEBUG_TIMER_FRQ)));
    num3 = static_cast<float>(0.8 * qSin(2 * PI * (FRQ-2) * (debugTimerSlotCnt / DEBUG_TIMER_FRQ))) +
           static_cast<float>(0.8 * qSin(2 * PI * (FRQ+2) * (debugTimerSlotCnt / DEBUG_TIMER_FRQ)));
    num4 = static_cast<float>(1.2 * qSin(2 * PI * (FRQ-4) * (debugTimerSlotCnt / DEBUG_TIMER_FRQ))) +
           static_cast<float>(1.2 * qSin(2 * PI * (FRQ+4) * (debugTimerSlotCnt / DEBUG_TIMER_FRQ)));
    if(ui->actionAscii->isChecked()){
        if(plotterManager.getDefaultPlotter()->plotControl->getEnableTimeStampMode())
            num1 = debugTimerSlotCnt;
        QString tmp;
        tmp = "{plotter:" +
                  QString::number(static_cast<double>(num1),'f') + "," +
                  QString::number(static_cast<double>(num2),'f') + "," +
                  QString::number(static_cast<double>(num3),'f') + "," +
                  QString::number(static_cast<double>(num4),'f') + "}\n";

        tmp += "{voltage:the vol is ### V}\n"
               "{cnt:the cnt is $$$}\n";
        tmp.replace("###", QString::number(3.3 + qrand()/static_cast<double>(RAND_MAX)/10.0, 'f', 3));
        tmp.replace("$$$", QString::number(static_cast<qint32>(debugTimerSlotCnt)));
        writeDataToDevice(tmp.toLocal8Bit());
    }else if(ui->actionFloat->isChecked()){
        QByteArray tmp;
        tmp.append(BYTE0(num1));tmp.append(BYTE1(num1));tmp.append(BYTE2(num1));tmp.append(BYTE3(num1));
        tmp.append(BYTE0(num2));tmp.append(BYTE1(num2));tmp.append(BYTE2(num2));tmp.append(BYTE3(num2));
        tmp.append(BYTE0(num3));tmp.append(BYTE1(num3));tmp.append(BYTE2(num3));tmp.append(BYTE3(num3));
        tmp.append(BYTE0(num4));tmp.append(BYTE1(num4));tmp.append(BYTE2(num4));tmp.append(BYTE3(num4));
        tmp.append(static_cast<char>(0x00));tmp.append(static_cast<char>(0x00));tmp.append(static_cast<char>(0x80));tmp.append(static_cast<char>(0x7F));

        writeDataToDevice(tmp);
    }

    debugTimerSlotCnt = debugTimerSlotCnt + 1;
}

/**
 * @brief     析构函数
 * @note      主要是保存配置、释放资源等
 */
MainWindow::~MainWindow()
{
    if(needSaveConfig && g_agree_statement){
        Config::setFirstRun(false);
        Config::setLogRecord(g_log_record);
        if(ui->actionUTF8->isChecked()){
            Config::setCodeRule(CodeRule_e::UTF8);
        }else if(ui->actionGBK->isChecked()){
            Config::setCodeRule(CodeRule_e::GBK);
        }
        if(ui->action_winLikeEnter->isChecked()){
            Config::setEnterStyle(EnterStyle_e::WinStyle);
        }else if(ui->action_unixLikeEnter->isChecked()){
            Config::setEnterStyle(EnterStyle_e::UnixStyle);
        }

        //global
        Config::setHexSendState(ui->hexSend->isChecked());
        Config::setHexShowState(ui->hexDisplay->isChecked());
        Config::setSendInterval(ui->sendInterval->text().toInt());
        Config::setTimeStampState(ui->timeStampCheckBox->isChecked());
        Config::setTimeStampTimeOut(ui->timeStampTimeOut->text().toInt());
        Config::setConfigBool(SECTION_GLOBAL, KEY_SIMPLE_MODE,
                            ui->actionSimpleMode->isChecked());
        Config::setMultiStringState(ui->actionMultiString->isChecked());
        Config::setKeyWordHighlightState(ui->actionKeyWordHighlight->isChecked());
        Config::setTextSendArea(ui->textEdit->toPlainText());
        Config::setVersion();
        QStringList multi;
        for(qint32 i = 0; i < ui->multiString->count(); i++){
            multi.append(ui->multiString->item(i)->text());
        }
        Config::setMultiString(multi);
        Config::setLastFileDialogPath(lastFileDialogPath);
        Config::setGUIFont(g_font);
        Config::setBackGroundColor(g_background_color);
        Config::setConfigNumber(SECTION_GLOBAL, KEY_THEME_INDEX, g_theme_index);
        Config::setPopupHotKey(g_popupHotKeySequence);
        Config::setTeeSupport(textExtractEnable);
        Config::setTeeLevel2NameSupport(p_textExtract->getLevel2NameSupport());
        Config::setConfigString(SECTION_GLOBAL, KEY_ACTIVATED_TAB,
                                ui->tabWidget->tabText(ui->tabWidget->currentIndex()));
        Config::setConfigString(SECTION_GLOBAL, KEY_REG_MATCH_STR,
                                ui->regMatchEdit->text());
        //网络
        Config::setConfigNumber(SECTION_GLOBAL, KEY_WORKMODE, g_network_comm_mode);
        Config::setConfigString(SECTION_NETWORK, KEY_NETWORK_MODE, ui->networkModeBox->currentText());
        QString udps_addr_list;
        udps_addr_list = ui->comboBox_remoteAddr->currentText();
        Config::setConfigString(SECTION_NETWORK, KEY_UDPS_REMOTE_ADDR, udps_addr_list);
        QString target_addr_list;
        target_addr_list = ui->comboBox_targetIP->currentText();
        Config::setConfigString(SECTION_NETWORK, KEY_CLIENT_TARGET_IP, target_addr_list);
        QString port_list;
        port_list = ui->comboBox_targetPort->currentText();
        Config::setConfigString(SECTION_NETWORK, KEY_CLIENT_TARGET_PORT, port_list);

        //serial 只保存成功打开过的
        Config::setPortName(serial.portName());
        Config::setBaudrate(serial.baudRate());
        Config::setDataBits(serial.dataBits());
        Config::setStopBits(serial.stopBits());
        Config::setParity(serial.parity());
        Config::setFlowControl(serial.flowControl());

        //plotter
        Config::setPlotterState(ui->actionPlotterSwitch->isChecked());
        if(ui->actionAscii->isChecked()){
            if(ui->actionSumCheck->isChecked())
                Config::setPlotterType(ProtocolType_e::Ascii_SumCheck);
            else
                Config::setPlotterType(ProtocolType_e::Ascii);
        }
        else if(ui->actionFloat->isChecked()){
            if(ui->actionSumCheck->isChecked())
                Config::setPlotterType(ProtocolType_e::Float_SumCheck);
            else
                Config::setPlotterType(ProtocolType_e::Float);
        }
        else if(ui->actionCSV->isChecked())
        {
            if(ui->actionSumCheck->isChecked())
                Config::setPlotterType(ProtocolType_e::CSV_SumCheck);
            else
                Config::setPlotterType(ProtocolType_e::CSV);
        }
        else if(ui->actionMAD->isChecked())
        {
            if(ui->actionSumCheck->isChecked())
                Config::setPlotterType(ProtocolType_e::MAD_SumCheck);
            else
                Config::setPlotterType(ProtocolType_e::MAD);
        }
        Config::setValueDisplayState(ui->actionValueDisplay->isChecked());
        //保存默认绘图器配置，不能用ui->customPlot了
        MyQCustomPlot* defaultPlotter = plotterManager.selectPlotter(plotProtocol->getDefaultPlotterTitle());
        if(defaultPlotter)
        {
            Config::setPlotterGraphNames(defaultPlotter->plotControl->getNameSets());
            if(defaultPlotter->getxAxisSource() == XAxis_Cnt)  //暂时不支持存储XY图模式的X轴名字
                Config::setXAxisName(defaultPlotter->xAxis->label());
            Config::setYAxisName(defaultPlotter->yAxis->label());
            Config::setRefreshYAxisState(defaultPlotter->getAutoRescaleYAxis());
            Config::setLineType(defaultPlotter->plotControl->getLineType());
            Config::setOpengGLState(defaultPlotter->getUseOpenGLState());
        }
        //默认绘图器名称
        Config::setConfigString(SECTION_GLOBAL, KEY_DEFAULT_PLOT_TITLE, plotProtocol->getDefaultPlotterTitle());

        //static
        Config::setLastRunTime(currentRunTime);
        Config::setTotalRunTime(currentRunTime);
        Config::setLastTxCnt(serial.getTotalTxCnt());//getTotalTxCnt是本次软件运行时的总发送量
        Config::setTotalTxCnt(serial.getTotalTxCnt());
        Config::setLastRxCnt(serial.getTotalRxCnt());
        Config::setTotalRxCnt(serial.getTotalRxCnt());
        Config::setTotalRunCnt(1);
        Config::addCurrentStatistic(KEY_TOTALPLOTTERUSE, statisticPlotterUseCnt);
        Config::addCurrentStatistic(KEY_TOTALPLOTTERNUM, statisticPlotterNumCnt);
        Config::addCurrentStatistic(KEY_TOTALVALUEDISPLAYUSE, statisticValueDisplayUseCnt);
        Config::addCurrentStatistic(KEY_TOTALFFTUSE, statisticFFTUseCnt);
        Config::addCurrentStatistic(KEY_TOTALMULTISTRUSE, statisticMultiStrUseCnt);
        Config::addCurrentStatistic(KEY_TOTALASCIITABLEUSE, statisticAsciiTableUseCnt);
        Config::addCurrentStatistic(KEY_TOTALPRIORITYTABLEUSE, statisticPriorityTableUseCnt);
        Config::addCurrentStatistic(KEY_TOTALSTM32ISPUSE, statisticStm32IspUseCnt);
        Config::addCurrentStatistic(KEY_TOTALHEXTOOLUSE, statisticHexToolUseCnt);
        Config::addCurrentStatistic(KEY_TOTALASCIILUSE, statisticASCIIUseCnt);
        Config::addCurrentStatistic(KEY_TOTALFLOATUSE, statisticFLOATUseCnt);
        Config::addCurrentStatistic(KEY_TOTALCSVUSE, statisticCSVUseCnt);
        Config::addCurrentStatistic(KEY_TOTALMADUSE, statisticMADUseCnt);
        Config::addCurrentStatistic(KEY_TOTALTEEUSE, statisticTeeUseCnt);
        Config::addCurrentStatistic(KEY_TOTALTEEPARSE, statisticTeeParseCnt);
        Config::addCurrentStatistic(KEY_TOTALREGPARSE, statisticRegParseCnt);
        Config::addCurrentStatistic(KEY_TOTALRECORDUSE, statisticRecordCnt);
    }else{
        if(g_agree_statement)
        {
            Config::writeDefault();
        }
    }

    //退出时做一次flush操作
    p_logger->logger_buff_flush(RECOVERY_LOG);
    if(!rawDataRecordPath.isEmpty())
    {
        p_logger->logger_buff_flush(RAW_DATA_LOG);
    }
    if(!graphDataRecordPath.isEmpty())
    {
        p_logger->logger_buff_flush(GRAPH_DATA_LOG);
    }

    plotProtocol_thread->quit();
    plotProtocol_thread->wait();
//    delete plotProtocol_thread; //deleteLater自动删除？
    delete plotProtocol_thread;

    p_textExtractThread->quit();
    p_textExtractThread->wait();
//    delete p_textExtract; //deleteLater自动删除？
    delete p_textExtractThread;

    p_regMatchThread->quit();
    p_regMatchThread->wait();
//    delete p_textExtract; //deleteLater自动删除？
    delete p_regMatchThread;

    p_logger_thread->quit();
    p_logger_thread->wait();
//    delete p_logger_thread; //deleteLater自动删除？
    delete p_logger_thread;

    p_networkCommThread->quit();
    p_networkCommThread->wait();
    delete p_networkCommThread;

    delete fft_window;
    fft_window = nullptr;

    delete highlighter;
    delete highlighter1;
    delete ui;
    delete http;
    delete g_popupHotkey;
    delete central;

    QFile recoveryFile(RECOVERY_FILE_PATH);
    if(recoveryFile.exists())
    {
        recoveryFile.remove();
    }
    Config::writeCommentMsgAtFileTop();
    qDebug()<<"~MainWindow";
}

/**
 * @brief     刷新串口
 */
void MainWindow::refreshCom()
{
    //测试更新下拉列表
    mySerialPort *testSerial = new mySerialPort;
    QList<QString> tmp;

    tmp = testSerial->refreshSerialPort();
    //刷新串口状态，需要记录当前选择的条目用于刷新后恢复
    QString portName = ui->comList->currentText().mid(0, ui->comList->currentText().indexOf('('));
    ui->comList->clear();
    foreach(const QString &info, tmp)
    {
        ui->comList->addItem(info);
    }
    if(ui->comList->count() == 0)
        ui->comList->addItem(tr("未找到可用串口，点我刷新！"));

    //恢复刷新前的选择
    ui->comList->setCurrentIndex(0);
    for(qint32 i = 0; i < ui->comList->count(); i++)
    {
        if(ui->comList->itemText(i).startsWith(portName))
        {
            ui->comList->setCurrentIndex(i);
            break;
        }
    }

    delete testSerial;
}

/**
 * @brief     刷新串口按钮的槽
 * @note      不通过点击事件直接调用好像会崩溃，有待排查
 */
void MainWindow::on_refreshCom_clicked()
{
    if(ui->refreshCom->isEnabled() == false){
        ui->statusBar->showMessage(tr("刷新功能被禁用"), 2000);
        return;
    }

    //扫描提示
    ui->statusBar->showMessage(tr("正在扫描可用串口……"), 2000);
    qApp->processEvents();

    refreshCom();

    ui->comList->showPopup();

    //扫描提示
    ui->statusBar->showMessage(tr("串口扫描完毕"), 1000);
}

/**
 * @brief     在只有一个串口时进行尝试打开
 */
void MainWindow::tryOpenSerial()
{
    //只存在一个串口时且串口未被占用自动打开
    if(ui->comList->count() == 1 &&
       ui->comList->currentText().indexOf(tr("BUSY")) == -1 &&
       ui->comList->currentText() != tr("未找到可用串口，点我刷新！"))
    {
        //同时还要没检测到恢复文件
        QFile recoveryFile(RECOVERY_FILE_PATH);
        if(recoveryFile.exists())
        {
            return;
        }
        ui->refreshCom->setChecked(false);
        ui->comSwitch->setChecked(true);
        on_comSwitch_clicked(true);
    }
    else
    {
        //如果有多个串口，则尝试选择（不开启）上次使用的端口号
        if(ui->comList->count() > 1)
        {
            QString name = Config::getPortName();
            for(qint32 i = 0; i < ui->comList->count(); i++)
            {
                if(ui->comList->itemText(i).startsWith(name))
                {
                    ui->comList->setCurrentIndex(i);
                    break;
                }
            }
        }
    }
}

/**
 * @brief     串口开关按钮的槽
 */
void MainWindow::on_comSwitch_clicked(bool checked)
{
    if(g_network_comm_mode)
        return;

    QString com = ui->comList->currentText().mid(0,ui->comList->currentText().indexOf('('));
    qint32 baud = ui->baudrateList->currentText().toInt();

    if(checked)
    {
        if(serial.open(com, baud)){
            ui->comSwitch->setText(tr("关闭串口"));
            ui->comSwitch->setChecked(true);
            g_lastSecsSinceEpoch = QDateTime::currentSecsSinceEpoch();
            qint64 consumedTime = QDateTime::currentSecsSinceEpoch() - g_lastSecsSinceEpoch;
            statusTimer->setText("Timer:" + formatTime(consumedTime * 1000));
        }
        else {
            ui->comSwitch->setText(tr("打开串口"));
            ui->comSwitch->setChecked(false);
            QString msg = tr("请检查下列情况后重新打开串口：\n\n")+
                          tr("# 线缆是否松动？\n")+
                          tr("# 是否选择了正确的串口设备？\n")+
                          tr("# 该串口是否被其他程序占用？\n")+
                          tr("# 是否设置了过高的波特率？\n");
            QMessageBox::critical(this, tr("串口打开失败!"), msg, QMessageBox::Ok);
        }
    }
    else
    {
        //关闭定时器
        if(cycleSendTimer.isActive()){
            cycleSendTimer.stop();
            ui->cycleSendCheck->setChecked(false);
        }

        serial.close();
        ui->comSwitch->setText(tr("打开串口"));
        ui->comSwitch->setChecked(false);
    }

    refreshCom();
}

/**
 * @brief     读取串口数据
 * @note      串口对象的数据更新的槽
 */
void MainWindow::readSerialPort()
{
    static QByteArray tmpReadBuff;
    QByteArray floatParseBuff;//用于绘图协议解析的缓冲。其中float协议不处理中文

    //先获取时间，避免解析数据导致时间消耗的影响
    QString timeString;
    timeString = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    timeString = "\n["+timeString+"]Rx<- ";

    //本函数执行统计，用于流控
    readSlotCnt++;

    //解析文件模式
    if(readFile){
        tmpReadBuff.append(readFileBuff);
    }
    else{
        //网络或者串口模式
        if(!g_network_comm_mode)
        {
            tmpReadBuff.append(serial.readAll());
        }
        else
        {
            int32_t len = networkRxBuff.size();
            tmpReadBuff.append(networkRxBuff.mid(0, len));
            networkRxBuff.remove(0, len);
        }
    }

    //空数据检查
    if(tmpReadBuff.isEmpty()){
        return;
    }

    //流控：当未勾选时间戳时可能启用流控，以降低后面的emit数量，大约能提升20%的性能
    if(!ui->timeStampCheckBox->isChecked())
    {
        //每秒信号数量大于x时每y次slot解析一次buffer
        if(diff_slotCnt > 1400 && readSlotCnt % 5)
            return;
        else if(diff_slotCnt > 800 && readSlotCnt % 4)
            return;
        else if(diff_slotCnt > 400 && readSlotCnt % 3)
            return;
        else if(diff_slotCnt > 200 && readSlotCnt % 2)
            return;
    }

    RxBuff.append(tmpReadBuff);

    //速度统计，不能和下面的互换，否则不准确
    statisticRxByteCnt += tmpReadBuff.size();

    //读取数据并衔接到上次未处理完的数据后面
    tmpReadBuff = unshowedRxBuff + tmpReadBuff;
    unshowedRxBuff.clear();

    //'\r'若单独结尾则可能被误切断，放到下一批数据中
    if(tmpReadBuff.endsWith('\r'))
    {
        unshowedRxBuff.append(tmpReadBuff.at(tmpReadBuff.size() - 1));
        tmpReadBuff.remove(tmpReadBuff.size() - 1, 1);
        if(tmpReadBuff.size() == 0)
            return;
    }

    //如果不是hex显示则要考虑中文处理
    if(ui->hexDisplay->isChecked() == false)
    {
        //只需要保证上屏的最后一个字节的高位不是1即可
        if(tmpReadBuff.back() & 0x80)
        {
            qint32 reversePos = tmpReadBuff.size() - 1;
            while(tmpReadBuff.at(reversePos) & 0x80)//不超过3次循环
            {
                reversePos--;
                if(reversePos < 0)
                    break;
            }
            unshowedRxBuff = tmpReadBuff.mid(reversePos + 1);
            tmpReadBuff = tmpReadBuff.mid(0,reversePos + 1);
        }
        //如果unshowedRxBuff正好是相关编码长度的倍数，则可以上屏
        if((ui->actionGBK->isChecked() && unshowedRxBuff.size() % 2 == 0) ||
           (ui->actionUTF8->isChecked() && unshowedRxBuff.size() % 3 == 0))
        {
            tmpReadBuff.append(unshowedRxBuff);
            unshowedRxBuff.clear();
        }
    }

    //数据交付给文本解析引擎(追加数据和解析分开防止高频解析带来的CPU压力)
    if(textExtractEnable)
        emit tee_appendData(tmpReadBuff);

    //数据交付正则匹配引擎
    if(!ui->regMatchEdit->text().isEmpty())
        emit regM_appendData(tmpReadBuff);

    //数据交付绘图解析引擎
    if(ui->actionPlotterSwitch->isChecked() ||
       ui->actionValueDisplay->isChecked() ||
       ui->actionFFTShow->isChecked())
    {
        emit protocol_appendData(tmpReadBuff);
    }

    emit logger_append(RECOVERY_LOG, tmpReadBuff);
    if(!rawDataRecordPath.isEmpty())
    {
        emit logger_append(RAW_DATA_LOG, tmpReadBuff);
    }

    //时间戳选项(toHexDisplay是比较耗时的，不建议用于带宽高于2Mbps的场景)
    if(ui->timeStampCheckBox->isChecked() && timeStampTimer.isActive()==false){
        //hex解析
        hexBrowserBuff.append(timeString + toHexDisplay(tmpReadBuff).toLatin1());//换行符在前面判断没有数据时自动追加一次
        //asic解析，显示的数据一律不要\r。且进行\0显示的检查
        while(tmpReadBuff.indexOf("\r\n")!=-1){
            tmpReadBuff.replace("\r\n","\n");
        }
        while(tmpReadBuff.indexOf('\0')!=-1){
            tmpReadBuff.replace('\0',"\\0");
        }
        BrowserBuff.append(timeString + QString::fromLocal8Bit(tmpReadBuff));//换行符在前面判断没有数据时自动追加一次
        timeStampTimer.setSingleShot(true);
        timeStampTimer.start(ui->timeStampTimeOut->text().toInt());
    }else{
        //hex解析
        hexBrowserBuff.append(toHexDisplay(tmpReadBuff).toLatin1());
        //asic解析，显示的数据一律不要\r。且进行\0显示的检查
        while(tmpReadBuff.indexOf("\r\n")!=-1){
            tmpReadBuff.replace("\r\n","\n");
        }
        while(tmpReadBuff.indexOf('\0')!=-1){
            tmpReadBuff.replace('\0',"\\0");
        }
        BrowserBuff.append(QString::fromLocal8Bit(tmpReadBuff));
    }

    //允许数据刷新
    TryRefreshBrowserCnt = TRY_REFRESH_BROWSER_CNT;

    tmpReadBuff.clear();
}


/**
 * @brief     打印数据到TextBrowser
 * @note      为了降低CPU占用只打印了最新的一部分少量数据
 */
static int32_t PAGING_SIZE = 4096; //TextBrowser显示大小
void MainWindow::printToTextBrowser()
{
    //当前窗口显示字符调整
    if(characterCount == 0 || windowSize != ui->textBrowser->size())
    {
        windowSize = ui->textBrowser->size();
        calcCharacterNumberInWindow();
        printToTextBrowser();
    }

    //暂停刷新则不刷新
    if(disableRefreshWindow ||
       ui->tabWidget->tabText(ui->tabWidget->currentIndex()) != MAIN_TAB_NAME)
    {
        return;
    }

    //多显示一点并对齐到characterCount_Col的倍数
    if(ui->hexDisplay->isChecked())
        PAGING_SIZE = characterCount * 1.2 + 1; //characterCount 小于4时乘1.2还是等于4所以要加1确保增加
    else
        PAGING_SIZE = characterCount * 1.2 + 1;
    if(PAGING_SIZE > characterCount_Col)
        PAGING_SIZE = PAGING_SIZE - PAGING_SIZE % characterCount_Col;
    else
        PAGING_SIZE = characterCount_Col;
    //满足gbk/utf8编码长度的倍数
    if(ui->actionGBK->isChecked())
    {
        PAGING_SIZE = PAGING_SIZE - PAGING_SIZE % 2;
    }else if(ui->actionUTF8->isChecked())
    {
        PAGING_SIZE = PAGING_SIZE - PAGING_SIZE % 3;
    }

    //打印数据
    ui->textBrowser->moveCursor(QTextCursor::End);
    if(ui->hexDisplay->isChecked()){
        if(hexBrowserBuff.size() < PAGING_SIZE){
            ui->textBrowser->setPlainText(hexBrowserBuff);
            hexBrowserBuffIndex = hexBrowserBuff.size();
        }else{
            ui->textBrowser->setPlainText(hexBrowserBuff.mid(hexBrowserBuff.size()-PAGING_SIZE));
            hexBrowserBuffIndex = PAGING_SIZE;
        }
    }else{
        if(BrowserBuff.size() < PAGING_SIZE){
            ui->textBrowser->setPlainText(BrowserBuff);
            BrowserBuffIndex = BrowserBuff.size();
        }else{
            //about 20ms
            ui->textBrowser->setPlainText(BrowserBuff.mid(BrowserBuff.size()-PAGING_SIZE));
            BrowserBuffIndex = PAGING_SIZE;
        }
    }

    ui->textBrowser->verticalScrollBar()->setValue(ui->textBrowser->verticalScrollBar()->maximum());
    ui->textBrowser->moveCursor(QTextCursor::End);
}

/**
 * @brief     修复文本大量留白的问题
 * @note      当缓冲数量大于窗口显示数量时，由于只显示了部分数据，
 *            在缺少换行符或者纯中文的情况下，就显示出留白问题。
 *            若存在留白现象，则滚动条数值变小，甚至消失，以此进行判断。
 * @return    是否进行了处理
 */
int32_t MainWindow::checkBlankProblem()
{
    int32_t ret = 0;

    if(ui->textBrowser->verticalScrollBar()->maximum() != 0)
        return ret;

    if(ui->hexDisplay->isChecked())
    {
        if(hexBrowserBuff.size() > characterCount)
        {
            if(characterCount == 0)
            {
                characterCount++;
            }
            characterCount = characterCount * 1.2 + 1;//characterCount 小于4时乘1.2还是等于4所以要加1确保增加
            if(characterCount > hexBrowserBuff.size())
                characterCount = hexBrowserBuff.size();
            printToTextBrowser();
            ret = 1;
        }
    }
    else
    {
        if(BrowserBuff.size() > characterCount)
        {
            if(characterCount == 0)
            {
                characterCount++;
            }
            characterCount = characterCount * 1.2 + 1;//characterCount 小于4时乘1.2还是等于4所以要加1确保增加
            if(characterCount > BrowserBuff.size())
                characterCount = BrowserBuff.size();
            printToTextBrowser();
            ret = 1;
        }
    }

    return ret;
}

/**
 * @brief     检查滚动条是否太多，如果是则进行处理
 * @note      滚动条太多时数据刷新会很慢
 * @return    是否进行了处理
 */
int32_t MainWindow::checkScrollBarTooLarge()
{
    #define MAX_BAR_VALUE (20)
    int32_t ret = 0;

    if(ui->textBrowser->verticalScrollBar()->maximum() < MAX_BAR_VALUE)
        return 0;

    float coef = ui->textBrowser->verticalScrollBar()->maximum() / MAX_BAR_VALUE;
    if(coef > 1)
    {
        characterCount = characterCount / coef;
        printToTextBrowser();
        ret = 1;
    }
    // else
    // {
    //     characterCount -= 1;
    //     if(characterCount < 0)
    //         characterCount = 0;
    // }

    return ret;
}

/**
 * @brief     串口对象发送了多少字节
 * @note      用于记录收发统计和进度条
 * @param[in] 发送的字节
 */
void MainWindow::serialBytesWritten(qint64 bytes)
{
    //发送速度统计
    statisticTxByteCnt += bytes;
}

/**
 * @brief     处理串口错误
 * @note      串口松动、拔出检测
 * @param[in] 错误码
 */
void MainWindow::handleSerialError(QSerialPort::SerialPortError errCode)
{
    if(g_network_comm_mode)
        return;

    //故障检测
    if(errCode == QSerialPort::ResourceError){
        //记录故障的串口号
        QString portName = serial.portName();
        //关闭串口
        on_comSwitch_clicked(false);
        //强提醒也争取了时间，如果是短时间松动，则点击确定后可以恢复所选的端口
        QMessageBox::warning(this, tr("警告"),
                             tr("检测到串口故障，已关闭串口。\n串口是否发生了松动？"));
        //【还要】再刷新一次并展开下拉列表
        on_refreshCom_clicked();
        //尝试恢复所选端口号
        for(qint32 i = 0; i < ui->comList->count(); i++){
            if(ui->comList->itemText(i).startsWith(portName)){
                ui->comList->setCurrentIndex(i);
            }
        }
    }
//    qDebug()<<"handleSerialError"<<errCode;
}

/**
 * @brief     连续发送定时器的槽
 * @note      用于周期发送功能
 */
void MainWindow::cycleSendTimerSlot()
{
    on_sendButton_clicked();
}

/**
 * @brief     从字符串中提取序列发送延迟时间
 * @note      用于多字符串组件的序列发送功能
 * @param[in] 要提取的字符串
 * @return    提取结果
 */
qint32 extractSeqenceTime(QString &str)
{
    QString temp;
    qint32 seqTime = -1;
    bool ok;
    QString comment;
    if(str.indexOf('|') != -1)
    {
        comment = str.mid(0, str.indexOf('|'));
    }
    if(comment.indexOf('[')==-1)
    {
        return -1;
    }
    temp = comment.mid(comment.indexOf('['));

    if(comment.indexOf(']')==-1)
    {
        return -1;
    }
    temp = temp.mid(1, temp.indexOf(']')-1);

    seqTime = temp.toInt(&ok);

    if(!ok || seqTime < 0)
    {
        return -1;
    }

    return seqTime;
}

/**
 * @brief     多字符串序列发送定时器的槽
 */
void MainWindow::multiStrSeqSendTimerSlot()
{
    if (!ui->actionMultiString->isChecked())
    {
        ui->clearWindows->setText(tr("清  空"));
        ui->clearWindows_simple->setText(ui->clearWindows->text());
        ui->clearWindows_simple_net->setText(ui->clearWindows->text());
        multiStrSeqSendTimer.stop();
        return;
    }
    if(ui->multiString->item(g_multiStr_cur_index + 1) == nullptr)
    {
        g_multiStr_cur_index = -1;
        ui->clearWindows->setText(tr("清  空"));
        ui->clearWindows_simple->setText(ui->clearWindows->text());
        ui->clearWindows_simple_net->setText(ui->clearWindows->text());
        multiStrSeqSendTimer.stop();
    }

    //剔除注释,并补上没有注释的字符串
    QString tmp = ui->multiString->item(++g_multiStr_cur_index)->text();
    if(tmp.indexOf('|') != -1)
    {
        tmp = tmp.mid(tmp.indexOf('|') + 1);
    }
    else
    {
        qint32 i = 0;
        for(i = 0; i < ui->multiString->count(); i++)
        {
            if(ui->multiString->item(i) == ui->multiString->item(g_multiStr_cur_index))
            {
                break;
            }
        }
        ui->multiString->item(g_multiStr_cur_index)->setText("CMD_" +
                                                              QString::number(i) + " |" +
                                                              tmp);
    }
    ui->textEdit->setText(tmp);
    on_sendButton_clicked();
}

/**
 * @brief     解析绘图器和分窗文本
 * @note      通常周期执行
 */
void MainWindow::parsePlotterAndTee()
{
    //触发文本提取引擎解析
    if(textExtractEnable)
        emit tee_parseData();

    //触发正则匹配引擎解析
    emit regM_parseData();

    //触发绘图器解析
    if(ui->actionPlotterSwitch->isChecked() ||
       ui->actionValueDisplay->isChecked() ||
       ui->actionFFTShow->isChecked())
    {
        emit protocol_parseData(g_enableSumCheck);
    }
}

/**
 * @brief     100Hz定时器
 * @note      一些代码可能需要多执行几次确保数据都处理完了
 */
void MainWindow::parseTimer100hzSlot()
{
    static uint32_t cnt = 0;

    if(cnt % (PLOTTER_SHOW_PERIOD/10) == 0)
    {
        //协议解析控制
        if(TryRefreshBrowserCnt == DO_NOT_REFRESH_BROWSER)
            return;

        parsePlotterAndTee();
    }
    cnt++;
}

/**
 * @brief     添加数据到多字符串组件中
 * @param[in] 添加的数据
 */
void MainWindow::addTextToMultiString(const QString &text)
{
    bool hasItem=false;
    QString containt;
    //比较发送的数据是否已经在多字符串列表中，顺便把没有注释的字符串补上注释
    for(qint32 i = 0; i < ui->multiString->count(); i++){
        containt = ui->multiString->item(i)->text();
        if(containt.indexOf('|') != -1)
        {
            containt = containt.mid(containt.indexOf('|') + 1);
        }
        else
        {
            ui->multiString->item(i)->setText("CMD_" +
                                                QString::number(i) + " |" +
                                                containt);
        }
        if(containt == text)
        {
            hasItem = true;
        }
    }
    if(!hasItem)
        ui->multiString->addItem("CMD_" +
                                QString::number(ui->multiString->count()) + " |" +
                                text);
}

/**
 * @brief     发送按钮点击
 */
void MainWindow::on_sendButton_clicked()
{
    QByteArray tmp;
    QString tail;

    if(!remindDeviceIsOpen())
        return;

    tmp = ui->textEdit->toPlainText().toLocal8Bit();

    //回车风格转换，win风格补上'\r'，默认unix风格
    if(ui->action_winLikeEnter->isChecked())
    {
        //win风格
        while (tmp.indexOf('\n') != -1) {
            tmp = tmp.replace('\n', '\t');
        }
        while (tmp.indexOf('\t') != -1) {
            tmp = tmp.replace('\t', "\r\n");
        }
    }
    else
    {
        //unix风格
        while (tmp.indexOf('\r') != -1) {
            tmp = tmp.remove(tmp.indexOf('\r'),1);
        }
    }

    //十六进制检查
    QByteArray sendArr; //真正发送出去的数据
    if(ui->hexSend->isChecked())
    {
        //以hex发送数据
        //HexStringToByteArray函数必须传入格式化后的字符串，如"02 31"
        bool ok;
        sendArr = HexStringToByteArray(tmp,ok); //hex转发送数据流
        if(ok){
            writeDataToDevice(sendArr);
        }else{
            ui->statusBar->showMessage(tr("文本输入区数据转换失败，放弃此次发送！"), 2000);
        }
    }
    else
    {
        sendArr = tmp;
        //utf8编码
        writeDataToDevice(sendArr);
    }

    //周期发送开启则立刻发送
    if(ui->cycleSendCheck->isChecked() && !g_network_comm_mode)
        serial.flush();

    //若添加了时间戳则把发送的数据也显示在接收区
    if(ui->timeStampCheckBox->isChecked())
    {
        QString timeString;
        timeString = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        timeString = "\n["+timeString+"]Tx-> ";

        hexBrowserBuff.append(timeString + toHexDisplay(sendArr));
        BrowserBuff.append(timeString + QString::fromLocal8Bit(sendArr));

        //打印数据
        printToTextBrowser();
    }

    //给多字符串控件添加条目, is_multi_str_double_click用于减少处理次数，高频发送时很有效
    if(ui->actionMultiString->isChecked() && !is_multi_str_double_click){
        addTextToMultiString(ui->textEdit->toPlainText());
    }
    is_multi_str_double_click = false;

    //更新收发统计(周期发送时收发统计直接通过周期定时器刷新以减少资源消耗)
    if(!ui->cycleSendCheck->isChecked())
    {
        if(g_network_comm_mode)
        {
            statusStatisticLabel->setText(p_networkComm->getTxRxString_with_color());
        }
        else
        {
            statusStatisticLabel->setText(serial.getTxRxString_with_color());
        }
    }

    //多字符串序列发送
    if(g_multiStr_cur_index != -1)
    {
        QString tmp = ui->multiString->item(g_multiStr_cur_index)->text();
        qint32 seqTime = extractSeqenceTime(tmp);
        if(seqTime > 0)
        {
            ui->clearWindows->setText(tr("中  止"));
            ui->clearWindows_simple->setText(ui->clearWindows->text());
            ui->clearWindows_simple_net->setText(ui->clearWindows->text());
            multiStrSeqSendTimer.start(seqTime);
        }
        else
        {
            g_multiStr_cur_index = -1;
            ui->clearWindows->setText(tr("清  空"));
            ui->clearWindows_simple->setText(ui->clearWindows->text());
            ui->clearWindows_simple_net->setText(ui->clearWindows->text());
            multiStrSeqSendTimer.stop();
        }
    }
}

/**
 * @brief     清空按钮点击
 */
void MainWindow::on_clearWindows_clicked()
{
    ui->clearWindows->setText(tr("清  空"));
    ui->clearWindows_simple->setText(ui->clearWindows->text());
    ui->clearWindows_simple_net->setText(ui->clearWindows->text());

    progressBar->setValue(0);
    progressBar->setVisible(false);

    //多字符串序列发送定时器，第二次才清空
    if(multiStrSeqSendTimer.isActive())
    {
        multiStrSeqSendTimer.stop();
        return;
    }

    //文件解析中止，第二次才清空
    if(readFile)
    {
        fileUnpacker->abort_unpack_file();
        readFile = false;
        openInteractiveUI();
        return;
    }

    //文件发送中止
    if(sendFile)
    {
        fileUnpacker->abort_unpack_file();
        sendFile = false;
        openInteractiveUI();
        return;
    }

    //定时器
    g_lastSecsSinceEpoch = QDateTime::currentSecsSinceEpoch();
    qint64 consumedTime = QDateTime::currentSecsSinceEpoch() - g_lastSecsSinceEpoch;
    statusTimer->setText("Timer:" + formatTime(consumedTime * 1000));

    //串口
    if(g_network_comm_mode)
    {
        p_networkComm->resetCnt();
    }
    else
    {
        serial.resetCnt();
        if(serial.isOpen())
            serial.flush();
    }

    //接收区
    ui->textBrowser->clear();
    RxBuff.clear();
    hexBrowserBuff.clear();
    hexBrowserBuffIndex = 0;
    BrowserBuff.clear();
    BrowserBuffIndex = 0;
    unshowedRxBuff.clear();

    //正则匹配区
//    emit regM_clearData();
    p_regMatch->clearData();//若点击了从缓冲中过滤可能会由于数据量过大一直在while解析导致无法响应clear信号所以直接调用函数
    ui->regMatchBrowser->clear();
    regMatchBufferLock.lock();
    regMatchBuffer.clear();
    regMatchBufferLock.unlock();

    //文本提取区
    bool reset_to_mainwindow = true;
    if(ui->tabWidget->tabText(ui->tabWidget->currentIndex()) == REGMATCH_TAB_NAME)
    {
        //如果当前激活窗口是REGMATCH_TAB_NAME则清空后继续维持这个状态
        //否则重置回MAIN_TAB_NAME窗口
        reset_to_mainwindow = false;
    }
    emit tee_clearData("");//clear temp buff
    for(int32_t i = 0; i < ui->tabWidget->count(); i++)
    {
        if(ui->tabWidget->tabText(i) != MAIN_TAB_NAME &&
           ui->tabWidget->tabText(i) != REGMATCH_TAB_NAME )
        {
            emit tee_clearData(ui->tabWidget->tabText(i));
            if(ui->tabWidget->widget(i) != nullptr)
            {
                teeManager.removeTeeBrowser(ui->tabWidget->tabText(i));
                delete ui->tabWidget->widget(i);
            }
//            ui->tabWidget->removeTab(i);
            i = 0;//重置计数器
        }
    }
    if(reset_to_mainwindow)
    {
        //重置回MAIN_TAB_NAME窗口
        int32_t i = ui->tabWidget->count() - 1;
        for(; i > 0; i--)
        {
            if(ui->tabWidget->tabText(i) == MAIN_TAB_NAME)
                break;
        }
        ui->tabWidget->setCurrentIndex(i);
    }

    //绘图器相关，这个宏用于控制点击清空后是只清除数据还是把绘图器对象也删掉
#ifdef  CLEAR_AND_DELETE_PLOTTER
    QString selectName;
    MyQCustomPlot* plotter = nullptr;
    for(int32_t i = 0; i < ui->tabWidget_plotter->count(); i++)
    {
        selectName = ui->tabWidget_plotter->tabText(i);
        if(selectName != plotProtocol->getDefaultPlotterTitle())
        {
            emit protocol_clearBuff(selectName);
            plotterManager.removePlotter(selectName);
            if(ui->tabWidget_plotter->widget(i) != nullptr)
            {
                delete ui->tabWidget_plotter->widget(i);
            }
//            ui->tabWidget_plotter->removeTab(i);
            i = 0;//重置计数器
        }
    }
    plotter = selectCurrentPlotter();
    if(plotter)
    {
        emit protocol_clearBuff(plotter->getPlotterTitle());
        plotterManager.clearPlotter(plotter->getPlotterTitle());
        plotter->replot();
    }
#else
    plotterManager.clearAllPlotter();
    emit protocol_clearBuff("");
#endif

    //fft
    fft_window->clearGraphs();

    //数值显示器
    deleteValueDisplaySlot();

    //数据恢复仪
    p_logger->clear_logger(RECOVERY_LOG);

    //更新收发统计
    if(g_network_comm_mode)
    {
        statusStatisticLabel->setText(p_networkComm->getTxRxString_with_color());
    }
    else
    {
        statusStatisticLabel->setText(serial.getTxRxString_with_color());
    }

    //估计窗口容纳字符数量
    calcCharacterNumberInWindow();
}

/**
 * @brief     周期发送框被勾选
 * @param[in] 勾选结果
 */
void MainWindow::on_cycleSendCheck_clicked(bool checked)
{
    if(ui->sendInterval->text().toInt() == 0)
    {
        QMessageBox::information(this, tr("提示"), tr("发送间隔不允许设为0。"));
        ui->cycleSendCheck->setChecked(false);
        return;
    }

    if(ui->sendInterval->text().toInt() < 15 && checked)
    {
        ui->statusBar->showMessage(tr("发送间隔较小可能不够准确。"), 2000);
    }

    if(!remindDeviceIsOpen())
    {
        ui->cycleSendCheck->setChecked(false);
        return;
    }

    //启停定时器
    if(checked)
    {
        ui->cycleSendCheck->setChecked(true);
        cycleSendTimer.setTimerType(Qt::PreciseTimer);
        cycleSendTimer.start(ui->sendInterval->text().toInt());
    }
    else
    {
        ui->cycleSendCheck->setChecked(false);
        cycleSendTimer.stop();
    }
}

/**
 * @brief     发送区文本发生变化
 * @note      用于hex发送模式下检测非法字符
 */
void MainWindow::on_textEdit_textChanged()
{
    QString tmp;
    tmp = ui->textEdit->toPlainText();

    //十六进制发送下的输入格式检查
    static QString lastText;
    if(ui->hexSend->isChecked()){
        if(!hexFormatCheck(tmp)){
            if(g_multiStr_cur_index != -1)
            {
                QMessageBox::warning(this, tr("警告"),
                                    QString("The %1th multi_string has illegal hexadecimal format.")
                                    .arg(g_multiStr_cur_index));
            }
            else
            {
                QMessageBox::warning(this, tr("警告"), tr("hex发送模式下存在非法的十六进制格式。"));
            }
            multiStrSeqSendTimer.stop();
            ui->clearWindows->setText(tr("清  空"));
            ui->clearWindows_simple->setText(ui->clearWindows->text());
            ui->clearWindows_simple_net->setText(ui->clearWindows->text());
            ui->textEdit->clear();
            ui->textEdit->insertPlainText(lastText);
            return;
        }
        //不能记录非空数据，因为clear操作也会触发本事件
        if(!tmp.isEmpty())
            lastText = tmp;
    }
}

/**
 * @brief     hex发送按钮状态改变
 * @note      保存和恢复发送区的文本（hex模式和非hex模式各自维护一个发送区副本）
 * @param[in]
 * @return
 */
void MainWindow::on_hexSend_stateChanged(qint32 arg1)
{
    arg1++;
    static QString lastAsciiText, lastHexText;
    if(ui->hexSend->isChecked()){
        lastAsciiText = ui->textEdit->toPlainText();
        ui->textEdit->clear();
        ui->textEdit->insertPlainText(lastHexText);
    }
    else{
        lastHexText = ui->textEdit->toPlainText();
        ui->textEdit->clear();
        ui->textEdit->insertPlainText(lastAsciiText);
    }
}

/**
 * @brief     hex显示按钮状态改变
 * @note      将当前接收框的内容转换为hex格式重新显示
 * @param[in]
 * @return
 */
void MainWindow::on_hexDisplay_clicked(bool checked)
{
    checked = !checked;
    printToTextBrowser();
}

/**
 * @brief     激活使用win风格回车（\r\n）
 * @note      QT会把\r也进行换行，所以CRLF模式会换2次行，所以要进行识别和处理
 * @param[in] 是否激活
 */
void MainWindow::on_action_winLikeEnter_triggered(bool checked)
{
    if(checked){
        ui->action_unixLikeEnter->setChecked(false);
    }else {
        ui->action_winLikeEnter->setChecked(false);
    }
}

/*
 * Action:激活使用unix风格回车（\n）
 * Function:
*/
/**
 * @brief     激活使用unix风格回车（\n）
 * @note      参考 @ref on_action_winLikeEnter_triggered()
 * @param[in] 是否激活
 */
void MainWindow::on_action_unixLikeEnter_triggered(bool checked)
{
    if(checked){
        ui->action_winLikeEnter->setChecked(false);
    }else {
        ui->action_unixLikeEnter->setChecked(false);
    }
}

/**
 * @brief     激活使用UTF8编码
 * @note      不同编码中文的长度不一样，要识别和处理
 * @param[in] 是否激活
 */
void MainWindow::on_actionUTF8_triggered(bool checked)
{
    ui->actionUTF8->setChecked(true);
    if(checked){
        //设置中文编码
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        QTextCodec::setCodecForLocale(codec);
        ui->actionGBK->setChecked(false);

        p_regMatch->updateCodec("UTF-8");
        on_regMatchEdit_textChanged(ui->regMatchEdit->text());
    }
}

/**
 * @brief     激活使用GBK编码
 * @note      不同编码中文的长度不一样，要识别和处理
 * @param[in] 是否激活
 */
void MainWindow::on_actionGBK_triggered(bool checked)
{
    ui->actionGBK->setChecked(true);
    if(checked){
        //设置中文编码
        QTextCodec *codec = QTextCodec::codecForName("GBK");
        QTextCodec::setCodecForLocale(codec);
        ui->actionUTF8->setChecked(false);

        p_regMatch->updateCodec("GBK");
        on_regMatchEdit_textChanged(ui->regMatchEdit->text());
    }
}

/**
 * @brief     保存数据动作触发
 */
void MainWindow::on_actionSaveOriginData_triggered()
{
    QString tabName = MAIN_TAB_NAME;
    bool ok = false;
    if(ui->tabWidget->count() > 1){
        QStringList list;
        for(qint32 i = 0; i < ui->tabWidget->count(); i++){
            list << ui->tabWidget->tabText(i);
        }
        tabName = QInputDialog::getItem(this, tr("选择保存窗口"), tr("名称标签"),
                                        list, 0, false, &ok, Qt::WindowCloseButtonHint);
        if(ok != true){
            return;
        }
//        qDebug()<<"tabName"<<tabName;
    }

    //如果追加时间戳则提示时间戳不会被保存
    if(ui->timeStampCheckBox->isChecked())
        QMessageBox::information(this,tr("提示"),tr("时间戳数据不会被保存！只保存接收到的原始数据。"));

    //打开保存文件对话框
    QString savePath = QFileDialog::getSaveFileName(this,
                                                    tr("保存原始数据-选择文件路径"),
                                                    lastFileDialogPath +
                                                    "[" + tabName + "]-" +
                                                    QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss") + ".dat",
                                                    "Dat File(*.dat);;All File(*.*)");
    //检查路径格式
    if(!savePath.endsWith(".dat")){
        if(!savePath.isEmpty())
            QMessageBox::information(this,tr("提示"),"尚未支持的文件格式，请选择dat文件。");
        return;
    }

    //子窗口数据由其线程负责存储
    if(tabName != MAIN_TAB_NAME && tabName != REGMATCH_TAB_NAME){
        emit tee_saveData(savePath, tabName, true);
        return;
    }
    if(tabName == REGMATCH_TAB_NAME)
    {
        emit regM_saveData(savePath);
        return;
    }

    //保存数据
    QFile file(savePath);
    //删除旧数据形式写文件
    if(file.open(QFile::WriteOnly|QFile::Truncate)){
        file.write(RxBuff);//不用DataStream写入非文本文件，它会额外添加4个字节作为文件头
        file.flush();
        file.close();

        QString str = "Total saved " + QString::number(file.size()) + " Bytes in " + savePath;
        appendMsgLogToBrowser(str);
    }else{
        QMessageBox::information(this,tr("提示"),tr("文件打开失败。"));
    }

    //记忆路径
    lastFileDialogPath = savePath;
    lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/')+1);
}

/**
 * @brief     解包文件
 * @note      测试文件可访问性并使能解包
 * @param[in] 应该为readFile或者sendFile
 * @param[in] 文件路径
 * @param[in] 解包成功后是否删除文件
 * @param[in] 解包大小
 */
int32_t MainWindow::unpack_file(bool &actionType, QString path, bool deleteIfSuccess, int32_t pack_size)
{
    closeInteractiveUI();
    if(readFile || sendFile)
    {
        openInteractiveUI();
        QMessageBox::information(this, tr("提示"), tr("请等待上一个文件解析完毕。"));
        return -1;
    }

    //检测是否可访问并开始解包
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))
    {
        openInteractiveUI();
        QMessageBox::information(this, tr("提示"), tr("解包文件打开失败。"));
        return -1;
    }
    file.close();
    actionType = true;
    fileUnpacker->unpack_file(path, deleteIfSuccess, pack_size);
    return 0;
}

/**
 * @brief     读取数据动作触发
 */
void MainWindow::on_actionOpenOriginData_triggered()
{
    static QString lastFileName;
    //打开文件对话框
    QString readPath = QFileDialog::getOpenFileName(this,
                                                    tr("读取原始数据-选择文件路径"),
                                                    lastFileDialogPath + lastFileName,
                                                    "Dat File(*.dat);;All File(*.*)");
    //检查文件路径结尾
    if(!readPath.endsWith(".dat")){
        if(!readPath.isEmpty())
            QMessageBox::information(this,tr("尚未支持的文件格式"),tr("请选择dat文件。"));
        return;
    }
    //记录上次路径
    lastFileDialogPath = readPath;
    lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/')+1);

    //开始解包
    if(unpack_file(readFile, readPath, false, UNPACK_SIZE_OF_RX))
    {
        lastFileName.clear();
        return;
    }

    //记录上一次文件名
    lastFileName = readPath;
    while(lastFileName.indexOf('/')!=-1){
        lastFileName = lastFileName.mid(lastFileName.indexOf('/')+1);
    }
}

/**
 * @brief     关于按钮触发
 */
void MainWindow::on_actionAbout_triggered()
{
    //生日检查啦
    QDateTime dateTime(QDateTime::currentDateTime());
    if(dateTime.toString("MM-dd") == BIRTHDAY_DATE)
    {
        int32_t year = 0;
        year = dateTime.toString("yyyy").toInt();
        year = year - QString(BIRTHDAY_YEAR).toInt();
        QMessageBox::information(this, "Happy~",
                                 "Today is my birthday!\n I'm " +
                                 QString::number(year) + " years old! :)");
    }

    //创建关于我对话框资源
    About_Me_Dialog* p = new About_Me_Dialog(this);
    p->getVersionString(Config::getVersion());
    //设置close后自动销毁
    p->setAttribute(Qt::WA_DeleteOnClose);
    //非阻塞式显示
    p->show();
}

/**
 * @brief     串口配置按钮被触发
 */
void MainWindow::on_actionCOM_Config_triggered()
{
    if(serial.isOpen())
    {
        QMessageBox::information(this, tr("提示"), tr("请先关闭串口。"));
        return;
    }
    //创建串口设置对话框
    settings_dialog* p = new settings_dialog(this);
    //对话框读取原配置
    p->setStopBits(serial.stopBits());
    p->setDataBits(serial.dataBits());
    p->setParity(serial.parity());
    p->setFlowControl(serial.flowControl());
    p->exec();
    //对话框返回新配置并设置
    if(p->clickedOK())
    {
        if(!serial.moreSetting(p->getStopBits(),
                               p->getParity(),
                               p->getFlowControl(),
                               p->getDataBits()))
        {
            QMessageBox::information(this, tr("提示"), tr("串口设置失败，请关闭串口重试"));
        }
    }

    delete p;
}

/**
 * @brief     波特率框文本变化
 * @note      检查输入合法性并重置波特率
 * @param[in]
 * @return
 */
void MainWindow::on_baudrateList_currentTextChanged(const QString &arg1)
{
    bool ok;
    qint32 baud = arg1.toInt(&ok);
    if(ok){
        serial.setBaudRate(baud);
    }
    else {
        QMessageBox::information(this,tr("提示"),tr("请输入合法波特率"));
    }
}

/**
 * @brief     comList控件被按下
 * @note      选择了新的端口号，重新打开串口
 * @param[in] 选择的端口号
 */
void MainWindow::on_comList_textActivated(const QString &arg1)
{
    //关闭自动发送功能
    if(ui->cycleSendCheck->isChecked()){
        on_cycleSendCheck_clicked(false);
    }
    if(ui->actiondebug->isChecked()){
        debugTimer.stop();
        ui->actiondebug->setChecked(false);
    }

    QString unused = arg1;//屏蔽警告
    //重新打开串口
    if(serial.isOpen()){
        on_comSwitch_clicked(false);
        on_comSwitch_clicked(true);
        if(serial.isOpen())
            ui->statusBar->showMessage(tr("已重新启动串口"), 2000);
        else
            ui->statusBar->showMessage(tr("串口重启失败"), 2000);
    }
}

/**
 * @brief     保存显示数据被按下
 */
void MainWindow::on_actionSaveShowedData_triggered()
{
    QString tabName = MAIN_TAB_NAME;
    bool ok = false;
    if(ui->tabWidget->count() > 1){
        QStringList list;
        for(qint32 i = 0; i < ui->tabWidget->count(); i++){
            list << ui->tabWidget->tabText(i);
        }
        tabName = QInputDialog::getItem(this, tr("选择保存窗口"), tr("名称标签"),
                                        list, 0, false, &ok, Qt::WindowCloseButtonHint);
        if(ok != true){
            return;
        }
//        qDebug()<<"tabName"<<tabName;
    }

    //打开保存文件对话框
    QString savePath = QFileDialog::getSaveFileName(
                this,
                tr("保存显示数据-选择文件路径"),
                lastFileDialogPath +
                "[" + tabName + "]-" +
                QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss") + ".txt",
                "Text File(*.txt);;All File(*.*)");

    //检查路径
    if(!savePath.endsWith("txt")){
        if(!savePath.isEmpty())
            QMessageBox::information(this,tr("尚未支持的文件格式"),tr("请选择txt文本文件。"));
        return;
    }

    //标签页的数据保存由其线程自己负责
    if(tabName != MAIN_TAB_NAME && tabName != REGMATCH_TAB_NAME){
        emit tee_saveData(savePath, tabName, false);
        return;
    }
    if(tabName == REGMATCH_TAB_NAME)
    {
        emit regM_saveData(savePath);
        return;
    }

    //记忆路径
    lastFileDialogPath = savePath;
    lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/')+1);

    //保存数据
    QFile file(savePath);
    QTextStream stream(&file);
    //删除旧数据形式写文件
    if(file.open(QFile::WriteOnly|QFile::Text|QFile::Truncate)){
        if(ui->hexDisplay->isChecked()){
            stream << hexBrowserBuff;
        }else{
            stream << BrowserBuff;
        }
        file.close();

        QString str = "Total saved " + QString::number(file.size()) + " Bytes in " + savePath;
        appendMsgLogToBrowser(str);
    }else{
        QMessageBox::information(this,tr("提示"),tr("文件打开失败。"));
    }
}

/**
 * @brief     更新按钮触发
 */
void MainWindow::on_actionUpdate_triggered()
{
    if(http)
    {
        ui->statusBar->showMessage(tr("正在检查更新……"), 2000);
        http->addTask(HTTP::GetVersion);
        http->addTask(HTTP::DownloadMSGs);
    }
}

/**
 * @brief     周期发送时长文本改变
 * @note      改变时重置周期发送定时器
 * @param[in] 新的文本
 */
void MainWindow::on_sendInterval_textChanged(const QString &arg1)
{
    if(arg1.toInt() == 0)
    {
        cycleSendTimer.stop();
        ui->cycleSendCheck->setChecked(false);
        ui->statusBar->showMessage(tr("发送间隔不允许设为0"), 2000);
        return;
    }

    if(cycleSendTimer.isActive())
        cycleSendTimer.setInterval(arg1.toInt());
}

/**
 * @brief     STM32_ISP按钮按下
 */
void MainWindow::on_actionSTM32_ISP_triggered()
{
    if(ui->comSwitch->isChecked())
    {
        QMessageBox::information(this, tr("提示"),
                                 tr("请先关闭串口。"));
        return;
    }

    QFile file;
    file.copy(":/stm32isp.exe","stm32isp.exe");
    file.setFileName("stm32isp.exe");
    file.setPermissions(QFile::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner);

    STM32ISP_Dialog *p = new STM32ISP_Dialog(this);
    p->exec();
    delete p;

    file.remove();

    statisticStm32IspUseCnt++;
}

/**
 * @brief     多字符串条目双击
 * @note      双击后发送数据
 * @param[in] 点击的条目
 */
void MainWindow::on_multiString_itemDoubleClicked(QListWidgetItem *item)
{
    is_multi_str_double_click = true;
    //剔除注释
    QString tmp = item->text();
    if(tmp.indexOf('|') != -1)
    {
        tmp = tmp.mid(tmp.indexOf('|') + 1);
    }
    else
    {
        qint32 i = 0;
        for(i = 0; i < ui->multiString->count(); i++)
        {
            if(ui->multiString->item(i) == item)
            {
                break;
            }
        }
        item->setText("CMD_" +
                      QString::number(i) + " |" +
                      tmp);
    }

    g_multiStr_cur_index = ui->multiString->currentIndex().row();
    ui->textEdit->setText(tmp);
    on_sendButton_clicked();
}

/**
 * @brief     多字符串开关触发
 */
void MainWindow::on_actionMultiString_triggered(bool checked)
{
    if(checked){
        ui->multiString->show();
        //设置颜色交错
        ui->multiString->setAlternatingRowColors(true);
        statisticMultiStrUseCnt++;
    }else {
        ui->multiString->hide();
    }
    adjustLayout();
}

/**
 * @brief     多字符串组件的右键菜单
 * @param[in] 右键的位置
 */
void MainWindow::on_multiString_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem* curItem = ui->multiString->itemAt( pos );
    QAction *editSeed = nullptr;
    QAction *editCommentSeed = nullptr;
    QAction *moveUpSeed = nullptr;
    QAction *moveDownSeed = nullptr;
    QAction *deleteSeed = nullptr;
    QAction *addSeed = nullptr;
    QAction *clearSeeds = nullptr;
    QMenu *popMenu = new QMenu( this );
    //添加右键菜单
    if( curItem != nullptr ){
        editSeed = new QAction(tr("编辑当前条目"), this);
        popMenu->addAction( editSeed );
        connect( editSeed, SIGNAL(triggered() ), this, SLOT( editSeedSlot()) );
        editCommentSeed = new QAction(tr("编辑当前条目注释"), this);
        popMenu->addAction( editCommentSeed );
        connect( editCommentSeed, SIGNAL(triggered() ), this, SLOT( editCommentSeedSlot()) );

        popMenu->addSeparator();
        moveUpSeed = new QAction(tr("上移当前条目"), this);
        popMenu->addAction( moveUpSeed );
        connect( moveUpSeed, SIGNAL(triggered() ), this, SLOT( moveUpSeedSlot()) );
        moveDownSeed = new QAction(tr("下移当前条目"), this);
        popMenu->addAction( moveDownSeed );
        connect( moveDownSeed, SIGNAL(triggered() ), this, SLOT( moveDownSeedSlot()) );

        popMenu->addSeparator();
        deleteSeed = new QAction(tr("删除当前条目"), this);
        popMenu->addAction( deleteSeed );
        connect( deleteSeed, SIGNAL(triggered() ), this, SLOT( deleteSeedSlot()) );

        popMenu->addSeparator();
    }
    addSeed = new QAction(tr("新增一个条目"), this);
    popMenu->addAction( addSeed );
    connect( addSeed, SIGNAL(triggered() ), this, SLOT( addSeedSlot()) );
    clearSeeds = new QAction(tr("清空所有条目"), this);
    popMenu->addAction( clearSeeds );
    connect( clearSeeds, SIGNAL(triggered() ), this, SLOT( clearSeedsSlot()) );
    popMenu->exec( QCursor::pos() );
    delete popMenu;
    delete clearSeeds;
    delete deleteSeed;
    delete editSeed;
    delete editCommentSeed;
    delete moveUpSeed;
    delete moveDownSeed;
    delete addSeed;
}

/**
 * @brief     编辑多字符串组件的条目
 */
void MainWindow::editSeedSlot()
{
    QListWidgetItem * item = ui->multiString->currentItem();
    if( item == nullptr )
        return;

    qint32 curIndex = ui->multiString->row(item);
    bool ok = false;
    QString comment, containt;
    containt = ui->multiString->item(curIndex)->text();
    if(containt.indexOf('|') != -1)
    {
        comment = containt.mid(0, containt.indexOf('|'));
        containt = containt.mid(containt.indexOf('|') + 1);
    }

    QString newStr = QInputDialog::getMultiLineText(this, tr("编辑条目"), tr("新的文本："),
                                                    containt,
                                                    &ok, Qt::WindowCloseButtonHint);
    newStr = comment + '|' + newStr;
    if(ok == true)
        ui->multiString->item(curIndex)->setText(newStr);
}

/**
 * @brief     编辑多字符串组件的条目的注释
 */
void MainWindow::editCommentSeedSlot()
{
    QListWidgetItem * item = ui->multiString->currentItem();
    if( item == nullptr )
        return;

    qint32 curIndex = ui->multiString->row(item);
    bool ok = false;
    QString comment, containt;
    containt = ui->multiString->item(curIndex)->text();
    if(containt.indexOf('|') != -1)
    {
        comment = containt.mid(0, containt.indexOf('|'));
        containt = containt.mid(containt.indexOf('|') + 1);
    }
    QString newStr = QInputDialog::getMultiLineText(this, tr("编辑条目注释"), tr("新的文本："),
                                                    comment,
                                                    &ok, Qt::WindowCloseButtonHint);
    if(newStr.indexOf('|') != -1)
    {
        QMessageBox::information(this, tr("提示"), tr("注释不允许使用 | 符号，本次修改被放弃"));
        return;
    }
    newStr = newStr + '|' + containt;
    if(ok == true)
        ui->multiString->item(curIndex)->setText(newStr);
}

/**
 * @brief     多字符串组件条目上移一个
 */
void MainWindow::moveUpSeedSlot()
{
    QListWidgetItem * item = ui->multiString->currentItem();
    if( item == nullptr )
        return;

    qint32 curIndex = ui->multiString->row(item);
    if(curIndex - 1 >= 0)
    {
        ui->multiString->takeItem(curIndex);
        ui->multiString->insertItem(curIndex - 1, item);
    }
}

/**
 * @brief     多字符串组件条目下移一个
 */
void MainWindow::moveDownSeedSlot()
{
    QListWidgetItem * item = ui->multiString->currentItem();
    if( item == nullptr )
        return;

    qint32 curIndex = ui->multiString->row(item);
    ui->multiString->takeItem(curIndex);
    ui->multiString->insertItem(curIndex + 1, item);
}

/**
 * @brief     多字符串组件条目删除一个
 */
void MainWindow::deleteSeedSlot()
{
    QListWidgetItem * item = ui->multiString->currentItem();
    if( item == nullptr )
        return;

    qint32 curIndex = ui->multiString->row(item);
    ui->multiString->takeItem(curIndex);
    delete item;
}

/**
 * @brief     清除多字符串组件条目
 */
void MainWindow::clearSeedsSlot()
{
    QMessageBox::StandardButton button;
    button = QMessageBox::information(this,
                                      tr("提示"),
                                      tr("确认清除所有条目？"),
                                      QMessageBox::Ok, QMessageBox::Cancel);
    if(button != QMessageBox::Ok)
        return;

    ui->multiString->clear();
}

/**
 * @brief     多字符串组件新增一个条目
 */
void MainWindow::addSeedSlot()
{
    bool ok;
    QString newStr = QInputDialog::getMultiLineText(this, tr("新增条目内容"), tr("新的文本："),
                                                    "",
                                                    &ok, Qt::WindowCloseButtonHint);
    if(!ok)
    {
        return;
    }
    addTextToMultiString(newStr);
}

/**
 * @brief     更新数据可视化按钮的标题
 */
void MainWindow::setVisualizerTitle(void)
{
    if(!ui->actionPlotterSwitch->isChecked() &&
       !ui->actionValueDisplay->isChecked() &&
       !ui->actionFFTShow->isChecked())
    {
        return;
    }

    if(ui->actionAscii->isChecked()){
        if(ui->actionSumCheck->isChecked())
            ui->visualizer->setTitle(tr("数据可视化：ASCII协议(和校验)"));
        else
            ui->visualizer->setTitle(tr("数据可视化：ASCII协议"));
    }
    else if(ui->actionFloat->isChecked()){
        if(ui->actionSumCheck->isChecked())
            ui->visualizer->setTitle(tr("数据可视化：FLOAT协议(和校验)"));
        else
            ui->visualizer->setTitle(tr("数据可视化：FLOAT协议"));
    }
    else if(ui->actionCSV->isChecked())
    {
        if(ui->actionSumCheck->isChecked())
            ui->visualizer->setTitle(tr("数据可视化：CSV协议(和校验)"));
        else
            ui->visualizer->setTitle(tr("数据可视化：CSV协议"));
    }
    else if(ui->actionMAD->isChecked())
    {
        if(ui->actionSumCheck->isChecked())
            ui->visualizer->setTitle(tr("数据可视化：MAD协议(和校验)"));
        else
            ui->visualizer->setTitle(tr("数据可视化：MAD协议"));
    }
}

/**
 * @brief     重置数据可视化按钮的标题
 */
void MainWindow::resetVisualizerTitle(void)
{
    if(ui->tabWidget_plotter->isVisible() ||
       ui->valueDisplay->isVisible() ||
       (fft_window && fft_window->isVisible()))
    {
        return;
    }

    ui->visualizer->setTitle(tr("数据可视化"));
}

/**
 * @brief     绘图器开关
 * @param[in] 是否打开
 */
void MainWindow::on_actionPlotterSwitch_triggered(bool checked)
{
    if(checked){
        ui->tabWidget_plotter->show();
        setVisualizerTitle();
        statisticPlotterUseCnt++;
    }else{
        ui->tabWidget_plotter->hide();
        resetVisualizerTitle();
    }

    adjustLayout();
}

/**
 * @brief     创建一个新的绘图器对象
 * @note      新对象的配置来源于默认绘图器
 * @param[in] 新绘图器对象的名称（标题）
 * @return    新的绘图器对象
 */
MyQCustomPlot* MainWindow::createNewPlotter(QString plotterTitle)
{
    MyQCustomPlot* plotter = nullptr;
    MyQCustomPlot* defaultPlotter = nullptr;
    defaultPlotter = plotterManager.getDefaultPlotter();
    if(!defaultPlotter)
    {
        qDebug() << "defaultPlotter is null at" << __FUNCTION__;
        return nullptr;
    }

    plotter = new MyQCustomPlot();
    plotter->init(ui->actionSavePlotData,
                    ui->actionSavePlotAsPicture,
                    defaultPlotter->getxAxisSource(),
                    defaultPlotter->getAutoRescaleYAxis(),
                    fft_window,
                    plotProtocol,
                    plotterTitle);
    //线型设置
    plotter->plotControl->setupLineType(defaultPlotter->plotControl->getLineType());
    //GPU设置
    plotter->useOpenGL(defaultPlotter->getUseOpenGLState());
    plotter->plotControl->setupFont(g_font);
    //背景色设置
    plotter->setBackground(g_background_color);
    plotter->legend->setBrush(g_background_color);

    return plotter;
}

/**
 * @brief     记录图像数据到文件
 * @note      已经是解析完成后的数据了
 * @param[in] 记录路径
 * @param[in] 绘图器标题
 * @param[in] 绘图器对应的数据
 * @return    错误码
 */
int32_t MainWindow::recordGraphDataToFile(const QString& recordPlotTitle, const QString& plotterTitle, const QVector<double>& oneRowData)
{
    MyQCustomPlot * plotter = nullptr;
    if(!graphDataRecordPath.isEmpty())
    {
        if(recordPlotTitle != plotterTitle)
        {
            return -1;
        }

        plotter = plotterManager.selectPlotter(recordPlotTitle);
        if(!plotter)
        {
            return -1;
        }
        //如果没找到指定的绘图器
        if(plotter->getPlotterTitle() != recordPlotTitle)
        {
            ui->statusBar->showMessage(tr("错误：未找到要记录的绘图名称！"), 2000);
            return -1;
        }
        //添加表头
        if(!QFile(graphDataRecordPath).exists())
        {
            QString head;
            QVector<QString> nameSet;
            plotter = nullptr;
            plotter = plotterManager.selectPlotter(recordPlotTitle);
            if(plotter)
            {
                nameSet = plotter->plotControl->getNameSets();
                for(int32_t i = 0; i < oneRowData.size(); i++)
                {
                    head.append(nameSet.at(i));
                    head.append(",");
                }
                head.replace(head.lastIndexOf(','), 1, '\n');//把最后一个逗号换成换行符
                //文件头需要立刻写
                p_logger->append_data_logger_buff(GRAPH_DATA_LOG, head.toLocal8Bit());
                p_logger->logger_buff_flush(GRAPH_DATA_LOG);
            }
        }
        //添加数据
        QString dataLine;
        foreach(double num, oneRowData)
        {
            dataLine.append(QString::number(num, 'f'));
            dataLine.append(",");
        }
        dataLine.replace(dataLine.lastIndexOf(','), 1, '\n');//把最后一个逗号换成换行符
        emit logger_append(GRAPH_DATA_LOG, dataLine.toLocal8Bit());
    }
    return 0;
}

/**
 * @brief     绘图器刷新定时器的槽
 * @note      绘图器、数值显示器、FFT都在这刷新
 */
void MainWindow::plotterShowTimerSlot()
{
    QVector<double> oneRowData;
    QVector<double> currentRowData; //当前激活窗口的最新数据
    QString plotterTitle;
    QString currentPlotterTitle;    //当前激活的绘图窗口名字
    MyQCustomPlot *plotter = nullptr;

    if(!ui->actionPlotterSwitch->isChecked() &&
       !ui->actionValueDisplay->isChecked() &&
        (fft_window && !fft_window->isVisible()))
    {
        return;
    }

    if(plotProtocol->hasParsedBuff() == 0 || disableRefreshWindow)
    {
        return;
    }

    //寻找当前激活的绘图窗口
    currentPlotterTitle = ui->tabWidget_plotter->tabText(ui->tabWidget_plotter->currentIndex());

    //数据填充与统计
    while(plotProtocol->hasParsedBuff() > 0)
    {
        plotProtocol->popOneRowData(plotterTitle, oneRowData);
        statisticPlotterNumCnt += oneRowData.size();
        if(currentPlotterTitle == plotterTitle)
        {
            currentRowData = oneRowData;
        }
        //实时数据记录仪
        recordGraphDataToFile(recordPlotTitle, plotterTitle, oneRowData);
        //FFT处理
        if(fft_window->isVisible() &&
           fft_window->getFFTPlotterTitle() == plotterTitle)
        {
            fft_window->appendData(oneRowData);
        }
        //绘图显示器
        if(ui->actionPlotterSwitch->isChecked())
        {
            //把数据添加进绘图对象中或者创建新的绘图对象
            plotter = nullptr;
            plotter = plotterManager.selectPlotter(plotterTitle);
            if(!plotter)
            {
                plotter = createNewPlotter(plotterTitle);
                plotterManager.addPlotter(plotterTitle, plotter);
                ui->tabWidget_plotter->addTab(plotter, plotterTitle);
            }
            plotter->plotControl->addDataToPlotter(oneRowData);
        }
    }
    //曲线刷新
	if(ui->actionPlotterSwitch->isChecked())
	{
        plotter = nullptr;
        plotter = plotterManager.selectPlotter(currentPlotterTitle);
        if(plotter)
        {
            if(plotter->getAutoRescaleYAxis())
            {
                plotter->yAxis->rescale(true);
            }
            plotter->replot();   //<20ms
        }
    }
    if(fft_window->isVisible())
    {
        fft_window->startFFTCal();
    }

    //数值显示器
    plotter = nullptr;
    plotter = plotterManager.selectPlotter(currentPlotterTitle);
    if(ui->actionValueDisplay->isChecked() && plotter)
    {
        fillDataToValueDisplay(plotter);
    }
}

/**
 * @brief     填充最新的数据到数值显示器中
 * @param[in] 当前的绘图器对象，里面会包含最新的一批数据
 */
void MainWindow::fillDataToValueDisplay(MyQCustomPlot *plotter)
{
    if(!plotter)
        return;

    QVector<double> recentRowData = plotter->plotControl->getRecentRowData();
    //判断是否添加行
    if(ui->valueDisplay->rowCount() < recentRowData.size())
    {
        //设置行
        ui->valueDisplay->setRowCount(recentRowData.size());
        //设置列，固定的
        ui->valueDisplay->setColumnCount(2);
        ui->valueDisplay->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("名称")));
        ui->valueDisplay->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("值")));
        ui->valueDisplay->horizontalHeader()->setStretchLastSection(true);
        ui->valueDisplay->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
        ui->valueDisplay->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    }
    //添加数据
    int32_t min = recentRowData.size() < plotter->plotControl->getNameSets().size()
                ? recentRowData.size() : plotter->plotControl->getNameSets().size();
    for(int32_t i = 0; i < min; i++)
    {
        //这里会重复new对象导致内存溢出吗
        ui->valueDisplay->setItem(i, 0, new QTableWidgetItem(plotter->plotControl->getNameSets().at(i)));
        ui->valueDisplay->setItem(i, 1, new QTableWidgetItem(QString::number(recentRowData.at(i),'g')));
        //不可编辑
        ui->valueDisplay->item(i,0)->setFlags(ui->valueDisplay->item(i,0)->flags() & (~Qt::ItemIsEditable));
        ui->valueDisplay->item(i,1)->setFlags(ui->valueDisplay->item(i,1)->flags() & (~Qt::ItemIsEditable));
    }
    while(ui->valueDisplay->rowCount() > min)
    {
        //TODO:注意下有没有内存泄漏或者非法访问
        delete ui->valueDisplay->takeItem(ui->valueDisplay->rowCount()-1 , 0);
        delete ui->valueDisplay->takeItem(ui->valueDisplay->rowCount()-1 , 1);
        ui->valueDisplay->removeRow(ui->valueDisplay->rowCount()-1);
    }
}

/**
 * @brief     Ascii协议按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionAscii_triggered(bool checked)
{
    Q_UNUSED(checked)

    if(ui->actionAscii->isChecked())
        statisticASCIIUseCnt++;

    QString empty;
    emit protocol_clearBuff(empty);
    plotProtocol->setProtocolType(DataProtocol::Ascii);
    ui->actionAscii->setChecked(true);
    ui->actionFloat->setChecked(false);
    ui->actionCSV->setChecked(false);
    ui->actionMAD->setChecked(false);

    setVisualizerTitle();
}

/**
 * @brief     FLOAT协议按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionFloat_triggered(bool checked)
{
    Q_UNUSED(checked)

    if(ui->actionFloat->isChecked())
        statisticFLOATUseCnt++;

    QString empty;
    emit protocol_clearBuff(empty);
    plotProtocol->setProtocolType(DataProtocol::Float);
    ui->actionAscii->setChecked(false);
    ui->actionFloat->setChecked(true);
    ui->actionCSV->setChecked(false);
    ui->actionMAD->setChecked(false);

    setVisualizerTitle();
}

/**
 * @brief     debug按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actiondebug_triggered(bool checked)
{
    if(checked)
    {
        debugTimerSlotCnt = 0;
        debugTimer.setTimerType(Qt::PreciseTimer);
        debugTimer.start(1000/DEBUG_TIMER_FRQ);
        connect(&debugTimer, SIGNAL(timeout()), this, SLOT(debugTimerSlot()));
    }
    else
    {
        debugTimer.stop();
        disconnect(&debugTimer, SIGNAL(timeout()), this, SLOT(debugTimerSlot()));
    }
}

/**
 * @brief     垂直滚动条动作触发
 * @note      处理上滑显示更多数据的逻辑
 * @param[in] 触发动作
 */
void MainWindow::verticalScrollBarActionTriggered(qint32 action)
{
    QScrollBar* bar = ui->textBrowser->verticalScrollBar();

    if(bar->maximum() == 0)
        return;

//    qDebug()<<"verticalScrollBarActionTriggered"<<action<<bar->value()<<bar->maximum()<<bar->sliderPosition()<<100*bar->sliderPosition()/bar->maximum();

    if(action == QAbstractSlider::SliderSingleStepAdd ||
       action == QAbstractSlider::SliderSingleStepSub||
       action == QAbstractSlider::SliderPageStepAdd||
       action == QAbstractSlider::SliderPageStepSub||
       action == QAbstractSlider::SliderMove){
        qint32 value = bar->value();
        qint32 oldMax = bar->maximum();
        qint32 newValue;
        bool res;

        if(disableRefreshWindow)
        {
            //是否显示完了
            if(ui->hexDisplay->isChecked()){
                res = hexBrowserBuffIndex != disableRefreshHexBrowserSize;
            }else{
                res = BrowserBuffIndex != disableRefreshBrowserSize;
            }
        }
        else
        {
            //是否显示完了
            if(ui->hexDisplay->isChecked()){
                res = hexBrowserBuffIndex != hexBrowserBuff.size();
            }else{
                res = BrowserBuffIndex != BrowserBuff.size();
            }
        }

        //翻到顶部了，加载更多内容
        if(value == 0 && res)
        {
            if(disableRefreshWindow)
            {
                //显示内容指数型增加
                if(BrowserBuffIndex * 2 < disableRefreshBrowserSize)
                {
                    BrowserBuffIndex = BrowserBuffIndex * 2;
                }
                else
                {
                    BrowserBuffIndex = disableRefreshBrowserSize;
                }
                if(hexBrowserBuffIndex * 2 < disableRefreshHexBrowserSize)
                {
                    hexBrowserBuffIndex = hexBrowserBuffIndex * 2;
                }
                else
                {
                    hexBrowserBuffIndex = disableRefreshHexBrowserSize;
                }
                if(ui->hexDisplay->isChecked()){
                    ui->textBrowser->setPlainText(
                                hexBrowserBuff.mid(
                                    disableRefreshHexBrowserSize - hexBrowserBuffIndex,
                                    hexBrowserBuffIndex));
                }else{
                    ui->textBrowser->setPlainText(
                                BrowserBuff.mid(
                                    disableRefreshBrowserSize - BrowserBuffIndex,
                                    BrowserBuffIndex));
                }
            }
            else
            {
                //显示内容指数型增加
                if(BrowserBuffIndex * 2 < BrowserBuff.size())
                {
                    BrowserBuffIndex = BrowserBuffIndex * 2;
                }
                else
                {
                    BrowserBuffIndex = BrowserBuff.size();
                }
                if(hexBrowserBuffIndex * 2 < hexBrowserBuff.size())
                {
                    hexBrowserBuffIndex = hexBrowserBuffIndex * 2;
                }
                else
                {
                    hexBrowserBuffIndex = hexBrowserBuff.size();
                }
                if(ui->hexDisplay->isChecked()){
                    ui->textBrowser->setPlainText(hexBrowserBuff.mid(hexBrowserBuff.size() - hexBrowserBuffIndex));
                }else{
                    ui->textBrowser->setPlainText(BrowserBuff.mid(BrowserBuff.size() - BrowserBuffIndex));
                }
            }

            //保持bar位置不动
            newValue = bar->maximum() - oldMax;
            bar->setValue(newValue);
        }
    }
}

#if SHOW_PLOTTER_SETTING
void MainWindow::on_actionLinePlot_triggered()
{
    QVector<MyQCustomPlot*> list = plotterManager.getAllPlotters();
    foreach(MyQCustomPlot* plotter, list)
    {
        if(!plotter)
            continue;
        plotter->plotControl->setupLineType(Line);
    }
    MyQCustomPlot* plotter = selectCurrentPlotter();
    if(!plotter)
    {
        qDebug() << "null ptr of plotter at " << __FUNCTION__ << "()";
        return;
    }
    if(plotter->selectedGraphs().size() == 0)
    {
        ui->actionLinePlot->setChecked(true);
        ui->actionScatterLinePlot->setChecked(false);
        ui->actionScatterPlot->setChecked(false);
    }
    else
    {
        ui->actionLinePlot->setChecked(false);
        ui->actionScatterLinePlot->setChecked(false);
        ui->actionScatterPlot->setChecked(false);
    }
}

void MainWindow::on_actionScatterLinePlot_triggered()
{
    QVector<MyQCustomPlot*> list = plotterManager.getAllPlotters();
    foreach(MyQCustomPlot* plotter, list)
    {
        if(!plotter)
            continue;
        plotter->plotControl->setupLineType(ScatterLine);
    }
    MyQCustomPlot* plotter = selectCurrentPlotter();
    if(!plotter)
    {
        qDebug() << "null ptr of plotter at " << __FUNCTION__ << "()";
        return;
    }
    if(plotter->selectedGraphs().size() == 0)
    {
        ui->actionLinePlot->setChecked(false);
        ui->actionScatterLinePlot->setChecked(true);
        ui->actionScatterPlot->setChecked(false);
    }
    else
    {
        ui->actionLinePlot->setChecked(false);
        ui->actionScatterLinePlot->setChecked(false);
        ui->actionScatterPlot->setChecked(false);
    }
}

void MainWindow::on_actionScatterPlot_triggered()
{
    QVector<MyQCustomPlot*> list = plotterManager.getAllPlotters();
    foreach(MyQCustomPlot* plotter, list)
    {
        if(!plotter)
            continue;
        plotter->plotControl->setupLineType(Scatter);
    }
    MyQCustomPlot* plotter = selectCurrentPlotter();
    if(!plotter)
    {
        qDebug() << "null ptr of plotter at " << __FUNCTION__ << "()";
        return;
    }
    if(plotter->selectedGraphs().size() == 0)
    {
        ui->actionLinePlot->setChecked(false);
        ui->actionScatterLinePlot->setChecked(false);
        ui->actionScatterPlot->setChecked(true);
    }
    else
    {
        ui->actionLinePlot->setChecked(false);
        ui->actionScatterLinePlot->setChecked(false);
        ui->actionScatterPlot->setChecked(false);
    }
}

void MainWindow::on_actionAutoRefreshYAxis_triggered(bool checked)
{
    ui->actionAutoRefreshYAxis->setChecked(checked);
}

void MainWindow::on_actionOpenGL_triggered(bool checked)
{
    //使能OpenGL
    QVector<MyQCustomPlot*> list = plotterManager.getAllPlotters();
    foreach(MyQCustomPlot* plotter, list)
    {
        if(!plotter)
            continue;
        if(checked)
        {
            plotter->setOpenGl(true);
            fft_window->setupPlotterOpenGL(true); //实际未使能
        }
        else
        {
            plotter->setOpenGl(false);
            fft_window->setupPlotterOpenGL(false);
        }
    }

    //重绘
    MyQCustomPlot* plotter = nullptr;
    plotter = selectCurrentPlotter();
    if(!plotter)
    {
        return;
    }
    plotter->replot();
}

void MainWindow::on_actionSelectXAxis_triggered(bool checked)
{
    Q_UNUSED(checked)

    //TODO:各自维护一个副本或者统一设置，这个不用保存可以考虑各自维护一个副本，但是维护副本的话清空后又没了，也许还是统一设置比较好
    MyQCustomPlot* plotter = nullptr;
    plotter = selectCurrentPlotter();
    if(!plotter)
    {
        qDebug() << "null plotter pointer at" << __FUNCTION__;
        return;
    }
    //TODO:这里有个static注意一下
    static QString defaultXAxisLabel = plotter->xAxis->label();
    bool ok;
    QString name;
    QVector<QString> nameSets = plotter->plotControl->getNameSets();
    QStringList list;
    list.append(tr("递增计数值"));
    for (int32_t i = 0; i < plotter->graphCount(); i++)
    {
        list.append(nameSets.at(i));
    }
    name = QInputDialog::getItem(this, tr("选择X轴"), tr("名称"),
                                    list, 0, false, &ok, Qt::WindowCloseButtonHint);
    if(!ok)
        return;

    if(name == tr("递增计数值") && ui->actionTimeStampMode->isChecked())
    {
        QMessageBox::information(this, tr("提示"), tr("递增计数值模式下将关闭时间戳模式"));
        ui->actionTimeStampMode->setChecked(false);
        plotter->plotControl->setEnableTimeStampMode(false);
    }

    //选择了新的X轴,更新plotter->getxAxisSource()
    for (int32_t i = 0; i < list.size(); i++)
    {
        if(list.at(i) == name)
        {
            plotter->setxAxisSource(i);
            //更改X轴标签和隐藏被选为X轴的曲线
            if(plotter->getxAxisSource() != XAxis_Cnt)
            {
                plotter->xAxis->setLabel(name);
                int32_t j = 0;
                for (j = 0; j < plotter->graphCount(); j++)
                {
                    if(plotter->graph(j)->name() == name)
                    {
                        plotter->graph(j)->setVisible(false);
                        plotter->legend->item(j)->setTextColor(Qt::gray);
                        continue;
                    }
                    plotter->graph(j)->setVisible(true);
                    plotter->legend->item(j)->setTextColor(Qt::black);
                }
            }
            else
            {
                //时域模式显示所有曲线
                int32_t j = 0;
                for (j = 0; j < plotter->graphCount(); j++)
                {
                    plotter->graph(j)->setVisible(true);
                    plotter->legend->item(j)->setTextColor(Qt::black);
                }
                plotter->xAxis->setLabel(defaultXAxisLabel);
            }
            //并清空图像(不删除句柄)
            QString empty;
            emit protocol_clearBuff(empty);
            plotter->plotControl->clearPlotter(-1);
//            while(plotter->graphCount()>1){
//                plotter->removeGraph(plotter->graphCount() - 1);
//            }
            plotter->yAxis->rescale(true);
            plotter->xAxis->rescale(true);
            //TODO:目前是会刷新非激活的窗口
            plotter->replot();
            break;
        }
    }
}

void MainWindow::on_actionTimeStampMode_triggered(bool checked)
{
    MyQCustomPlot* plotter = nullptr;
    plotter = selectCurrentPlotter();
    if(!plotter)
    {
        qDebug() << "null plotter pointer at" << __FUNCTION__;
        return;
    }

    if(checked && plotter->getxAxisSource() == XAxis_Cnt)
    {
        QMessageBox::StandardButton button;
        button = QMessageBox::information(this, tr("提示"), tr("需要选择一条曲线作为时间戳数据"), QMessageBox::Ok, QMessageBox::Cancel);
        if(button != QMessageBox::Ok)
        {
            ui->actionTimeStampMode->setChecked(!checked);
            plotter->plotControl->setEnableTimeStampMode(!checked);
            return;
        }
        on_actionSelectXAxis_triggered(checked);
        if(plotter->getxAxisSource() == XAxis_Cnt)//弹出窗口但是没有选择
        {
            ui->actionTimeStampMode->setChecked(!checked);
            plotter->plotControl->setEnableTimeStampMode(!checked);
            return;
        }
    }
    ui->actionTimeStampMode->setChecked(checked);
    plotter->plotControl->setEnableTimeStampMode(checked);

    return;
}

#endif

/**
 * @brief     重置默认配置按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionResetDefaultConfig_triggered(bool checked)
{
    if(checked)
    {
        QMessageBox::Button button;
        button = QMessageBox::warning(this,
                                      tr("警告：确认恢复默认设置吗？"),
                                      tr("该操作会重置软件初始状态！"),
                                      QMessageBox::Ok|QMessageBox::No);
        if(button == QMessageBox::No)
            return;
        needSaveConfig = false;
        QMessageBox::information(this, tr("提示"), tr("重置成功。请重启程序。"));
    }else{
        needSaveConfig = true;
    }
}

/**
 * @brief     帮助按钮触发
 */
void MainWindow::on_actionManual_triggered()
{
    //创建关于我对话框资源
    About_Me_Dialog* p = new About_Me_Dialog(this);
    p->getVersionString(Config::getVersion());
    p->showManualDoc();
    p->resize(1024,768);
    //设置close后自动销毁
    p->setAttribute(Qt::WA_DeleteOnClose);
    //非阻塞式显示
    p->show();
}

/**
 * @brief     保存曲线数据按钮触发
 */
void MainWindow::on_actionSavePlotData_triggered()
{
    //判断是通过右键绘图窗口触发的还是点击菜单栏触发的
    bool click_by_file_menu = ui->file->underMouse();

    QString selectName;
    MyQCustomPlot* plotter = nullptr;
    plotter = selectCurrentPlotter();
    //如果有多个绘图器且是通过菜单栏触发该功能则进行选择。
    if(ui->tabWidget_plotter->count() > 1 && click_by_file_menu)
    {
        //选择绘图名称
        bool ok;
        QString label;
        label = tr("请选择需要保存的绘图器。");
        QVector<MyQCustomPlot*> plotters = plotterManager.getAllPlotters();
        QStringList items;
        foreach(MyQCustomPlot* plotter, plotters)
        {
            if(plotter)
            {
                items << plotter->getPlotterTitle();
            }
        }
        items.sort();

        //增加一个选项用于自动保存所有绘图器
        QString save_all = "SAVE ALL!";
        while(items.indexOf(save_all) != -1)
        {
            save_all.insert(0, '0');
        }
        items.insert(0, save_all);

        selectName = QInputDialog::getItem(this, tr("提示"),
                                    label,
                                    items,
                                    0,
                                    false,
                                    &ok);
        if(!ok)
        {
            return;
        }

        //判断是否保存所有数据
        if(selectName != save_all)
        {
            plotter = nullptr;
            plotter = plotterManager.selectPlotter(selectName);
            if(!plotter)
            {
                return;
            }
        }
        else
        {
            //自动保存所有数据
            QString saveFolder = QFileDialog::getExistingDirectory(this,
                                                                   tr("保存全部绘图数据-选择文件夹"),
                                                                   lastFileDialogPath);
            QDir saveDir(saveFolder);
            QString saveFile;
            QString savePath;
            QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");
            foreach(MyQCustomPlot* plotter, plotters)
            {
                if(plotter)
                {
                    plotter->replot();
                    saveFile = "[" + plotter->getPlotterTitle() + "]-" +
                               timeStamp + ".xlsx";
                    savePath = saveDir.absoluteFilePath(saveFile);
                    if(MyXlsx::write(plotter, savePath))
                    {
                        //打印成功信息
                        QString str = "\nSave successful in " + savePath + "\n";
                        BrowserBuff.append(str);
                        hexBrowserBuff.append(toHexDisplay(str.toLocal8Bit()));
                        printToTextBrowser();
                    }
                }
            }
            //记录路径
            lastFileDialogPath = savePath;
            lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/') + 1);
            return;
        }
    }
    plotter->replot();

    if(plotter->graph(0)->data()->size() == 0)
    {
        QMessageBox::information(this, tr("提示"), tr("绘图器数据容器为空，无法保存。"));
        return;
    }
    //打开保存文件对话框
    QString savePath = QFileDialog::getSaveFileName(this,
                                                    tr("保存绘图数据-选择文件路径"),
                                                    lastFileDialogPath +
                                                    "[" + plotter->getPlotterTitle() + "]-" +
                                                    QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")+".xlsx",
                                                    "XLSX File(*.xlsx);;CSV File(*.csv);;TXT File(*.txt);;All File(*.*)");
    //检查路径格式
    if(!savePath.endsWith(".xlsx") &&
       !savePath.endsWith(".csv") &&
       !savePath.endsWith(".txt")){
        if(!savePath.isEmpty())
            QMessageBox::information(this, tr("提示"),
                                     tr("尚未支持的文件格式。请选择xlsx或者csv或者txt格式文件。"));
        return;
    }

    bool ok = false;
    if(savePath.endsWith(".xlsx")){
        if(MyXlsx::write(plotter, savePath))
            ok = true;
    }else if(savePath.endsWith(".csv")){
        if(plotter->saveGraphAsTxt(savePath,','))
            ok = true;
    }else if(savePath.endsWith(".txt")){
        if(plotter->saveGraphAsTxt(savePath,' '))
            ok = true;
    }

    //记录路径
    lastFileDialogPath = savePath;
    lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/') + 1);

    if(ok){
        QString str = "\nSave successful in " + savePath + "\n";
        BrowserBuff.append(str);
        hexBrowserBuff.append(toHexDisplay(str.toLocal8Bit()));
        printToTextBrowser();
    }else{
        QMessageBox::warning(this, tr("警告"), tr("保存失败。文件是否被其他软件占用？"));
    }

}

/**
 * @brief     保存曲线图片按钮触发
 */
void MainWindow::on_actionSavePlotAsPicture_triggered()
{
    //判断是通过右键绘图窗口触发的还是点击菜单栏触发的
    bool click_by_file_menu = ui->file->underMouse();

    //如果有多个绘图器则进行选择
    QString selectName;
    MyQCustomPlot* plotter = nullptr;
    plotter = selectCurrentPlotter();
    //如果有多个绘图器且是通过菜单栏触发该功能则进行选择。
    if(ui->tabWidget_plotter->count() > 1 && click_by_file_menu)
    {
        //选择绘图名称
        bool ok;
        QString label;
        label = tr("请选择需要保存的绘图器。");
        QVector<MyQCustomPlot*> plotters = plotterManager.getAllPlotters();
        QStringList items;
        foreach(MyQCustomPlot* plotter, plotters)
        {
            if(plotter)
            {
                items << plotter->getPlotterTitle();
            }
        }
        items.sort();

        //增加一个选项用于自动保存所有绘图器
        QString save_all = "SAVE ALL!";
        while(items.indexOf(save_all) != -1)
        {
            save_all.insert(0, '0');
        }
        items.insert(0, save_all);

        selectName = QInputDialog::getItem(this, tr("提示"),
                                    label,
                                    items,
                                    0,
                                    false,
                                    &ok);
        if(!ok)
        {
            return;
        }
        //判断是否保存所有图片
        if(selectName != save_all)
        {
            plotter = nullptr;
            plotter = plotterManager.selectPlotter(selectName);
            if(!plotter)
            {
                return;
            }
        }
        else
        {
            //保存所有图片
            QString saveFolder = QFileDialog::getExistingDirectory(this,
                                                                   tr("保存全部绘图图像-选择文件夹"),
                                                                   lastFileDialogPath);
            QDir saveDir(saveFolder);
            QString saveFile;
            QString savePath;
            QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");
            foreach(MyQCustomPlot* plotter, plotters)
            {
                if(plotter)
                {
                    //这里要setCurrentIndex以“激活适配”窗口，否则图片是正方形
                    int32_t currentIndex = ui->tabWidget_plotter->currentIndex();
                    for(int32_t index = 0; index < ui->tabWidget_plotter->count(); index++)
                    {
                        if(ui->tabWidget_plotter->tabText(index) == plotter->getPlotterTitle())
                        {
                            ui->tabWidget_plotter->setCurrentIndex(index);
                            plotter->replot();
                            break;
                        }
                    }
                    ui->tabWidget_plotter->setCurrentIndex(currentIndex);

                    saveFile = "[" + plotter->getPlotterTitle() + "]-" +
                               timeStamp + ".bmp";
                    savePath = saveDir.absoluteFilePath(saveFile);
                    if(plotter->saveBmp(savePath))
                    {
                        //打印成功信息
                        QString str = "\nSave successful in " + savePath + "\n";
                        BrowserBuff.append(str);
                        hexBrowserBuff.append(toHexDisplay(str.toLocal8Bit()));
                        printToTextBrowser();
                    }
                }
            }
            //记录路径
            lastFileDialogPath = savePath;
            lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/') + 1);
            return;
        }
    }
    plotter->replot();

    if(!ui->tabWidget_plotter->isVisible())
    {
        QMessageBox::information(this, tr("提示"), tr("绘图器未开启，无法保存图片。"));
        return;
    }

    //打开保存文件对话框
    QString savePath = QFileDialog::getSaveFileName(this,
                                                    tr("曲线保存图片-选择文件路径"),
                                                    lastFileDialogPath +
                                                    "[" + plotter->getPlotterTitle() + "]-" +
                                                    QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"),
                                                    "Bmp File(*.bmp);;Pdf File(*.pdf);;Jpeg File(*.jpg);;Png File(*.png);;All File(*.*)");
    //检查路径格式
    if(!savePath.endsWith(".jpg") &&
       !savePath.endsWith(".bmp") &&
       !savePath.endsWith(".png") &&
       !savePath.endsWith(".pdf")){
        if(!savePath.isEmpty())
            QMessageBox::information(this,tr("提示"),tr("尚未支持的文件格式。请选择jpg/bmp/png/pdf文件。"));
        return;
    }

    //保存
    bool ok = false;
    if(savePath.endsWith(".jpg")){
        if(ui->customPlot->saveJpg(savePath,0,0,1,100))
            ok = true;
    }
    if(savePath.endsWith(".bmp")){
        if(ui->customPlot->saveBmp(savePath))
            ok = true;
    }
    if(savePath.endsWith(".png")){
        if(ui->customPlot->savePng(savePath,0,0,1,100))
            ok = true;
    }
    if(savePath.endsWith(".pdf")){
        if(ui->customPlot->savePdf(savePath))
            ok = true;
    }

    //记录上次路径
    lastFileDialogPath = savePath;
    lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/') + 1);

    if(ok){
        QString str = "\nSave successful in " + savePath + "\n";
        BrowserBuff.append(str);
        hexBrowserBuff.append(toHexDisplay(str.toLocal8Bit()));
        printToTextBrowser();
    }else{
        QMessageBox::warning(this, tr("警告"), tr("保存失败。文件是否被其他软件占用？"));
    }
}

/**
 * @brief     关键字高亮按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionKeyWordHighlight_triggered(bool checked)
{
    static int32_t first_run = 1;
    if(checked){
        if(highlighter == nullptr)
        {
            highlighter = new Highlighter(ui->textBrowser->document());
        }
        if(highlighter1 == nullptr)
        {
            highlighter1 = new Highlighter(ui->regMatchBrowser->document());
        }
    }else{
        delete highlighter;
        delete highlighter1;
        highlighter = nullptr;
        highlighter1 = nullptr;
    }

    if(first_run)
    {
        first_run = 0;
        return;
    }
    QMessageBox::information(this, tr("提示"), tr("高亮设置将应用于新的显示窗口。"));
}

/**
 * @brief     将字节数据转换成人类习惯的单位
 * @note      统计用的
 * @param[in] 字节数据
 * @return    转换结果
 */
QString MainWindow::sta_ConvertHuman_Byte(double num)
{
    double num_back = num;
    int64_t nest = 0;
    QString unit;
    QString ret;
    nest = 0;

    while(num > 1024){
        num = num / 1024.0;
        nest++;
    }

    switch(nest){
        case 0:
            unit = " B";
            ret  = QString::number(static_cast<int64_t>(num)) + unit;
            break;
        case 1:
            unit = " KB";
            ret  = QString::number(num, 'f', 3) + unit;
            break;
        case 2:
            unit = " MB";
            ret  = QString::number(num, 'f', 3) + unit;
            break;
        case 3:
            unit = " GB";
            ret  = QString::number(num, 'f', 3) + unit;
            break;
        case 4:
            unit = " TB";
            ret  = QString::number(num, 'f', 3) + unit;
            break;
        default:
            num = num_back;
            ret  = QString::number(static_cast<int64_t>(num)) + " B";
            break;
    }

    return ret;
}

/**
 * @brief     时间数据转换成人类习惯的单位
 * @note      统计用的
 * @param[in] 秒数据
 * @return    转换结果
 */
QString MainWindow::sta_ConvertHuman_Time(double sec)
{
    //时间常数
    const int64_t mi = 60;
    const int64_t hh = mi * 60;
    const int64_t dd = hh * 24;

    int64_t day = 0;
    int64_t hour = 0;
    int64_t minute = 0;
    int64_t second = 0;

    //时间换算
    day = static_cast<int64_t>(sec / dd);
    hour = static_cast<int64_t>((sec - day * dd) / hh);
    minute = static_cast<int64_t>((sec - day * dd - hour * hh) / mi);
    second = static_cast<int64_t>((sec - day * dd - hour * hh - minute * mi));

    return QString::number(day, 10) + tr(" 天 ") +
           QString::number(hour, 10) + tr(" 小时 ") +
           QString::number(minute, 10) + tr(" 分钟 ") +
           QString::number(second, 10) + tr(" 秒");
}

/**
 * @brief     收发结果转换成等级称号排名
 * @note      统计用的
 * @param[in] 总发送量
 * @param[in] 总接收量
 * @return    等级称号排名
 */
QString MainWindow::statisticConvertRank(double totalTx, double totalRx)
{
    QString rankStr;
    double totalTxRx_MB = totalTx/1024.0/1024.0 + totalRx/1024.0/1024.0;
    if(totalTxRx_MB<100){
        rankStr = tr("恭喜阁下，获得了【青铜码农】的称号！请再接再厉！");
    }else if(totalTxRx_MB<200){
        rankStr = tr("恭喜阁下，获得了【白银码农】的称号！请再接再厉！");
    }else if(totalTxRx_MB<400){
        rankStr = tr("恭喜阁下，获得了【黄金码农】的称号！请再接再厉！");
    }else if(totalTxRx_MB<800){
        rankStr = tr("恭喜阁下，获得了【铂金码农】的称号！请再接再厉！");
    }else if(totalTxRx_MB<1600){
        rankStr = tr("恭喜阁下，获得了【星钻码农】的称号！请再接再厉！");
    }else if(totalTxRx_MB<3200){
        rankStr = tr("恭喜阁下，获得了【皇冠码农】的称号！请再接再厉！");
    }else if(totalTxRx_MB<6400){
        rankStr = tr("恭喜阁下，获得了【王牌码农】的称号！请再接再厉！");
    }else{
        rankStr = tr("荣誉只是浮云~");
    }
    return rankStr;
}

/**
 * @brief     使用统计按钮触发
 * @note      显示使用统计
 */
void MainWindow::on_actionUsageStatistic_triggered()
{
    double currentTx = serial.getTotalTxCnt() + p_networkComm->getTotalTxCnt();
    double currentRx = serial.getTotalRxCnt() + p_networkComm->getTotalRxCnt();
    double totalTx = Config::getTotalTxCnt().toDouble() + currentTx;
    double totalRx = Config::getTotalRxCnt().toDouble() + currentRx;
    double totalRunTime = Config::getTotalRunTime().toDouble() + currentRunTime;

    QString totalTxStr;
    QString totalRxStr;
    QString currentTxStr;
    QString currentRxStr;
    QString currentRunTimeStr, totalRunTimeStr;
    QString currentRegParseStr;
    QString currentTeeParseStr;
    QString currentPlotterNumStr;
    QString totalPlotterNumStr;
    QString totalValueDisplayUseStr;
    QString totalPlotterUseStr;
    QString totalFFTUseStr;
    QString totalMultiStrUseStr;
    QString totalAsciiTableUseStr;
    QString totalPriorityTableUseStr;
    QString totalStm32IspUseStr;
    QString totalHexToolUseStr;
    QString totalASCIIUseStr;
    QString totalFLOATUseStr;
    QString totalCSVUseStr;
    QString totalMADUseStr;
    QString totalTeeUseStr;
    QString totalTeeParseStr;
    QString totalRegParseStr;
    QString totalRecordUseStr;
    QString rankStr;

    //单位换算
    totalTxStr   = sta_ConvertHuman_Byte(totalTx);
    totalRxStr   = sta_ConvertHuman_Byte(totalRx);
    currentTxStr = sta_ConvertHuman_Byte(currentTx);
    currentRxStr = sta_ConvertHuman_Byte(currentRx);
    currentRegParseStr    = sta_ConvertHuman_Byte(statisticRegParseCnt);
    currentTeeParseStr    = sta_ConvertHuman_Byte(statisticTeeParseCnt);
    currentPlotterNumStr  = sta_ConvertHuman_Byte(statisticPlotterNumCnt);
    currentPlotterNumStr  = currentPlotterNumStr.mid(0, currentPlotterNumStr.size() - 1); //移除最后的字符B
    totalRegParseStr    = sta_ConvertHuman_Byte(Config::getTotalStatistic(KEY_TOTALREGPARSE) + statisticRegParseCnt);
    totalTeeParseStr    = sta_ConvertHuman_Byte(Config::getTotalStatistic(KEY_TOTALTEEPARSE) + statisticTeeParseCnt);
    totalPlotterNumStr  = sta_ConvertHuman_Byte(Config::getTotalStatistic(KEY_TOTALPLOTTERNUM) + statisticPlotterNumCnt);
    totalPlotterNumStr  = totalPlotterNumStr.mid(0, totalPlotterNumStr.size() - 1); //移除最后的字符B

    //次数换算
    totalValueDisplayUseStr = QString::number(Config::getTotalStatistic(KEY_TOTALVALUEDISPLAYUSE) + statisticValueDisplayUseCnt);
    totalPlotterUseStr      = QString::number(Config::getTotalStatistic(KEY_TOTALPLOTTERUSE) + statisticPlotterUseCnt);
    totalFFTUseStr          = QString::number(Config::getTotalStatistic(KEY_TOTALFFTUSE) + statisticFFTUseCnt);
    totalMultiStrUseStr     = QString::number(Config::getTotalStatistic(KEY_TOTALMULTISTRUSE) + statisticMultiStrUseCnt);
    totalAsciiTableUseStr   = QString::number(Config::getTotalStatistic(KEY_TOTALASCIITABLEUSE) + statisticAsciiTableUseCnt);
    totalPriorityTableUseStr= QString::number(Config::getTotalStatistic(KEY_TOTALPRIORITYTABLEUSE) + statisticPriorityTableUseCnt);
    totalStm32IspUseStr     = QString::number(Config::getTotalStatistic(KEY_TOTALSTM32ISPUSE) + statisticStm32IspUseCnt);
    totalHexToolUseStr      = QString::number(Config::getTotalStatistic(KEY_TOTALHEXTOOLUSE) + statisticHexToolUseCnt);
    totalASCIIUseStr        = QString::number(Config::getTotalStatistic(KEY_TOTALASCIILUSE) + statisticASCIIUseCnt);
    totalFLOATUseStr        = QString::number(Config::getTotalStatistic(KEY_TOTALFLOATUSE) + statisticFLOATUseCnt);
    totalCSVUseStr          = QString::number(Config::getTotalStatistic(KEY_TOTALCSVUSE) + statisticCSVUseCnt);
    totalMADUseStr          = QString::number(Config::getTotalStatistic(KEY_TOTALMADUSE) + statisticMADUseCnt);
    totalTeeUseStr          = QString::number(Config::getTotalStatistic(KEY_TOTALTEEUSE) + statisticTeeUseCnt);
    totalRecordUseStr       = QString::number(Config::getTotalStatistic(KEY_TOTALRECORDUSE) + statisticRecordCnt);

    //时间换算
    currentRunTimeStr = sta_ConvertHuman_Time(currentRunTime);
    totalRunTimeStr   = sta_ConvertHuman_Time(totalRunTime);

    //排名换算
    rankStr = statisticConvertRank(totalTx, totalRx);

    //上屏显示
    QString str;
    str.append(tr("## 软件版本：")+Config::getVersion() + "\n");
    str.append("\n");
    str.append(tr("## 设备信息") + "\n");
    str.append(tr("   MAC地址：")+HTTP::getHostMacAddress() + "\n");
    str.append("\n");
    str.append(tr("## 软件使用统计") + "\n");
    str.append(tr("   ### 自本次启动软件以来，阁下：") + "\n");
    str.append(tr("   - 共发送数据：") + currentTxStr + "\n");
    str.append(tr("   - 共接收数据：") + currentRxStr + "\n");
    str.append(tr("   - 共运行本软件：") + currentRunTimeStr + "\n");
    str.append(tr("   - 绘制数据点数：") + currentPlotterNumStr + "\n");
    str.append(tr("   - 数据分窗解析数据：") + currentTeeParseStr + "\n");
    str.append(tr("   - filter解析数据：") + currentRegParseStr + "\n");
    str.append("\n");
    str.append(tr("   ### 自首次启动软件以来，阁下：") + "\n");
    str.append(tr("   - 首次启动日期：") + Config::getFirstStartTime() + "\n");
    str.append(tr("   - 共发送数据：") + totalTxStr + "\n");
    str.append(tr("   - 共接收数据：") + totalRxStr + "\n");
    str.append(tr("   - 共运行本软件：") + totalRunTimeStr + "\n");
    str.append(tr("   - 共启动本软件：") + QString::number(Config::getTotalRunCnt().toLongLong()+1)+tr(" 次") + "\n");
    str.append(tr("   - 使用绘图器次数：") + totalPlotterUseStr + "\n");
    str.append(tr("   - 绘制数据点数：") + totalPlotterNumStr + "\n");
    str.append(tr("   - 使用数值显示器次数：") + totalValueDisplayUseStr + "\n");
    str.append(tr("   - 使用频谱图次数：") + totalFFTUseStr + "\n");
    str.append(tr("   - 使用数据分窗次数：") + totalTeeUseStr + "\n");
    str.append(tr("   - 数据分窗解析数据：") + totalTeeParseStr + "\n");
    str.append(tr("   - filter解析数据：") + totalRegParseStr + "\n");
    str.append(tr("   - 使用多字符串次数：") + totalMultiStrUseStr + "\n");
    str.append(tr("   - 使用ASCII码表次数：") + totalAsciiTableUseStr + "\n");
    str.append(tr("   - 使用运算优先级表次数：") + totalPriorityTableUseStr + "\n");
    str.append(tr("   - 使用STM32ISP次数：") + totalStm32IspUseStr + "\n");
    str.append(tr("   - 使用HEX Tool次数：") + totalHexToolUseStr + "\n");
    str.append(tr("   - 使用ASCII协议次数：") + totalASCIIUseStr + "\n");
    str.append(tr("   - 使用FLOAT协议次数：") + totalFLOATUseStr + "\n");
    str.append(tr("   - 使用CSV协议次数：") + totalCSVUseStr + "\n");
    str.append(tr("   - 使用MAD协议次数：") + totalMADUseStr + "\n");
    str.append(tr("   - 使用实时数据记录仪次数：") + totalRecordUseStr + "\n");
    str.append("\n");
    str.append(tr("   ")+rankStr + "\n");
    str.append("\n");
    str.append(tr("## 隐私声明") + "\n");
    str.append(tr("  - 以上统计信息可能会被上传至服务器用于统计。") + "\n");
    str.append(tr("  - 其他任何信息均不会被上传。") + "\n");
    str.append(tr("  - 如阁下不同意本声明，可阻断本软件的网络请求或者阁下应该停止使用本软件。") + "\n");
    str.append("\n");
    str.append(tr("## 感谢阁下的使用") + "\n");
    str.append("\n");

    //创建关于我对话框资源
    About_Me_Dialog* p = new About_Me_Dialog(this);
    p->getVersionString(Config::getVersion());
    p->showMarkdown(str);
    p->resize(1024,768);
    //设置close后自动销毁
    p->setAttribute(Qt::WA_DeleteOnClose);
    //非阻塞式显示
    p->show();
}

/**
 * @brief     发送文件按钮触发
 */
void MainWindow::on_actionSendFile_triggered()
{
    if(!remindDeviceIsOpen())
    {
        return;
    }

    static QString lastFileName;
    //打开文件对话框
    QString readPath = QFileDialog::getOpenFileName(this,
                                                    tr("打开文件"),
                                                    lastFileDialogPath + lastFileName,
                                                    "All File(*.*)");
    //检查文件路径结尾
    if(readPath.isEmpty()){
        return;
    }

    //记录上次路径
    lastFileDialogPath = readPath;
    lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/') + 1);

    //解包文件
    if(unpack_file(sendFile, readPath, false, UNPACK_SIZE_OF_TX))
    {
        return;
    }

    //记录上一次文件名
    lastFileName = readPath;
    while(lastFileName.indexOf('/') != -1){
        lastFileName = lastFileName.mid(lastFileName.indexOf('/') + 1);
    }
}

/**
 * @brief     数值显示器按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionValueDisplay_triggered(bool checked)
{
    if(checked){
        ui->valueDisplay->show();
        setVisualizerTitle();
        statisticValueDisplayUseCnt++;
    }else{
        ui->valueDisplay->hide();
        resetVisualizerTitle();
    }

    adjustLayout();
}

/**
 * @brief     关闭刷新窗口按钮触发
 * @param[in] 新的状态
 */
void MainWindow::disableRefreshWindow_triggered(bool checked)
{
    #define PLOT_BUFF_WARNING (1*10000)
    static bool displayedAllData = false;
    disableRefreshWindow = checked;
    fft_window->disableRePlot(checked);
    if(disableRefreshWindow)
    {
        if(plotProtocol->hasParsedBuff() > PLOT_BUFF_WARNING)
        {
            ui->statusBar->showMessage(tr("警告：待绘图数据较多，请尽早使能刷新！"));
        }
        else
        {
            ui->statusBar->showMessage(tr("已暂停刷新数据。"));
        }
        //记录此刻缓冲区的大小，取size速度相对较快，高频接收时暂停点更加准确
        if(!displayedAllData)
        {
            displayedAllData = true;
            disableRefreshBrowserSize = BrowserBuff.size();
            disableRefreshHexBrowserSize = hexBrowserBuff.size();
        }
    }
    else
    {
        ui->statusBar->showMessage("");
        displayedAllData = false;
    }
}

/**
 * @brief     显示所有文本按钮触发
 */
void MainWindow::showAllTextBrowser_triggered()
{
    if(disableRefreshWindow)
    {
        if(ui->hexDisplay->isChecked()){
            ui->textBrowser->setPlainText(hexBrowserBuff.mid(0, disableRefreshHexBrowserSize));
            hexBrowserBuffIndex = disableRefreshHexBrowserSize;
        }else{
            ui->textBrowser->setPlainText(BrowserBuff.mid(0, disableRefreshBrowserSize));
            BrowserBuffIndex = disableRefreshBrowserSize;
        }
    }
    else
    {
        if(ui->hexDisplay->isChecked()){
            ui->textBrowser->setPlainText(hexBrowserBuff);
            hexBrowserBuffIndex = hexBrowserBuff.size();
        }else{
            ui->textBrowser->setPlainText(BrowserBuff);
            BrowserBuffIndex = BrowserBuff.size();
        }
    }

    ui->textBrowser->moveCursor(QTextCursor::End);
}

/**
 * @brief     开关textBrowser文本编辑模式
 */
void MainWindow::editMode_triggered(bool checked)
{
    editMode->setChecked(checked);
    if(checked)
    {
        QMessageBox::StandardButton button;
        button = QMessageBox::information(this, tr("提示"),
                                 tr("编辑模式仅修改显示在GUI上的数据，原始数据无法修改。") + "\n\n" +
                                 tr("注意：所有可能触发GUI更新的操作都将导致修改丢失，如数据接收更新、滚动条上滑触顶、HEX显示切换等。"),
                                 QMessageBox::Ok, QMessageBox::Cancel);
        if(button == QMessageBox::Cancel)
        {
            editMode->setChecked(false);
            return;
        }
    }
    ui->textBrowser->setReadOnly(!checked);
}

/**
 * @brief     复制所选文本到剪贴板
 */
void MainWindow::copySelectedTextBrowser_triggered(void)
{
    QClipboard *clipboard = QApplication::clipboard();
    if(ui->textBrowser->textCursor().selectedText().isEmpty())
        return;
    clipboard->setText(ui->textBrowser->textCursor().selectedText());
}

/**
 * @brief     复制全部显示文本到剪贴板
 */
void MainWindow::copyAllTextBrowser_triggered(void)
{
    QClipboard *clipboard = QApplication::clipboard();
    if(ui->hexDisplay->isChecked())
    {
        clipboard->setText(hexBrowserBuff);
        return;
    }
    clipboard->setText(BrowserBuff);
}

/**
 * @brief     复制全部原始数据到剪贴板
 */
void MainWindow::copyAllData_triggered(void)
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(RxBuff);
}

/**
 * @brief     textBrowser右键内容初始化
 */
void MainWindow::textBrowser_rightClickContext_init()
{
    popMenu = new QMenu( this );
    //添加右键菜单
    stopRefresh = new QAction(tr("暂停刷新数据"), this);
    stopRefresh->setCheckable(true);
    showAllText = new QAction(tr("显示所有文本"), this);
    editMode = new QAction(tr("编辑模式"), this);
    editMode->setCheckable(true);
    copyText = new QAction(tr("复制所选文本"), this);
    copyAllText = new QAction(tr("复制全部显示数据"), this);
    copyAllData = new QAction(tr("复制全部原始数据"), this);
    saveShowedData = new QAction(tr("保存显示数据"), this);
    saveOriginData = new QAction(tr("保存原始数据"), this);
    clearTextBrowser = new QAction(tr("清空数据显示区"), this);

    popMenu->addAction( stopRefresh );
    popMenu->addSeparator();
    popMenu->addAction( showAllText );
    popMenu->addAction( editMode );
    popMenu->addSeparator();
    popMenu->addAction( copyText );
    popMenu->addAction( copyAllText );
    popMenu->addAction( copyAllData );
    popMenu->addSeparator();
    popMenu->addAction( saveShowedData );
    popMenu->addAction( saveOriginData );
    popMenu->addSeparator();
    popMenu->addAction( clearTextBrowser );

    connect( stopRefresh, SIGNAL(triggered(bool) ),
             this, SLOT( disableRefreshWindow_triggered(bool)) );
    connect( showAllText, SIGNAL(triggered() ), this, SLOT( showAllTextBrowser_triggered()) );
    connect( editMode, SIGNAL(triggered(bool) ), this, SLOT( editMode_triggered(bool)) );
    connect( copyText, SIGNAL(triggered() ), this, SLOT( copySelectedTextBrowser_triggered()) );
    connect( copyAllText, SIGNAL(triggered() ), this, SLOT( copyAllTextBrowser_triggered()) );
    connect( copyAllData, SIGNAL(triggered() ), this, SLOT( copyAllData_triggered()) );
    connect( saveOriginData, SIGNAL(triggered() ), this, SLOT( on_actionSaveOriginData_triggered()) );
    connect( saveShowedData, SIGNAL(triggered() ), this, SLOT( on_actionSaveShowedData_triggered()) );
    connect( clearTextBrowser, SIGNAL(triggered() ), this, SLOT( clearTextBrowserSlot()) );
}

/**
 * @brief     main窗口右键菜单
 * @param[in] 右键位置
 */
void MainWindow::on_textBrowser_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos)

    //暂停刷新
    if(disableRefreshWindow)
        stopRefresh->setChecked(true);
    else
        stopRefresh->setChecked(false);

    //复制所选文本
    if(ui->textBrowser->textCursor().selectedText().isEmpty())
        copyText->setEnabled(false);
    else
        copyText->setEnabled(true);

    popMenu->exec( QCursor::pos() );
}

/**
 * @brief     清除main窗口数据
 */
void MainWindow::clearTextBrowserSlot()
{
    ui->textBrowser->clear();
    RxBuff.clear();
    hexBrowserBuff.clear();
    hexBrowserBuffIndex = 0;
    BrowserBuff.clear();
    BrowserBuffIndex = 0;
    unshowedRxBuff.clear();
}

/**
 * @brief     数值显示器右键菜单
 * @param[in] 右键位置
 */
void MainWindow::on_valueDisplay_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos)

    QList<QTableWidgetItem*> selectedItems = ui->valueDisplay->selectedItems();
    QAction *deleteValueDisplayRow = nullptr;
    QAction *deleteValueDisplay = nullptr;
    QMenu *popMenu = new QMenu( this );
    //添加右键菜单
    if( selectedItems.size() ){
        deleteValueDisplayRow = new QAction(tr("删除元素所在行"), this);
        popMenu->addAction( deleteValueDisplayRow );
        connect( deleteValueDisplayRow, SIGNAL(triggered() ), this, SLOT( deleteValueDisplayRowSlot()) );

        popMenu->addSeparator();
    }
    deleteValueDisplay = new QAction(tr("删除所有行"), this);
    popMenu->addAction( deleteValueDisplay );
    connect( deleteValueDisplay, SIGNAL(triggered() ), this, SLOT( deleteValueDisplaySlot()) );
    popMenu->exec( QCursor::pos() );
    delete popMenu;
    delete deleteValueDisplayRow;
    delete deleteValueDisplay;
}

/**
 * @brief     删除数值显示器一行
 * @return
 */
void MainWindow::deleteValueDisplayRowSlot()
{
    QList<QTableWidgetItem*> selectedItems = ui->valueDisplay->selectedItems();

    while(selectedItems.size()){
        ui->valueDisplay->removeRow(selectedItems.at(0)->row());
        selectedItems.pop_front();
    }
}

/**
 * @brief     删除数值显示器所有行
 * @return
 */
void MainWindow::deleteValueDisplaySlot()
{
    while(ui->valueDisplay->rowCount()>0){
        ui->valueDisplay->removeRow(0);
    }
}

/**
 * @brief     时间戳按钮状态改变
 * @param[in] 状态值
 */
void MainWindow::on_timeStampCheckBox_stateChanged(int arg1)
{
    if(arg1 != 0){
        ui->timeStampTimeOut->setEnabled(true);
        timeStampTimer.setTimerType(Qt::PreciseTimer);
        timeStampTimer.setSingleShot(true);
        timeStampTimer.start(ui->timeStampTimeOut->text().toInt());
    }else{
        ui->timeStampTimeOut->setEnabled(false);
        timeStampTimer.stop();
    }
}

/**
 * @brief     时间戳超时文本改变
 * @param[in] 新的超时值
 */
void MainWindow::on_timeStampTimeOut_textChanged(const QString &arg1)
{
    timeStampTimer.setSingleShot(true);
    timeStampTimer.start(arg1.toInt());
}

/**
 * @brief     选择当前显示的绘图器
 */
MyQCustomPlot* MainWindow::selectCurrentPlotter()
{
    MyQCustomPlot* plotter = nullptr;
    QString currentPlotterTitle;
    currentPlotterTitle = ui->tabWidget_plotter->tabText(
                            ui->tabWidget_plotter->currentIndex());
    plotter = plotterManager.selectPlotter(currentPlotterTitle);
    //如果未找到绘图器则使用默认绘图器
    if(!plotter)
    {
        plotter = plotterManager.getDefaultPlotter();
        if(!plotter)
        {
            qDebug() << "do not find current plotter and default plotter is null!!!";
            return nullptr;
        }
        qDebug() << "do not find current plotter and return default plotter("
                 << plotter->getPlotterTitle() << ")";
    }
    return plotter;
}

/**
 * @brief     字体设置按钮触发
 */
void MainWindow::on_actionFontSetting_triggered()
{
    bool ok;
    QFont font;
    font = QFontDialog::getFont(&ok, g_font, this, tr("选择字体"));
    if(ok)
    {
        g_font = font;
        updateUIPanelFont(g_font);

        teeManager.updateAllTeeBrowserFont(g_font);
    }
}

/**
 * @brief     更新UI面板的背景色
 * @param[in] 新的背景色
 */
void MainWindow::updateUIPanelBackground(QColor itsColor)
{
    int32_t r, g, b;
    if(itsColor.isValid())
    {
        g_background_color = itsColor;
        g_background_color.getRgb(&r,&g,&b);
    }
    else
    {
        r = g = b = 0xFF;
        g_background_color.setRed(r);
        g_background_color.setGreen(g);
        g_background_color.setBlue(b);
    }

    QString background = "{ background-color: rgb(RGBR,RGBG,RGBB);}";
    background.replace("RGBR", QString::number(r));
    background.replace("RGBG", QString::number(g));
    background.replace("RGBB", QString::number(b));

    ui->textBrowser->setStyleSheet("QPlainTextEdit" + background);
    ui->regMatchBrowser->setStyleSheet("QPlainTextEdit" + background);
    ui->textEdit->setStyleSheet("QTextEdit" + background);
    ui->regMatchEdit->setStyleSheet("QLineEdit" + background);
    ui->valueDisplay->setStyleSheet("QTableWidget" + background);
    ui->multiString->setStyleSheet("QListWidget" + background);

    teeManager.updateAllTeeBrowserBackground(itsColor);

    // 没有效果
    // this->setStyleSheet("QMainWindow" + background);
    // ui->tabWidget->setStyleSheet("QTabWidget" + background);

    // 可以改，但不从这改
    // ui->statusBar->setStyleSheet("QStatusBar" + background);
    // QPalette palette(this->palette());
    // palette.setColor(QPalette::Background, QColor(0xFFFFFF));
    // this->setPalette(palette);

    plotterManager.updateAllPlotterBackGround(itsColor);
    selectCurrentPlotter()->replot();
    //菜单栏写死咯
    ui->menuBar->setStyleSheet("QMenuBar{ background-color: rgb(240,240,240);}");
}

/**
 * @brief     更新UI面板的字体
 * @param[in] 新的字体
 */
void MainWindow::updateUIPanelFont(QFont font)
{
    ui->textBrowser->document()->setDefaultFont(font);
    ui->textEdit->document()->setDefaultFont(font);
    plotterManager.updateAllPlotterFont(font);
    fft_window->setupPlotterFont(font);
    ui->regMatchBrowser->document()->setDefaultFont(font);
//    ui->multiString->setFont(font);
//    ui->regMatchEdit->setFont(font);
//    ui->valueDisplay->setFont(font);
//    ui->multiString->setFont(font);
}

/**
 * @brief     背景色设置按钮触发
 */
void MainWindow::on_actionBackGroundColorSetting_triggered()
{
    QColor color;
    color = QColorDialog::getColor(g_background_color, this,
                                          tr("选择背景色"),
                                          QColorDialog::ShowAlphaChannel);
    if(!color.isValid())
        return;

    updateUIPanelBackground(color);
}

/**
 * @brief     和校验按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionSumCheck_triggered(bool checked)
{
    ui->actionSumCheck->setChecked(checked);
    g_enableSumCheck = checked;
    setVisualizerTitle();
}

/**
 * @brief     全局弹出热键按钮触发
 */
void MainWindow::on_actionPopupHotkey_triggered()
{
    bool ok;
    QString newKeySeq = QInputDialog::getText(this,
                                              tr("修改全局弹出热键"), tr("新的全局弹出热键："),
                                              QLineEdit::Normal, g_popupHotKeySequence ,
                                              &ok, Qt::WindowCloseButtonHint);
    if(ok==false)
    {
        return;
    }
    registPopupHotKey(newKeySeq);
}

/**
 * @brief     按键按下事件
 * @param[in] 事件值
 */
void MainWindow::keyPressEvent(QKeyEvent *e)
{
//    qDebug()<<"MainWindow::keyPressEvent"<<e->key();
    emit sendKeyToPlotter(e, true);
}

/**
 * @brief     按键松开事件
 * @param[in] 事件值
 */
void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
//    qDebug()<<"MainWindow::keyReleaseEvent"<<e->key();
    emit sendKeyToPlotter(e, false);
}

/**
 * @brief     计算当前窗口能显示多少字符
 * @note      属于耗时函数（几十~几百毫秒）
 */
void MainWindow::calcCharacterNumberInWindow()
{
    QString saveCurrentDataInBrowser; //不要用QByteArray
    QCursor saveCurrentCursor;
    saveCurrentDataInBrowser = ui->textBrowser->toPlainText();
    saveCurrentCursor = ui->textBrowser->cursor();

    //MainWindow尺寸改变时计算当前窗口能显示多少字符。
    //若MainWindow尺寸未变，内部控件尺寸变化的情况使用轮询解决
    ui->textBrowser->setPlainText("");
    //获取列数
    while (ui->textBrowser->document()->lineCount() < 2)
    {
        ui->textBrowser->insertPlainText("0");
    }
    //获取行数
    while(ui->textBrowser->verticalScrollBar()->maximum() == 0)
    {
        ui->textBrowser->appendPlainText("0");
    }

    ui->textBrowser->moveCursor(QTextCursor::Start);
    ui->textBrowser->moveCursor(QTextCursor::Down);
    ui->textBrowser->moveCursor(QTextCursor::Left);

//    qDebug()<< __FUNCTION__
//            << ui->textBrowser->document()->characterCount()
//            << ui->textBrowser->document()->lineCount()
//            << ui->textBrowser->textCursor().columnNumber();

    characterCount_Row = ui->textBrowser->document()->lineCount() - 1;
    characterCount_Col = ui->textBrowser->textCursor().columnNumber() + 1;
    characterCount = characterCount_Col * characterCount_Row;
    characterCount_bak = characterCount;

    ui->textBrowser->setPlainText(saveCurrentDataInBrowser);
    ui->textBrowser->setCursor(saveCurrentCursor);
}

/**
 * @brief     窗口大小改变事件
 * @note      用于估计窗口能显示多少字符
 * @param[in] 事件值
 */
void MainWindow::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event)

    //首次启动不运行，防止卡死
    static uint8_t first_run = 1;
    if(first_run)
    {
        first_run = 0;
        return;
    }

    //只响应显示主窗口时的窗口改变动作，其他类型的窗口只做记录，下次显示主窗口时进行响应
    if(ui->tabWidget->tabText(ui->tabWidget->currentIndex()) != MAIN_TAB_NAME)
    {
        return;
    }

    calcCharacterNumberInWindow();
}

/**
 * @brief     分裂器移动事件的槽
 * @note      用于重新估计窗口能显示多少字符
 * @param[in] 新的位置
 */
void MainWindow::splitterMovedSlot(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);
    // qDebug() << "splitterMovedSlot" << pos << index << ui->widget_input->height();

    //简洁模式控制
    bool trig = false;
    static int32_t lastHeight = 0;
    if( (ui->widget_input->height() == 0 && lastHeight != 0) ||
        (ui->widget_input->height() != 0 && lastHeight == 0))
    {
        trig = true;
    }
    lastHeight = ui->widget_input->height();
    if(trig)
    {
        bool checked = static_cast<bool>(ui->widget_input->height());
        on_actionSimpleMode_triggered(!checked);
        trig = false;
    }

    //计算可显示字符并刷新显示
    calcCharacterNumberInWindow();
    printToTextBrowser();
}

/**
 * @brief     Tee子选项卡关闭请求
 * @param[in] 要关闭的选项卡索引
 */
void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    QString selectName;
    selectName = ui->tabWidget->tabText(index);
    //禁止删除主窗口
    if(selectName == MAIN_TAB_NAME)
    {
        if(!ui->textBrowser->toPlainText().isEmpty())
        {
            QMessageBox::StandardButtons button;
            button = QMessageBox::information(this, tr("提示"),
                                            tr("不允许删除该窗口。是否要清除当前窗口数据？"),
                                            QMessageBox::Ok, QMessageBox::Cancel);
            if(button == QMessageBox::Ok)
            {
                clearTextBrowserSlot();
                return;
            }
        }
        else
        {
            ui->statusBar->showMessage(tr("不允许删除该窗口。"), 2000);
        }
        return;
    }
    //禁止删除匹配窗口
    if(selectName == REGMATCH_TAB_NAME)
    {
        if(!ui->regMatchBrowser->toPlainText().isEmpty())
        {
            QMessageBox::StandardButtons button;
            button = QMessageBox::information(this, tr("提示"),
                                            tr("不允许删除该窗口。是否要清除当前窗口数据？"),
                                            QMessageBox::Ok, QMessageBox::Cancel);
            if(button == QMessageBox::Ok)
            {
                ui->regMatchBrowser->clear();
                return;
            }
        }
        else
        {
            ui->statusBar->showMessage(tr("不允许删除该窗口。"), 2000);
        }
        return;
    }
    emit tee_clearData(selectName);
    if(selectName != nullptr)
    {
        teeManager.removeTeeBrowser(selectName);
        delete ui->tabWidget->widget(index);
    }
//    ui->tabWidget->removeTab(index);//removeTab不会释放对象
}

/**
 * @brief     绘图器选项卡关闭请求
 * @param[in] 要关闭的选项卡索引
 */
void MainWindow::on_tabWidget_plotter_tabCloseRequested(int index)
{
    QString selectName;
    selectName = ui->tabWidget_plotter->tabText(index);
    //禁止删除主窗口
    if(selectName == plotProtocol->getDefaultPlotterTitle())
    {
        if(plotterManager.getDefaultPlotter()->graph()->dataCount())
        {
            QMessageBox::StandardButtons button;
            button = QMessageBox::information(this, tr("提示"),
                                            tr("不允许删除默认绘图窗口。是否要清除该窗口数据？"),
                                            QMessageBox::Ok, QMessageBox::Cancel);
            if(button == QMessageBox::Ok)
            {
                plotterManager.getDefaultPlotter()->clear();
                return;
            }
        }
        else
        {
            ui->statusBar->showMessage(tr("不允许删除默认绘图窗口，请先指定其他默认绘图窗口。"), 2000);
            return;
        }
    }
    emit protocol_clearBuff(selectName);
    plotterManager.removePlotter(selectName);
    if(ui->tabWidget_plotter->widget(index) != nullptr)
    {
        delete ui->tabWidget_plotter->widget(index);
    }
    MyQCustomPlot* plotter = nullptr;
    plotter = selectCurrentPlotter();
    if(plotter)
    {
        plotter->replot();
    }
}

/**
 * @brief     Tee窗口选项卡点击事件
 * @param[in] 被点击的选项卡索引
 */
void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    static int32_t barValue = 0;
    // 记忆失焦时的滚动条位置
    if(ui->tabWidget->tabText(ui->tabWidget->currentIndex()) == MAIN_TAB_NAME)
    {
        barValue = ui->textBrowser->verticalScrollBar()->value();
    }

    ui->tabWidget->setCurrentIndex(index);
    // 当选中main窗口时
    if(ui->tabWidget->tabText(index) == MAIN_TAB_NAME)
    {
        // 需要刷新时才刷新
        if(TryRefreshBrowserCnt && !disableRefreshWindow)
        {
            calcCharacterNumberInWindow();
            printToTextBrowser();
            TryRefreshBrowserCnt = TRY_REFRESH_BROWSER_CNT;
        }
        else
        {
            // 否则恢复滚动条位置
            ui->textBrowser->verticalScrollBar()->setValue(barValue);
        }
    }
}

/**
 * @brief     绘图器选项卡点击事件
 * @param[in] 被点击的选项卡索引
 */
void MainWindow::on_tabWidget_plotter_tabBarClicked(int index)
{
    ui->tabWidget_plotter->setCurrentIndex(index);
    QString selectName = ui->tabWidget_plotter->tabText(index);
    MyQCustomPlot *plotter = nullptr;
    plotter = plotterManager.selectPlotter(selectName);

    if(!plotter)
    {
        return;
    }

    //曲线刷新
    if(ui->actionPlotterSwitch->isChecked())
    {
        if(plotter->getAutoRescaleYAxis())
        {
            plotter->yAxis->rescale(true);
        }
        plotter->replot();   //<20ms
    }
    if(ui->actionValueDisplay->isChecked())
    {
        fillDataToValueDisplay(plotter);
    }
}

/**
 * @brief     拖拽进入事件
 * @note      配合实现拖拽打开文件功能
 * @param[in] 进入事件
 */
void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasFormat("text/uri-list")) //只能打开文本文件
        e->acceptProposedAction(); //可以在这个窗口部件上拖放对象
}

/**
 * @brief     拖拽松开事件
 * @note      实现拖拽打开文件功能
 * @param[in] 松开事件
 */
void MainWindow::dropEvent(QDropEvent *e)
{
    //获取文件路径列表
    if(e->mimeData()->text().indexOf('\n') != -1)
    {
        QMessageBox::information(this, tr("提示"), tr("仅支持单文件解析。"));
        return;
    }
    //e->mimeData()->text()好像是QT bug无法识别{}等符号，这里做一下转换
    QString text = e->mimeData()->text();
    text.replace("%7B", "{");
    text.replace("%7D", "}");
    text.replace("%23", "#");
    text.replace("%25", "%");
    text.replace("%5E", "^");
    text.replace("%60", "`");
    //获取单个文件路径并剔除前缀
    QString path;
    path = text;
    path = path.mid(8);
    if(!path.endsWith("dat"))
    {
        QMessageBox::information(this, tr("提示"), tr("仅支持dat文件解析。"));
        return;
    }
    if(QMessageBox::information(this, tr("提示"),
                                tr("确认解析该文件？") + "\n" +
                                path,
                                QMessageBox::Ok, QMessageBox::Cancel) != QMessageBox::Ok)
    {
        return;
    }
    //开始解包
    unpack_file(readFile, path, false, UNPACK_SIZE_OF_RX);
}

/**
 * @brief     频谱图按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionFFTShow_triggered(bool checked)
{
    ui->actionFFTShow->setChecked(checked);
    if(!fft_window)
        return;
    if(checked)
    {

        //手动选择FFT窗口或者自动选择当前窗口进行FFT
#define SELECT_PLOTTER_TO_FFT
#ifdef SELECT_PLOTTER_TO_FFT
        QVector<MyQCustomPlot*> plotters = plotterManager.getAllPlotters();
        QStringList list;
        foreach(MyQCustomPlot* plotter, plotters)
        {
            if(plotter)
            {
                list << plotter->getPlotterTitle();
            }
        }
        bool ok;
        QString item;
        if(list.size() > 1)
        {
            item = QInputDialog::getItem(this, tr("提示"),
                                                tr("选择要进行FFT的绘图名称："),
                                                list, 0, true, &ok);
            if(!ok)
            {
                ui->actionFFTShow->setChecked(false);
                return;
            }
        }
        else if(list.size() == 1)
        {

            item = list.at(0);
        }
#else
        QString item = selectCurrentPlotter()->getPlotterTitle();
#endif

        QPoint pos = QPoint(this->pos().x() + this->geometry().width(), this->pos().y());
        fft_window->move(pos);
        fft_window->setVisible(true);
        fft_window->setFFTPlotterTitle(item);
        setVisualizerTitle();
        statisticFFTUseCnt++;
        return;
    }
    fft_window->setVisible(false);
    resetVisualizerTitle();//要放在setVisible后面
    return;
}

/**
 * @brief     主窗口移动事件
 * @note      FFT窗口需要跟着主窗口移动
 * @param[in] 移动事件
 */
void MainWindow::moveEvent(QMoveEvent *event)
{
    static QPoint lastPos;
    if(fft_window && fft_window->isVisible())
    {
        QPoint pos = QPoint(event->pos() - lastPos);
        fft_window->move(fft_window->pos() + pos);
    }
    lastPos = event->pos();
}

/**
 * @brief     Tee二级名字提取按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionTeeLevel2NameSupport_triggered(bool checked)
{
    p_textExtract->setLevel2NameSupport(checked);
    ui->actionTeeLevel2NameSupport->setChecked(checked);
}

/**
 * @brief     Tee功能使能按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionTeeSupport_triggered(bool checked)
{
    ui->actionTeeSupport->setChecked(checked);
    ui->actionTeeLevel2NameSupport->setEnabled(checked);
    textExtractEnable = checked;

    if(checked)
    {
        statisticTeeUseCnt++;
    }
}

/**
 * @brief     ASCII表按钮触发
 */
void MainWindow::on_actionASCIITable_triggered()
{
    static Text_Browser_Dialog *p = nullptr;
    statisticAsciiTableUseCnt++;
    if(p)
    {
        p->show();
        return;
    }
    p = new Text_Browser_Dialog(this);
    p->showAsciiTable();
    p->show();
}

/**
 * @brief     更新“功能”按钮标题
 * @note      为了增加Recording标识
 */
void MainWindow::updateFunctionButtonTitle()
{
    if(ui->actionRecordRawData->isChecked() ||
       ui->actionRecordGraphData->isChecked())
    {
        statisticRecordCnt++;
        ui->function->setTitle(tr("功能(Recording)"));
        return;
    }
    ui->function->setTitle(tr("功能"));
    return;
}

/**
 * @brief     记录原始数据按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionRecordRawData_triggered(bool checked)
{
    ui->actionRecordRawData->setChecked(checked);
    if(checked)
    {
        QString savePath;
        QString high_consume_mem_remind;
        if(ui->actionTeeSupport->isChecked() || !ui->regMatchEdit->text().isEmpty())
        {
            high_consume_mem_remind = "\n\n" + tr("检测到数据分窗功能已开启或者filter窗口有数据要匹配，这可能成倍地增加内存消耗。") + "\n"
                                     + tr("如果需要长时间记录数据建议关闭这两个功能。");
        }
        //串口开启状态下则默认保存到程序所在目录，因为选择文件路径的对话框是阻塞型的，串口开启下会影响接收
        if(serial.isOpen() || p_networkComm->isOpen())
        {
            savePath = "Recorder[Raw]-" + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss") + ".dat";
            rawDataRecordPath = QCoreApplication::applicationDirPath() + "/" + savePath;
            p_logger->init_logger(RAW_DATA_LOG, rawDataRecordPath);
            QMessageBox::information(this,
                                     tr("提示"),
                                     tr("数据记录仪初始化完成！") + "\n\n" +
                                     tr("后续数据将被记录到程序所在目录下文件：") + savePath + "\n\n" +
                                     tr("如需更改记录位置，请先关闭串口/网络再使用本功能。") +
                                     high_consume_mem_remind);

        }
        else
        {
            //如果上次文件记录路径是空则用保存数据的上次路径
            if(lastRawDataRecordPath.isEmpty())
            {
                lastRawDataRecordPath = lastFileDialogPath;
            }
            //打开保存文件对话框
            savePath = QFileDialog::getSaveFileName(this,
                                                    tr("记录原始数据到文件-选择文件路径"),
                                                    lastRawDataRecordPath + "Recorder[Raw]-" + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss") + ".dat",
                                                    "DAT File(*.dat);;All File(*.*)");
            //检查路径格式
            if(!savePath.endsWith(".dat")){
                if(!savePath.isEmpty())
                    QMessageBox::information(this, tr("提示"),
                                             tr("尚未支持的文件格式，请选择dat格式文件。"));
                ui->actionRecordRawData->setChecked(false);
                return;
            }
            rawDataRecordPath = savePath;
            p_logger->init_logger(RAW_DATA_LOG, rawDataRecordPath);
            if(ui->actionTeeSupport->isChecked() || !ui->regMatchEdit->text().isEmpty())
            {
                QMessageBox::information(this, tr("提示"),
                                         tr("数据记录仪初始化完成！") + high_consume_mem_remind);
            }
        }
        updateFunctionButtonTitle();
        return;
    }
    lastRawDataRecordPath = rawDataRecordPath;
    lastRawDataRecordPath = lastRawDataRecordPath.mid(0, lastRawDataRecordPath.lastIndexOf('/')+1);
    rawDataRecordPath.clear();
    p_logger->logger_buff_flush(RAW_DATA_LOG);
    updateFunctionButtonTitle();
}

void MainWindow::on_actionHexConverter_triggered(bool checked)
{
    Q_UNUSED(checked)
    static Hex_Tool_Dialog *p = nullptr;
    statisticHexToolUseCnt++;
    if(p)
    {
        p->show();
        return;
    }
    p = new Hex_Tool_Dialog(this);
    p->show();
}

/**
 * @brief     asciiMacth文本编辑窗口文本改变
 * @note      用于重设匹配字符串
 * @param[in] 新的字符串
 */
void MainWindow::on_regMatchEdit_textChanged(const QString &arg1)
{
    if(!p_regMatch)
    {
        return;
    }
    QByteArray temp = arg1.toLocal8Bit();
    foreach(char ch, temp)
    {
        if(ch & 0x80)
        {
            //该操作会再次触发信号
            ui->regMatchEdit->setText("ERROR: Please use ASCII character!");
            return;
        }
    }
//    emit regM_clearData();  //在解析大量数据时会在parsePacksFromBuffer中循环解析，所以不会处理信号的
    regMatchBufferLock.lock();
    regMatchBuffer.clear();
    regMatchBufferLock.unlock();
    p_regMatch->updateRegMatch(arg1, true);
    ui->regMatchBrowser->clear();
}

/**
 * @brief     记录图像数据按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionRecordGraphData_triggered(bool checked)
{
    ui->actionRecordGraphData->setChecked(checked);
    if(checked)
    {
        QString savePath;
        QString high_consume_mem_remind;
        if(ui->actionTeeSupport->isChecked() || !ui->regMatchEdit->text().isEmpty())
        {
            high_consume_mem_remind = "\n\n" + tr("检测到数据分窗功能已开启或者filter窗口有数据要匹配，这可能成倍地增加内存消耗。") + "\n"
                                     + tr("如果需要长时间记录数据建议关闭这两个功能。");
        }
        //串口开启状态下则默认保存到程序所在目录，因为选择文件路径的对话框是阻塞型的，串口开启下会影响接收
        if(serial.isOpen() || p_networkComm->isOpen())
        {
            recordPlotTitle = plotProtocol->getDefaultPlotterTitle();
            savePath = "Recorder[" + recordPlotTitle + "]-" + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss") + ".csv";
            graphDataRecordPath = QCoreApplication::applicationDirPath() + "/" + savePath;
            p_logger->init_logger(GRAPH_DATA_LOG, graphDataRecordPath);
            QMessageBox::information(this,
                                     tr("提示"),
                                     tr("数据记录仪初始化完成！") + "\n\n" +
                                     tr("默认绘图名称") + "[" + recordPlotTitle + "]" +
                                     tr("将被记录到程序所在目录下文件：") + "\n" +
                                     savePath + "\n\n" +
                                     tr("如需更改记录位置或绘图名称，请先关闭串口/网络再使用本功能。") +
                                     high_consume_mem_remind);
        }
        else
        {
            //选择绘图名称
            bool ok;
            QString label;
            label = tr("请选择需要记录的绘图名称。若使用非ASCII协议请选择默认绘图名称。") + "\n\n" +
                    tr("当前默认绘图名称为：") + plotProtocol->getDefaultPlotterTitle();
            QVector<MyQCustomPlot*> plotters = plotterManager.getAllPlotters();
            QStringList items;
            foreach(MyQCustomPlot* plotter, plotters)
            {
                if(plotter)
                {
                    items << plotter->getPlotterTitle();
                }
            }
            items.sort();
            QString text;
            text = QInputDialog::getItem(this, tr("提示"), label, items, 0, true, &ok);
            recordPlotTitle = text;
            if(!ok)
            {
                ui->actionRecordGraphData->setChecked(false);
                return;
            }

            //如果上次文件记录路径是空则用保存数据的上次路径
            if(lastGraphDataRecordPath.isEmpty())
            {
                lastGraphDataRecordPath = lastFileDialogPath;
            }
            //打开保存文件对话框(由于xlsx文件被office打开后导致纸飞机无法写入数据造成数据漏存等因素，因此不推荐用于流式保存)
            savePath = QFileDialog::getSaveFileName(this,
                                                    tr("记录曲线数据到文件-选择文件路径"),
                                                    lastGraphDataRecordPath +
                                                    "Recorder[" + recordPlotTitle + "]-" + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss") + ".csv",
                                                    "CSV File(*.csv);;TXT File(*.txt);;XLSX File[NOT Recommended](*.xlsx);;All File(*.*)");
            //检查路径格式
            if(!savePath.endsWith(".xlsx") && !savePath.endsWith(".csv") && !savePath.endsWith(".txt")){
                if(!savePath.isEmpty())
                    QMessageBox::information(this, tr("提示"),
                                             tr("尚未支持的文件格式，请选择csv/txt/xlsx格式文件。"));
                ui->actionRecordGraphData->setChecked(false);
                return;
            }
            graphDataRecordPath = savePath;
            p_logger->init_logger(GRAPH_DATA_LOG, graphDataRecordPath);
            if(ui->actionTeeSupport->isChecked() || !ui->regMatchEdit->text().isEmpty())
            {
                QMessageBox::information(this, tr("提示"),
                                         tr("数据记录仪初始化完成！") + high_consume_mem_remind);
            }
        }
        updateFunctionButtonTitle();
        return;
    }
    lastGraphDataRecordPath = graphDataRecordPath;
    lastGraphDataRecordPath = lastGraphDataRecordPath.mid(0, lastGraphDataRecordPath.lastIndexOf('/')+1);
    graphDataRecordPath.clear();
    p_logger->logger_buff_flush(GRAPH_DATA_LOG);
    updateFunctionButtonTitle();
}

/**
 * @brief     LOG记录开关按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionLogRecord_triggered(bool checked)
{
    if(!checked)
    {
        qDebug() << "close log record";
    }

    g_log_record = checked;
    ui->actionLogRecord->setChecked(checked);

    if(checked)
    {
        qDebug() << "open log record";
    }
}

/**
 * @brief     CSV协议按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionCSV_triggered(bool checked)
{
    Q_UNUSED(checked)

    if(ui->actionCSV->isChecked())
        statisticCSVUseCnt++;

    QString empty;
    emit protocol_clearBuff(empty);
    plotProtocol->setProtocolType(DataProtocol::CSV);
    ui->actionAscii->setChecked(false);
    ui->actionFloat->setChecked(false);
    ui->actionCSV->setChecked(true);
    ui->actionMAD->setChecked(false);

    setVisualizerTitle();
}

/**
 * @brief     MAD协议按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionMAD_triggered(bool checked)
{
    Q_UNUSED(checked)

    if(ui->actionMAD->isChecked())
        statisticMADUseCnt++;

    QString empty;
    emit protocol_clearBuff(empty);
    plotProtocol->setProtocolType(DataProtocol::MAD);
    ui->actionAscii->setChecked(false);
    ui->actionFloat->setChecked(false);
    ui->actionCSV->setChecked(false);
    ui->actionMAD->setChecked(true);

    setVisualizerTitle();
}

/**
 * @brief     简洁模式的清空按钮
 */
void MainWindow::on_clearWindows_simple_clicked()
{
    on_clearWindows_clicked();
}

/**
 * @brief     简洁模式按钮触发
 * @param[in] 新的状态
 */
void MainWindow::on_actionSimpleMode_triggered(bool checked)
{
    int32_t length;
    QList<int32_t> lengthList;
    ui->actionSimpleMode->setChecked(checked);
    if(checked)
    {
        ui->clearWindows_simple->setVisible(true);
        ui->clearWindows_simple_net->setVisible(true);
        ui->comboBox_targetPort->setMaximumSize(120, 0xFFFFFF);
        //splitter_io垂直间距调整
        length = splitter_io->height();
        lengthList << static_cast<int32_t>(length)
                << static_cast<int32_t>(0);
        splitter_io->setSizes(lengthList);
    }
    else
    {
        ui->clearWindows_simple->setVisible(false);
        ui->clearWindows_simple_net->setVisible(false);
        ui->comboBox_targetPort->setMaximumSize(0xFFFFFF, 0xFFFFFF);
        length = splitter_io->height();
        lengthList << static_cast<int32_t>(length*0.8)
                << static_cast<int32_t>(length*0.2);
        splitter_io->setSizes(lengthList);
    }
}

/**
 * @brief     设置默认绘图器按钮触发
 */
void MainWindow::on_actionSetDefaultPlotterTitle_triggered()
{
    bool ok;
    QString text;
    QString label;
    label = tr("# 请选择常用的绘图窗口作为默认绘图窗口。") + "\n" +
            tr("# 默认绘图窗口将会常驻并且支持保存配置等完整功能。") + "\n" +
            tr("# 后续创建的绘图窗口配置将以默认绘图窗口配置为准。") + "\n" +
            tr("# 若列表没有所想要的窗口名称请先进行绘图操作。") + "\n\n" +
            tr("更改默认绘图窗口：");
    QVector<MyQCustomPlot*> plotters = plotterManager.getAllPlotters();
    QStringList items;
    foreach(MyQCustomPlot* plotter, plotters)
    {
        if(plotter)
        {
            items << plotter->getPlotterTitle();
        }
    }
    text = QInputDialog::getItem(this, tr("提示"),
                                 label,
                                 items,
                                 0,
                                 false,//不可编辑
                                 &ok);
    if(!ok)
    {
        return;
    }
    plotProtocol->setDefaultPlotterTitle(text);
    plotterManager.setDefaultPlotter(text);
}

/**
 * @brief     从缓冲中重新匹配数据
 */
void MainWindow::on_regMatchSwitch_clicked(bool checked)
{
    Q_UNUSED(checked)

    /* 单击式，所以这个目前没用 */
//    if(!checked)
//    {
//        emit regM_clearData();
//        ui->regMatchBrowser->clear();
//        regMatchBufferLock.lock();
//        regMatchBuffer.clear();
//        regMatchBufferLock.unlock();
//        return;
//    }

    if(ui->regMatchEdit->text().isEmpty())
    {
        QMessageBox::information(this, tr("提示"), tr("请输入要过滤的关键字符。"));
        return;
    }

    #define MAX_MATCH_SIZE (0.5 * 1024 * 1024)
    QByteArray arr;
    if(RxBuff.size() > MAX_MATCH_SIZE)
    {
        static int32_t current = 0;
        QString label;
        QString item;
        QStringList items;
        bool ok;
        label = tr("当前缓冲较多，请选择想要过滤的数据量：") + "\n"
                + tr("（数据量增大将增加处理时间。）");
        items << "1MB" << "1.5MB" << "2MB" << "ALL";
        item = QInputDialog::getItem(this, tr("提示"),
                                     label, items, current, false, &ok);
        if(!ok)
        {
            return;
        }
        current = items.indexOf(item);
        item.remove("MB");
        int32_t match_size = item.toFloat(&ok) * 1024 * 1024;
        if(!ok)
        {
            arr = RxBuff;
        }
        else
        {
            int32_t temp_size = RxBuff.size();
            match_size = match_size > temp_size ? temp_size : match_size;
            arr = RxBuff.mid(RxBuff.size() - match_size);
        }
    }
    else
    {
        arr = RxBuff;
    }

    ui->regMatchBrowser->clear();
    emit regM_appendData(arr);
    emit regM_parseData();
}

/**
 * @brief     运算优先级表触发
 * @note      打开运算优先级表
 */
void MainWindow::on_actionPriorityTable_triggered()
{
    static Text_Browser_Dialog *p = nullptr;
    statisticPriorityTableUseCnt++;
    if(p)
    {
        p->show();
        return;
    }
    p = new Text_Browser_Dialog(this);
    p->showPriorityTable();
    p->show();
}

/**
 * @brief     设置颜色主题
 * @param[in] 颜色主题序号
 */
void MainWindow::setWindowTheme(int32_t themeIndex)
{
    //default
    QFile styleFile;
    QString style;
    QPalette thisPalette(this->palette());
    switch(themeIndex)
    {
        case 0:
            styleFile.setFileName(":/style.css");
            styleFile.open(QFile::ReadOnly);
            style = styleFile.readAll();
            styleFile.close();
            this->setStyleSheet(style);
            updateUIPanelBackground(QColor(Qt::white));
            ui->statusBar->setStyleSheet("QStatusBar{ background-color: rgb(240,240,240);}");
            this->setAutoFillBackground(true);
            this->setPalette(QPalette(QColor(240, 240, 240)));
        break;

        case 1:
            styleFile.setFileName(":/qss/flatwhite.css");
            styleFile.open(QFile::ReadOnly);
            style = styleFile.readAll();
            styleFile.close();
            this->setStyleSheet(style);
            updateUIPanelBackground(QColor(Qt::white));
            ui->statusBar->setStyleSheet("QStatusBar{ background-color: rgb(255,255,255);}");
            thisPalette.setColor(QPalette::Background, QColor(0xFFFFFF));
            this->setPalette(thisPalette);
        break;
        default:
            themeIndex = 0;
            styleFile.setFileName(":/style.css");
            styleFile.open(QFile::ReadOnly);
            style = styleFile.readAll();
            styleFile.close();
            this->setStyleSheet(style);
            ui->statusBar->setStyleSheet("QStatusBar{ background-color: rgb(240,240,240);}");
            this->setAutoFillBackground(true);
            this->setPalette(QPalette(QColor(240, 240, 240)));
            qDebug() << "unmatched themeIndex" << themeIndex << "at" << __FUNCTION__;
            break;
    }
    g_theme_index = themeIndex;
}

/**
 * @brief     设置颜色主题动作触发
 */
void MainWindow::on_actionSelectTheme_triggered()
{
    bool ok;
    QStringList items;
    QString theme;
    items << "0:default" << "1:flatwhite";
    theme = QInputDialog::getItem(this, tr("提示"),
                                 tr("颜色主题"),
                                 items,
                                 g_theme_index,
                                 false,//不可编辑
                                 &ok);
    if(!ok)
    {
        return;
    }

    int32_t index = theme.mid(0, 1).toInt();
    setWindowTheme(index);
}

/**
 * @brief     往浏览器添加调试信息
 */
void MainWindow::appendMsgLogToBrowser(QString str)
{
    str = QDateTime::currentDateTime().toString("[hh:mm:ss]# ") + str + "\n";
    BrowserBuff.append(str);
    hexBrowserBuff.append(toHexDisplay(str.toLocal8Bit()));
    printToTextBrowser();
}

/**
 * @brief     更改通信模式：网络通信、串口通信
 */
void MainWindow::changeCommMode(bool isNetworkComm)
{
    //隐藏所有相关控件，再选择性打开
    ui->widget_ctrl->setVisible(false);
    ui->widget_net_ctrl->setVisible(false);
    ui->actionCOM_Config->setVisible(false);

    g_network_comm_mode = isNetworkComm;
    if(g_network_comm_mode)
    {
        //网络通信模式
        on_comSwitch_clicked(false);
        QRegExp validAddrFormat;
        validAddrFormat.setPattern("\\d{0,3}\\.\\d{0,3}\\.\\d{0,3}\\.\\d{0,3}");
        ui->comboBox_targetIP->setValidator(new QRegExpValidator(validAddrFormat, this));
        validAddrFormat.setPattern("\\d{0,5}");
        ui->comboBox_targetPort->setValidator(new QRegExpValidator(validAddrFormat, this));
        on_networkModeBox_activated("TCP Server");
        ui->widget_net_ctrl->setVisible(true);
    }
    else
    {
        ui->actionCOM_Config->setVisible(true);
        on_networkSwitch_clicked(false);
        //串口通信模式
        ui->widget_ctrl->setVisible(true);
        refreshCom();
    }
}

/**
 * @brief     更新网络开关文本
 * @param[in] 网络模式
 * @param[in] 是否为按下状态
 */
void MainWindow::updateNetworkSwitchText(const QString &networkMode, bool pressed)
{
    if(networkMode == "TCP Server")
    {
        if(pressed)
        {
            ui->networkSwitch->setText(tr("断开"));
        }
        else
        {
            ui->networkSwitch->setText(tr("监听"));
        }
    }
    else if(networkMode == "TCP Client")
    {
        if(pressed)
        {
            ui->networkSwitch->setText(tr("关闭"));
        }
        else
        {
            ui->networkSwitch->setText(tr("打开"));
        }
    }
    else if(networkMode == "UDP Server")
    {
        if(pressed)
        {
            ui->networkSwitch->setText(tr("断开"));
        }
        else
        {
            ui->networkSwitch->setText(tr("监听"));
        }
    }
    else if(networkMode == "UDP Client")
    {
        if(pressed)
        {
            ui->networkSwitch->setText(tr("关闭"));
        }
        else
        {
            ui->networkSwitch->setText(tr("打开"));
        }
    }
}

/**
 * @brief     切换网络模式
 */
void MainWindow::on_networkModeBox_activated(const QString &arg1)
{
    static QString last_arg1;
    if(ui->networkSwitch->isChecked())
    {
        qDebug() << "can not change network mode when working" << __FUNCTION__;
        return;
    }
    ui->networkModeBox->setCurrentText(arg1);

    //先重置状态（切换可编辑性后字体重置了，可能是bug）
    ui->comboBox_targetIP->setEditable(true);
    ui->comboBox_targetPort->setEditable(true);
    ui->comboBox_remoteAddr->setEditable(true);
    ui->comboBox_targetIP->setStyleSheet("QComboBox{ font-family: Microsoft YaHei UI;}");
    ui->comboBox_targetPort->setStyleSheet("QComboBox{ font-family: Microsoft YaHei UI;}");
    ui->comboBox_remoteAddr->setStyleSheet("QComboBox{ font-family: Microsoft YaHei UI;}");
    ui->comboBox_remoteAddr->setVisible(false);
    ui->label_remoteAddr->setVisible(false);

    //在Server和Client切换时备份/恢复Client模式下的targetIP
    if((last_arg1 == "TCP Client" || last_arg1 == "UDP Client")
        && (arg1 == "TCP Server" || arg1 == "UDP Server"))
    {
        client_targetIP_backup_List.clear();
        for(int32_t i = 0; i < ui->comboBox_targetIP->count(); i++)
        {
            client_targetIP_backup_List << ui->comboBox_targetIP->itemText(i);
        }
    }
    else if((last_arg1 == "TCP Server" || last_arg1 == "UDP Server")
            && (arg1 == "TCP Client" || arg1 == "UDP Client"))
    {
        if(client_targetIP_backup_List.isEmpty())
        {
            if(arg1 != "TCP Client")
                client_targetIP_backup_List << p_networkComm->getLocalIP();
            client_targetIP_backup_List << "127.0.0.1";
        }
        ui->comboBox_targetIP->clear();
        ui->comboBox_targetIP->addItems(client_targetIP_backup_List);
    }

    //在切到TCP Server或者UDP Server时备份/恢复远端地址
    if(arg1 == "TCP Server")
    {
        server_remoteAddr_backup_list.clear();
        for(int32_t i = 0; i < ui->comboBox_remoteAddr->count(); i++)
        {
            server_remoteAddr_backup_list << ui->comboBox_remoteAddr->itemText(i);
        }
    }
    else if(arg1 == "UDP Server")
    {
        if(server_remoteAddr_backup_list.isEmpty())
        {
            server_remoteAddr_backup_list << p_networkComm->getLocalIP() + ":" + ui->comboBox_targetPort->currentText();
            server_remoteAddr_backup_list << "127.0.0.1:" + ui->comboBox_targetPort->currentText();
        }
        ui->comboBox_remoteAddr->clear();
        ui->comboBox_remoteAddr->addItems(server_remoteAddr_backup_list);
    }

    if(arg1 == "TCP Server")
    {
        ui->label_targetIP->setText(tr("监听地址"));
        ui->label_targetPort->setText(tr("监听端口"));
        ui->networkSwitch->setText(tr("监听"));

        ui->comboBox_targetIP->clear();
        ui->comboBox_targetIP->setEditable(false);
        ui->comboBox_targetIP->addItem(p_networkComm->getLocalIP());
        ui->comboBox_targetIP->addItem("127.0.0.1");

        ui->comboBox_remoteAddr->clear();
        ui->comboBox_remoteAddr->setEditable(false);
        ui->comboBox_remoteAddr->setVisible(true);
        ui->label_remoteAddr->setVisible(true);
        ui->label_remoteAddr->setText(tr("当前连接"));
    }
    else if(arg1 == "TCP Client")
    {
        ui->label_targetIP->setText(tr("远端地址"));
        ui->label_targetPort->setText(tr("远端端口"));
        ui->networkSwitch->setText(tr("打开"));
    }
    else if(arg1 == "UDP Server")
    {
        ui->label_targetIP->setText(tr("监听地址"));
        ui->label_targetPort->setText(tr("监听端口"));
        ui->networkSwitch->setText(tr("监听"));

        ui->comboBox_targetIP->clear();
        ui->comboBox_targetIP->setEditable(false);
        ui->comboBox_targetIP->addItem(p_networkComm->getLocalIP());
        ui->comboBox_targetIP->addItem("127.0.0.1");

        ui->comboBox_remoteAddr->setEditable(true);
        ui->comboBox_remoteAddr->setVisible(true);
        ui->label_remoteAddr->setVisible(true);
        ui->label_remoteAddr->setText(tr("远端地址"));
    }
    else if(arg1 == "UDP Client")
    {
        ui->label_targetIP->setText(tr("远端地址"));
        ui->label_targetPort->setText(tr("远端端口"));
        ui->networkSwitch->setText(tr("打开"));
    }
    last_arg1 = arg1;
}

/**
 * @brief     设置所有网络相关UI的使能状态
 * @param[in] 使能状态
 */
void MainWindow::setAllNetControlPanelEnabled(bool enable)
{
    ui->comboBox_targetPort->setEnabled(enable);
    ui->networkModeBox->setEnabled(enable);
//    ui->label_targetIP->setEnabled(enable);
    ui->comboBox_targetIP->setEnabled(enable);
    if(ui->networkModeBox->currentText() == "TCP Server"
      || ui->networkModeBox->currentText() == "UDP Server")
    {
        ui->comboBox_remoteAddr->setEnabled(true);
        ui->label_remoteAddr->setEnabled(true);
    }
    else
    {
        ui->comboBox_remoteAddr->setEnabled(enable);
//        ui->label_remoteAddr->setEnabled(enable);
    }
}

/**
 * @brief     检查网络地址的输入合法性
 */
bool MainWindow::checkIPAddrIsValid(void)
{
    QString errMsg;
    QRegExp validAddrFormat;
    validAddrFormat.setPattern("\\d{1,3}.\\d{1,3}.\\d{1,3}.\\d{1,3}");
    if(ui->comboBox_targetIP->isVisible()
       && !ui->comboBox_targetIP->currentText().contains(validAddrFormat))
    {
        errMsg = ui->label_targetIP->text();
        goto failed;
    }
    validAddrFormat.setPattern("\\d{1,5}");
    if(ui->comboBox_targetPort->isVisible()
       && !ui->comboBox_targetPort->currentText().contains(validAddrFormat))
    {
        errMsg = ui->label_targetPort->text();
        goto failed;
    }
    validAddrFormat.setPattern("\\d{1,3}.\\d{1,3}.\\d{1,3}.\\d{1,3}:\\d{1,5}");
    if(!ui->comboBox_remoteAddr->currentText().isEmpty()
       && ui->comboBox_remoteAddr->isVisible()
       && !ui->comboBox_remoteAddr->currentText().contains(validAddrFormat))
    {
        errMsg = ui->label_remoteAddr->text();
        goto failed;
    }
    return true;
failed:
    QMessageBox::information(this, tr("提示"),
                             tr("非法的") + errMsg);
    return false;
}

/**
 * @brief     添加网络地址到下拉列表中
 * @note      建议先验证地址合法性
 */
void MainWindow::addNetAddrToComBoBox(void)
{
    if(ui->comboBox_targetIP->findText(ui->comboBox_targetIP->currentText()) == -1)
    {
        if(ui->comboBox_targetIP->currentText().isEmpty())
            return;
        ui->comboBox_targetIP->addItem(ui->comboBox_targetIP->currentText());
    }
    if(ui->comboBox_targetPort->findText(ui->comboBox_targetPort->currentText()) == -1)
    {
        if(ui->comboBox_targetPort->currentText().isEmpty())
            return;
        ui->comboBox_targetPort->addItem(ui->comboBox_targetPort->currentText());
    }
    if(ui->comboBox_remoteAddr->findText(ui->comboBox_remoteAddr->currentText()) == -1)
    {
        if(ui->comboBox_remoteAddr->currentText().isEmpty())
            return;
        ui->comboBox_remoteAddr->addItem(ui->comboBox_remoteAddr->currentText());
    }
}

/**
 * @brief     网络开关按钮
 * @note      连接/断开网络
 * @param[in] 开关动作
 */
void MainWindow::on_networkSwitch_clicked(bool checked)
{
    QString arg1 = ui->networkModeBox->currentText();
    if(checked)
    {
        setAllNetControlPanelEnabled(true);

        if(!checkIPAddrIsValid())
        {
            goto failed;
        }

        if(arg1 == "TCP Server")
        {
            emit connectToNetwork(TCP_SERVER,
                                  ui->comboBox_targetIP->currentText(),
                                  ui->comboBox_targetPort->currentText().toInt());
        }
        else if(arg1 == "TCP Client")
        {
            emit connectToNetwork(TCP_CLIENT,
                                  ui->comboBox_targetIP->currentText(),
                                  ui->comboBox_targetPort->currentText().toInt());
        }
        else if(arg1 == "UDP Server")
        {
            QString addr = ui->comboBox_remoteAddr->currentText();
            p_networkComm->setRemoteUdpAddr(addr.mid(0, addr.indexOf(':')),
                                             addr.mid(addr.indexOf(':') + 1).toUInt());
            emit connectToNetwork(UDP_SERVER,
                                  ui->comboBox_targetIP->currentText(),
                                  ui->comboBox_targetPort->currentText().toInt());
        }
        else if(arg1 == "UDP Client")
        {
            emit connectToNetwork(UDP_CLIENT,
                                  ui->comboBox_targetIP->currentText(),
                                  ui->comboBox_targetPort->currentText().toInt());
        }
        //把网络地址加入combobox
        addNetAddrToComBoBox();
        setAllNetControlPanelEnabled(false);
        updateNetworkSwitchText(arg1, true);
        return;
    }
    //关闭定时器
    if(cycleSendTimer.isActive()){
        cycleSendTimer.stop();
        ui->cycleSendCheck->setChecked(false);
    }
    emit disconnectFromNetwork();
failed:
    ui->networkSwitch->setChecked(false);
    setAllNetControlPanelEnabled(true);
    updateNetworkSwitchText(arg1, false);
    return;
}

/**
 * @brief     网络模式切换
 * @note      切换TCP、UDP等
 * @param[in] 切换动作
 */
void MainWindow::on_actionNetworkMode_triggered(bool checked)
{
    if(ui->comSwitch->isChecked() || ui->networkSwitch->isChecked())
    {
        QMessageBox::information(this, tr("提示"),
                                 tr("请先关闭串口/网络后再进行操作。"));
        ui->actionNetworkMode->setChecked(!checked);
        return;
    }
    ui->actionNetworkMode->setChecked(checked);
    changeCommMode(checked);
}

/**
 * @brief     网络错误信息
 * @note      集合了TCP、UDP等各模式的错误
 * @param[in] 错误码
 * @param[in] 人类可读的错误细节
 */
void MainWindow::errorNetwork(qint32 err, QString details)
{
    switch (err) {
    case NET_ERR_CONNECT:
        emit disconnectFromNetwork();
        ui->networkSwitch->setChecked(false);
        setAllNetControlPanelEnabled(true);
        updateNetworkSwitchText(ui->networkModeBox->currentText(), false);
        QMessageBox::information(this, tr("提示"),
                                 tr("网络打开失败。") + "\n"
                                 + details);
        break;
    case NET_ERR_DISCONNECT:
        ui->networkSwitch->setChecked(false);
        setAllNetControlPanelEnabled(true);
        updateNetworkSwitchText(ui->networkModeBox->currentText(), false);
        ui->statusBar->showMessage(details, 2000);
        if(ui->networkModeBox->currentText() == "TCP Client")
        {
            appendMsgLogToBrowser("Client disconnected by server.");
        }
        break;
    case NET_ERR_WRITE:
        ui->statusBar->showMessage(tr("网络数据发送异常。"), 2000);
        appendMsgLogToBrowser("WRITE_ERR: " + details);
        break;
    case NET_ERR_READ:
        ui->statusBar->showMessage(tr("网络数据接收异常。"), 2000);
        appendMsgLogToBrowser("READ_ERR: " + details);
        break;
    case NET_ERR_MULTI_SOCKET:
        // QMessageBox::information(this, tr("提示"),
        //                          tr("连接已拒绝。") + "\n"
        //                          + tr("暂未支持多客户端连接。"));
        ui->statusBar->showMessage(details, 2000);
        appendMsgLogToBrowser(details);
        break;
    case NET_ERR_ACCEPT_ERR:
        ui->statusBar->showMessage(details, 2000);
        appendMsgLogToBrowser(details);
        break;
    case NET_ERR_SOCKET_ERR:
        ui->statusBar->showMessage(details, 2000);
        appendMsgLogToBrowser(details);
        break;
    default:
        qDebug() << __FUNCTION__ << "unknown err" << err << details;
        break;
    }
}

/**
 * @brief     网络状态信息更新槽
 * @note      正常的网络连接、断开等信息的同步
 * @param[in] 信息类型
 * @param[in] 信息文本
 */
void MainWindow::msgNetwork(qint32 type, QString msg)
{
    switch (type) {
    case NET_MSG_NEW_CONNECT:
        if(ui->networkModeBox->currentText() == "TCP Server")
        {
            if(ui->comboBox_remoteAddr->findText(msg) == -1)
            {
                ui->comboBox_remoteAddr->addItem(msg);
            }
            ui->comboBox_remoteAddr->setCurrentText(msg);
        }
        else if(ui->networkModeBox->currentText() == "UDP Server")
        {
            if(ui->comboBox_remoteAddr->findText(msg) == -1)
            {
                ui->comboBox_remoteAddr->addItem(msg);
            }
            ui->comboBox_remoteAddr->setCurrentText(msg);
        }
        break;
    case NET_MSG_DISCONNECT:
        if(ui->networkModeBox->currentText() == "TCP Server")
        {
            int32_t index = 0;
            index = ui->comboBox_remoteAddr->findText(msg);
            ui->comboBox_remoteAddr->removeItem(index);
        }
        break;
    default:
        qDebug() << __FUNCTION__ << "unknown msg" << msg;
        break;
    }
}

void MainWindow::readDataNetwork(const QByteArray &data)
{
    networkRxBuff.append(data);
    readSerialPort();
}

/**
 * @brief     远端地址激活了新的选项
 * @param[in] 新的选项
 */
void MainWindow::on_comboBox_remoteAddr_activated(const QString &arg1)
{
    ui->comboBox_remoteAddr->setCurrentText(arg1);
    if(ui->networkModeBox->currentText() == "UDP Server")
    {
        if(!checkIPAddrIsValid())
            return;
        p_networkComm->setRemoteUdpAddr(arg1.mid(0, arg1.indexOf(':')),
                                         arg1.mid(arg1.indexOf(':') + 1).toUInt());
    }
}

/**
 * @brief     远端地址当前文本修改动作触发
 * @note      textChanged事件内不要用setText否则每次修改后光标会调到文本末尾
 * @param[in] 新的文本
 */
void MainWindow::on_comboBox_remoteAddr_currentTextChanged(const QString &arg1)
{
    if(arg1.isEmpty())
        return;

    if(ui->networkModeBox->currentText() == "UDP Server")
    {
        QString errMsg;
        QRegExp validAddrFormat;
        validAddrFormat.setPattern("\\d{1,3}.\\d{1,3}.\\d{1,3}.\\d{1,3}:\\d{1,5}");
        if(ui->comboBox_remoteAddr->isVisible()
           && !arg1.contains(validAddrFormat))
        {
            return;
        }

        p_networkComm->setRemoteUdpAddr(arg1.mid(0, arg1.indexOf(':')),
                                         arg1.mid(arg1.indexOf(':') + 1).toUInt());
        //不自动添加进combobox，否则配合自动补全功能，会把没输入完的地址添加进去然后又给你补全提示
    }
}

/**
 * @brief     网络模式清空按钮按下
 */
void MainWindow::on_clearWindows_simple_net_clicked()
{
    on_clearWindows_clicked();
}
