#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHotkey>

#define RECORDER_FILE_PATH           "ComAssistantRecorder.dat"
#define BACKUP_RECORDER_FILE_PATH    "ComAssistantRecorder_back.dat"

static qint32   g_xAxisSource = XAxis_Cnt;
static qint32   g_multiStr_cur_index;
static QColor   g_background_color;
static QFont    g_font;
static bool     g_enableSumCheck;
static qint64   g_lastSecsSinceEpoch;
static QString  g_popupHotKeySequence;
static QHotkey  *g_popupHotkey = new QHotkey(nullptr);
//注册全局快捷键
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
        QMessageBox::information(this, tr("提示"), tr("全局弹出热键已被占用")+ "[" + keySequence +"]");
        return false;
    }
}

/*
 * Function:读取配置
*/
void MainWindow::readConfig()
{
    //先写入版本号和启动时间
    Config::setVersion();
    Config::setStartTime(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));

    //发送注释
    on_actionSendComment_triggered(Config::getSendComment());
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

    //多字符串
    ui->actionMultiString->setChecked(Config::getMultiStringState());
    on_actionMultiString_triggered(Config::getMultiStringState());
    QStringList multi;
    multi = Config::getMultiString();
    while(multi.size()>0){
        ui->multiString->addItem(multi.at(0));
        multi.pop_front();
    }

    //文件对话框路径
    lastFileDialogPath = Config::getLastFileDialogPath();
    //加载高亮规则
    ui->actionKeyWordHighlight->setChecked(Config::getKeyWordHighlightState());
    on_actionKeyWordHighlight_triggered(Config::getKeyWordHighlightState());

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
    qint32 r,g,b;
    QColor color = Config::getBackGroundColor();
    if(color.isValid()){
        g_background_color = color;
        g_background_color.getRgb(&r,&g,&b);
    }else{
        r=g=b=255;
        g_background_color.setRed(r);
        g_background_color.setGreen(g);
        g_background_color.setBlue(b);
    }
    QString str = "QPlainTextEdit{ background-color: rgb(RGBR,RGBG,RGBB);}";
    str.replace("RGBR", QString::number(r));
    str.replace("RGBG", QString::number(g));
    str.replace("RGBB", QString::number(b));
    ui->textBrowser->setStyleSheet(str);

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

    //轴标签
    ui->customPlot->xAxis->setLabel(Config::getXAxisName());
    ui->customPlot->yAxis->setLabel(Config::getYAxisName());
    //数值显示器
    ui->actionValueDisplay->setChecked(Config::getValueDisplayState());
    on_actionValueDisplay_triggered(Config::getValueDisplayState());
    //图像名字集
    ui->customPlot->plotControl->setNameSet(Config::getPlotterGraphNames(ui->customPlot->plotControl->getMaxValidGraphNumber()));
    //OpenGL
    ui->actionOpenGL->setChecked(Config::getOpengGLState());
    on_actionOpenGL_triggered(Config::getOpengGLState());

    //refreshYAxis
    on_actionAutoRefreshYAxis_triggered(Config::getRefreshYAxisState());
    //line type
    switch (Config::getLineType()) {
    case LineType_e::Line:
        on_actionLinePlot_triggered();
        break;
    case LineType_e::Scatter_Line:
        on_actionScatterLinePlot_triggered();
        break;
    case LineType_e::Scatter:
        on_actionScatterPlot_triggered();
        break;
    default:
        on_actionLinePlot_triggered();
        break;
    }

    //注册全局呼出快捷键
    registPopupHotKey(Config::getPopupHotKey());
}

void MainWindow::adjustLayout()
{
    qint32 length;
    QList<qint32> lengthList;

    //splitter_io垂直间距调整
    length = splitter_io->height();
    lengthList.clear();
    lengthList << static_cast<qint32>(length*0.8)
               << static_cast<qint32>(length*0.2);
    splitter_io->setSizes(lengthList);
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
    central = new QVBoxLayout(this);
    central->addWidget(ui->widget_ctrl);
    central->addWidget(splitter_io);
    ui->centralWidget->setLayout(central);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)

{
    ui->setupUi(this);

    //槽
    connect(this, SIGNAL(parseFileSignal()),this,SLOT(parseFileSlot()));
    connect(&cycleSendTimer, SIGNAL(timeout()), this, SLOT(cycleSendTimerSlot()));
    connect(&secTimer, SIGNAL(timeout()), this, SLOT(secTimerSlot()));
    connect(&printToTextBrowserTimer, SIGNAL(timeout()), this, SLOT(printToTextBrowserTimerSlot()));
    connect(&plotterShowTimer, SIGNAL(timeout()), this, SLOT(plotterShowTimerSlot()));
    connect(&multiStrSeqSendTimer, SIGNAL(timeout()), this, SLOT(multiStrSeqSendTimerSlot()));
    connect(&serial, SIGNAL(readyRead()), this, SLOT(readSerialPort()));
    connect(&serial, SIGNAL(bytesWritten(qint64)), this, SLOT(serialBytesWritten(qint64)));
    connect(&serial, SIGNAL(error(QSerialPort::SerialPortError)),  this, SLOT(handleSerialError(QSerialPort::SerialPortError)));
    connect(this, SIGNAL(sendKeyToPlotter(QKeyEvent*, bool)), ui->customPlot, SLOT(recvKey(QKeyEvent*, bool)));

    connect(ui->textBrowser->verticalScrollBar(),SIGNAL(actionTriggered(int)),this,SLOT(verticalScrollBarActionTriggered(int)));

    //状态栏标签
    statusRemoteMsgLabel = new QLabel(this);
    statusSpeedLabel = new QLabel(this);
    statusStatisticLabel = new QLabel(this);
    statusTimer = new QLabel(this);
    statusTimer->setText("Timer:" + formatTime(0));
    statusRemoteMsgLabel->setOpenExternalLinks(true);//可打开外链
    ui->statusBar->addPermanentWidget(statusRemoteMsgLabel);
    ui->statusBar->addPermanentWidget(statusTimer);
    ui->statusBar->addPermanentWidget(statusStatisticLabel);//显示永久信息
    ui->statusBar->addPermanentWidget(statusSpeedLabel);

    //设置波特率框和发送间隔框的合法输入范围
    ui->baudrateList->setValidator(new QIntValidator(0,9999999,this));
    ui->sendInterval->setValidator(new QIntValidator(0,99999,this));
    ui->timeStampTimeOut->setValidator(new QIntValidator(0,99999,this));

    //加载高亮规则
    on_actionKeyWordHighlight_triggered(ui->actionKeyWordHighlight->isChecked());

    //fft
    fft_window = new FFT_Dialog(ui->actionFFTShow, this);

    //初始化绘图控制器
    ui->customPlot->init(ui->statusBar, ui->plotterSetting,
                         ui->actionSavePlotData, ui->actionSavePlotAsPicture,
                         &g_xAxisSource, &autoRefreshYAxisFlag,
                         fft_window);

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

    //http
    http = new HTTP(this);

    //设置窗体布局
    layoutConfig();

    //绑定分裂器移动槽
    connect(ui->splitter_display, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMovedSlot(int, int)));
    connect(splitter_io, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMovedSlot(int, int)));
    connect(splitter_output, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMovedSlot(int, int)));

    //显示收发统计
    serial.resetCnt();
    statusStatisticLabel->setText(serial.getTxRxString());

    //搜寻可用串口，并尝试打开
    on_refreshCom_clicked();
    tryOpenSerial();

    //数值显示器初始化
    ui->valueDisplay->setColumnCount(2);
    ui->valueDisplay->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("名称")));
    ui->valueDisplay->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("值")));
    ui->valueDisplay->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->valueDisplay->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->valueDisplay->horizontalHeader()->setStretchLastSection(true);

    //加载样式表
    QFile styleFile(":/style.css");
    styleFile.open(QFile::ReadOnly);
    QString style = styleFile.readAll();
    styleFile.close();
    this->setStyleSheet(style);
    g_font = Config::getGUIFont();
    ui->textBrowser->document()->setDefaultFont(g_font);
    ui->textEdit->document()->setDefaultFont(g_font);
    ui->multiString->setFont(g_font);
    ui->customPlot->plotControl->setupFont(g_font);
    fft_window->setupPlotterFont(g_font);

    this->setWindowTitle(tr("纸飞机串口助手") + " - V"+Config::getVersion());

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

    //计时器
    g_lastSecsSinceEpoch = QDateTime::currentSecsSinceEpoch();

    //显示界面
    this->show();

    //读取配置（所有资源加载完成后读取，有些配置需要根据可见性改变所以显示后读取）
    readConfig();

    //调整窗体布局
    adjustLayout();
    //是否首次运行
    if(Config::getFirstRun()){
        QMessageBox::information(this, tr("提示"), tr("欢迎使用纸飞机串口调试助手。\n\n由于阁下是首次运行，接下来会弹出帮助文件和相关声明，请认真阅读。\n\n若阁下继续使用本软件则代表阁下接受并同意相关声明，\n否则请自行关闭软件。"));
        //弹出帮助文件
        on_actionManual_triggered();
        //弹出声明
        on_actionAbout_triggered();        
    }

    readRecorderFile();

}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
}

//打开可交互控件
void MainWindow::openInteractiveUI()
{
    ui->comSwitch->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->multiString->setEnabled(true);
    ui->cycleSendCheck->setEnabled(true);
    ui->clearWindows->setText(tr("清  空"));
}

//关闭可交互控件
void MainWindow::closeInteractiveUI()
{
    ui->comSwitch->setEnabled(false);
    ui->sendButton->setEnabled(false);
    ui->multiString->setEnabled(false);
    cycleSendTimer.stop();
    ui->cycleSendCheck->setEnabled(false);
    ui->cycleSendCheck->setChecked(false);
    ui->clearWindows->setText(tr("中  止"));
}

//文件分包函数，所有传入的参数都可能被修改
int32_t MainWindow::divideDataToPacks(QByteArray &input, QByteArrayList &output, int32_t pack_size, bool &divideFlag)
{
    closeInteractiveUI();

    int32_t pack_num = input.size() / pack_size;
    int32_t pack_cnt = 0;
    while(input.size() > pack_size && divideFlag){
        output.append(input.mid(0, pack_size));
        input.remove(0, pack_size);
        ui->statusBar->showMessage(tr("文件分包进度：") +
                                   QString::number(100 * pack_cnt /pack_num) +
                                   "%", 2000);
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

int32_t MainWindow::parseDatFile(QString path, bool removeAfterRead)
{
    QFile file(path);
    QByteArray buff;
    //读文件
    if(file.open(QFile::ReadOnly)){
        on_clearWindows_clicked();
        buff.clear();
        buff = file.readAll();
        file.close();
        if(removeAfterRead)
        {
            file.remove();
        }

        //文件分包
        #define PACKSIZE 4096
        parseFileBuffIndex = 0;
        parseFileBuff.clear();
        parseFile = true;
        if(divideDataToPacks(buff, parseFileBuff, PACKSIZE, parseFile))
            return -1;
        RxBuff.clear();

        ui->textBrowser->clear();
        ui->textBrowser->appendPlainText("File size: " + QString::number(file.size()) + " Byte");
        ui->textBrowser->appendPlainText("Read containt:\n");
        BrowserBuff.clear();
        BrowserBuff.append(ui->textBrowser->toPlainText());

        // 解析读取的数据
        unshowedRxBuff.clear();
        emit parseFileSignal();
    }else{
        QMessageBox::information(this,tr("提示"),tr("文件打开失败。"));
        return -1;
    }
    return 0;
}

//启动时检测有无记录数据文件
void MainWindow::readRecorderFile()
{
    QFile recorderFile(RECORDER_FILE_PATH);
    if(recorderFile.exists() && recorderFile.size())
    {
        qint32 button;
        button = QMessageBox::information(this, tr("提示"),
                                          tr("检测到数据记录文件，可能是上次未正确关闭程序导致的。") + "\n\n" +
                                          tr("点击Ok恢复上次数据") + "\n" +
                                          tr("点击Discard丢弃上次数据") + "\n" +
                                          tr("点击Cancel自行处理上次数据") + "\n",
                                          QMessageBox::Ok, QMessageBox::Discard, QMessageBox::Cancel);
        if(button == QMessageBox::Discard)
        {
            recorderFile.remove();
        }
        else if(button == QMessageBox::Cancel)
        {
            //重命名前确保没有含新名字的文件
            QFile backFile(BACKUP_RECORDER_FILE_PATH);
            if(backFile.exists())
                backFile.remove();
            if(backFile.exists())
            {
                QMessageBox::information(this, tr("提示"),
                                        tr("旧备份记录数据文件删除失败，请自行处理：") + BACKUP_RECORDER_FILE_PATH);
                return;
            }
            //重命名
            recorderFile.rename(BACKUP_RECORDER_FILE_PATH);
            QDir appDir(QCoreApplication::applicationDirPath());
            QMessageBox::information(this, tr("提示"),
                                     tr("记录数据文件已另存到程序所在目录：") + "\n" +
                                     appDir.absoluteFilePath(BACKUP_RECORDER_FILE_PATH) + "\n" +
                                     tr("请自行处理。"));
        }
        else if(button == QMessageBox::Ok)
        {
            //读文件
            parseDatFile(RECORDER_FILE_PATH, true);
        }
    }
}

void MainWindow::tee_saveDataResult(const qint32& result, const QString &path, const qint32 fileSize)
{
    QString str;
    switch (result) {
    case TextExtractEngine::SAVE_OK:
        str = "\nTotal saved " + QString::number(fileSize) + " Bytes in " + path + "\n";
        BrowserBuff.append(str);
        hexBrowserBuff.append(toHexDisplay(str.toLocal8Bit()));
        printToTextBrowser();
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

void MainWindow::tee_textGroupsUpdate(const QString &name, const QByteArray &data)
{
//    qDebug()<<"tee_textGroupsUpdate";
    QPlainTextEdit *textEdit = nullptr;
    qint32 count = 0;

    //find tab name
    count = ui->tabWidget->count();
    for(qint32 i = 0; i < count; i++){
        if(ui->tabWidget->tabText(i) == name){
            textEdit = dynamic_cast<QPlainTextEdit *>(ui->tabWidget->widget(i));
        }
    }

    //insert data to plainText
    if(textEdit){
        textEdit->appendPlainText(data);
    }else{
        //新增textEdit，并设置字体、背景、高亮器等属性
        textEdit = new QPlainTextEdit(this);
        textEdit->setFont(g_font);

        qint32 r,g,b;
        g_background_color.getRgb(&r,&g,&b);
        QString str = "QPlainTextEdit{ background-color: rgb(RGBR,RGBG,RGBB);}";
        str.replace("RGBR", QString::number(r));
        str.replace("RGBG", QString::number(g));
        str.replace("RGBB", QString::number(b));
        textEdit->setStyleSheet(str);

        new Highlighter(textEdit->document());

        textEdit->setReadOnly(true);

        ui->tabWidget->addTab(textEdit, name);
        textEdit->appendPlainText(data);
    }
}

void MainWindow::printToTextBrowserTimerSlot()
{
    //characterCount=0时或者窗口大小改变时重算窗口并重新显示
    if(characterCount == 0 || windowSize != ui->textBrowser->size())
    {
        printToTextBrowser();
        return;
    }
    if(RefreshTextBrowser == false)
        return;

    //打印数据
    printToTextBrowser();

    if(RefreshTextBrowser)
        RefreshTextBrowser = false;
}


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

    QString hou = QString::number(hour,10);
    QString min = QString::number(minute,10);
    QString sec = QString::number(second,10);
    QString msec = QString::number(milliSecond,10);

    //qDebug() << "minute:" << min << "second" << sec << "ms" << msec <<endl;

    return hou + ":" + min + ":" + sec ;
}

void MainWindow::secTimerSlot()
{
    static int64_t secCnt = 0;
    static qint32 msgIndex = 0;
    double idealSpeed = 0;
    qint32 txLoad, rxLoad;

    //传输速度统计与显示
    rxSpeedKB = static_cast<double>(statisticRxByteCnt) / 1024.0;
    statisticRxByteCnt = 0;
    txSpeedKB = static_cast<double>(statisticTxByteCnt) / 1024.0;
    statisticTxByteCnt = 0;
    //负载率计算(公式中的1是起始位)
    idealSpeed = (double)serial.baudRate()/(serial.stopBits()+serial.parity()+serial.dataBits()+1)/1024.0;
    txLoad = 100 * txSpeedKB / idealSpeed;
    rxLoad = 100 * rxSpeedKB / idealSpeed;
    if(txLoad>100)txLoad = 100;//由于电脑串口是先放进缓冲再发送，因此会出现使用率大于100%的情况
    if(rxLoad>100)rxLoad = 100;

    //收发速度显示与颜色控制
    QString txSpeedStr;
    QString rxSpeedStr;
    #define HIGH_LOAD_WARNING    90
    if(txSpeedKB==0)
    {
        txSpeedStr = " T:" + QString::number(txSpeedKB) + "KB/s(" + QString::number(txLoad) + "%)";
    }
    else if(txSpeedKB < 1000)
    {
        txSpeedStr = " T:" + QString::number(txSpeedKB, 'f', 2) + "KB/s(" + QString::number(txLoad) + "%)";
    }else
    {
        txSpeedStr = " T:" + QString::number(txSpeedKB, 'g', 2) + "KB/s(" + QString::number(txLoad) + "%)";
    }
    if(txLoad > HIGH_LOAD_WARNING)
    {
        txSpeedStr = "<font color=#FF5A5A>" + txSpeedStr + "</font>";
    }
    if(rxSpeedKB==0)
    {
        rxSpeedStr = " R:" + QString::number(rxSpeedKB) + "KB/s(" + QString::number(rxLoad) + "%)";
    }
    else if(rxSpeedKB < 1000)
    {
        rxSpeedStr = " R:" + QString::number(rxSpeedKB, 'f', 2) + "KB/s(" + QString::number(rxLoad) + "%)";
    }else
    {
        rxSpeedStr = " R:" + QString::number(rxSpeedKB, 'g', 2) + "KB/s(" + QString::number(rxLoad) + "%)";
    }
    if(rxLoad > HIGH_LOAD_WARNING)
    {
        rxSpeedStr = "<font color=#FF5A5A>" + rxSpeedStr + "</font>";
    }
    statusSpeedLabel->setText(txSpeedStr + rxSpeedStr);

    //显示远端下载的信息
    if(http->getMsgList().size() > 0 && secCnt % 10 == 0){
        statusRemoteMsgLabel->setText(http->getMsgList().at(msgIndex++));
        if(msgIndex == http->getMsgList().size())
            msgIndex = 0;
    }

    if(ui->comSwitch->isChecked()){
        qint64 consumedTime = QDateTime::currentSecsSinceEpoch() - g_lastSecsSinceEpoch;
        statusTimer->setText("Timer:" + formatTime(consumedTime * 1000));
    }else{
        g_lastSecsSinceEpoch = QDateTime::currentSecsSinceEpoch();
    }

    secCnt++;
    currentRunTime++;
}

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
        if(serial.isOpen()){
            serial.write(tmp.toLocal8Bit());
        }
    }else if(ui->actionFloat->isChecked()){
        QByteArray tmp;
        tmp.append(BYTE0(num1));tmp.append(BYTE1(num1));tmp.append(BYTE2(num1));tmp.append(BYTE3(num1));
        tmp.append(BYTE0(num2));tmp.append(BYTE1(num2));tmp.append(BYTE2(num2));tmp.append(BYTE3(num2));
        tmp.append(BYTE0(num3));tmp.append(BYTE1(num3));tmp.append(BYTE2(num3));tmp.append(BYTE3(num3));
        tmp.append(BYTE0(num4));tmp.append(BYTE1(num4));tmp.append(BYTE2(num4));tmp.append(BYTE3(num4));
        tmp.append(static_cast<char>(0x00));tmp.append(static_cast<char>(0x00));tmp.append(static_cast<char>(0x80));tmp.append(static_cast<char>(0x7F));
        if(serial.isOpen()){
            serial.write(tmp);
        }
    }

    debugTimerSlotCnt = debugTimerSlotCnt + 1;
}

MainWindow::~MainWindow()
{
    if(needSaveConfig){
        Config::setFirstRun(false);
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
        Config::setPopupHotKey(g_popupHotKeySequence);
        Config::setSendComment(ui->actionSendComment->isChecked());
        Config::setTeeSupport(textExtractEnable);
        Config::setTeeLevel2NameSupport(p_textExtract->getLevel2NameSupport());

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

        Config::setPlotterGraphNames(ui->customPlot->plotControl->getNameSetsFromPlot());
        if(g_xAxisSource == XAxis_Cnt)  //暂时不支持存储XY图模式的X轴名字
            Config::setXAxisName(ui->customPlot->xAxis->label());
        Config::setYAxisName(ui->customPlot->yAxis->label());
        Config::setValueDisplayState(ui->actionValueDisplay->isChecked());
        Config::setOpengGLState(ui->actionOpenGL->isChecked());
        Config::setRefreshYAxisState(ui->actionAutoRefreshYAxis->isChecked());
        if(ui->actionLinePlot->isChecked())
        {
            Config::setLineType(LineType_e::Line);
        }else if(ui->actionScatterLinePlot->isChecked())
        {
            Config::setLineType(LineType_e::Scatter_Line);
        }else if(ui->actionScatterPlot->isChecked())
        {
            Config::setLineType(LineType_e::Scatter);
        }else
        {
            Config::setLineType(LineType_e::Line);
        }

        //static
        Config::setLastRunTime(currentRunTime);
        Config::setTotalRunTime(currentRunTime);
        Config::setLastTxCnt(serial.getTotalTxCnt());//getTotalTxCnt是本次软件运行时的总发送量
        Config::setTotalTxCnt(serial.getTotalTxCnt());
        Config::setLastRxCnt(serial.getTotalRxCnt());
        Config::setTotalRxCnt(serial.getTotalRxCnt());
        Config::setTotalRunCnt(1);
    }else{
        Config::writeDefault();
    }

    p_textExtractThread->quit();
    p_textExtractThread->wait();
//    delete p_textExtract; //deleteLater自动删除？
    delete p_textExtractThread;

    delete fft_window;
    fft_window = nullptr;

    delete highlighter;
    delete ui;
    delete http;
    delete g_popupHotkey;

    QFile recorderFile(RECORDER_FILE_PATH);
    if(recorderFile.exists())
    {
        recorderFile.remove();
    }
    qDebug()<<"~MainWindow";
}

/*
 * Function:刷新串口按下。不知道为什么打开串口后再调用该函数就崩溃
*/
void MainWindow::on_refreshCom_clicked()
{   
    if(ui->refreshCom->isEnabled()==false){
        ui->statusBar->showMessage(tr("刷新功能被禁用"), 2000);
        return;
    }

    //测试更新下拉列表
    mySerialPort *testSerial = new mySerialPort;
    QList<QString> tmp;

    tmp = testSerial->refreshSerialPort();
    //刷新串口状态，需要记录当前选择的条目用于刷新后恢复
    QString portName = ui->comList->currentText().mid(0,ui->comList->currentText().indexOf('('));
    ui->comList->clear();
    foreach(const QString &info, tmp)
    {
        ui->comList->addItem(info);
    }
    if(ui->comList->count() == 0)
        ui->comList->addItem(tr("未找到可用串口!"));

    //恢复刷新前的选择
    ui->comList->setCurrentIndex(0);
    for(qint32 i = 0; i < ui->comList->count(); i++){
        if(ui->comList->itemText(i).startsWith(portName)){
            ui->comList->setCurrentIndex(i);
            break;
        }
    }

    delete testSerial;
}

/*
 * Function:在只有一个串口设备时且未被占用时尝试打开
*/
void MainWindow::tryOpenSerial()
{
    //只存在一个串口时且串口未被占用时自动打开
    if(ui->comList->count()==1 && ui->comList->currentText().indexOf(tr("BUSY"))==-1 && ui->comList->currentText()!=tr("未找到可用串口!")){
        ui->refreshCom->setChecked(false);
        ui->comSwitch->setChecked(true);
        on_comSwitch_clicked(true);
    }else{
        //如果有多个串口，则尝试选择上次使用的端口号
        if(ui->comList->count()>1){
            QString name = Config::getPortName();
            for(qint32 i = 0; i < ui->comList->count(); i++){
                if(ui->comList->itemText(i).startsWith(name)){
                    ui->comList->setCurrentIndex(i);
                    break;
                }
            }
        }
    }
}

//串口开关
void MainWindow::on_comSwitch_clicked(bool checked)
{
    QString com = ui->comList->currentText().mid(0,ui->comList->currentText().indexOf('('));
    qint32 baud = ui->baudrateList->currentText().toInt();

    if(checked)
    {
        if(serial.open(com,baud)){
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
        //清空文件缓冲
        SendFileBuff.clear();
        SendFileBuffIndex = 0;
        //关闭定时器
        if(cycleSendTimer.isActive()){
            cycleSendTimer.stop();
            ui->cycleSendCheck->setChecked(false);
        }

        serial.close();
        ui->comSwitch->setText(tr("打开串口"));
        ui->comSwitch->setChecked(false);
    }

    on_refreshCom_clicked();
}

void MainWindow::recordDataToFile(QByteArray &buff)
{
    QFile file(RECORDER_FILE_PATH);
    QTextStream stream(&file);
    if(file.open(QFile::WriteOnly|QFile::Append)){
        stream<<buff;
        file.close();
    }
}

/*
 * Function:从串口读取数据
*/
void MainWindow::readSerialPort()
{
    QByteArray tmpReadBuff;
    QByteArray floatParseBuff;//用于绘图协议解析的缓冲。其中float协议不处理中文

    //先获取时间，避免解析数据导致时间消耗的影响
    QString timeString;
    timeString = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    timeString = "\n["+timeString+"]Rx<- ";

    //解析文件模式
    if(parseFile){
        tmpReadBuff = parseFileBuff.at(parseFileBuffIndex++);
        RxBuff.append(tmpReadBuff);
    }
    else{
        if(serial.isOpen()){
            tmpReadBuff = serial.readAll(); //tmpReadBuff一定不为空。
            RxBuff.append(tmpReadBuff);
        }else
            return;
    }

    //空数据检查
    if(tmpReadBuff.isEmpty()){
        return;
    }

    //速度统计，不能和下面的互换，否则不准确
    statisticRxByteCnt += tmpReadBuff.size();

    //读取数据并衔接到上次未处理完的数据后面
    tmpReadBuff = unshowedRxBuff + tmpReadBuff;
    unshowedRxBuff.clear();

    //'\r'若单独结尾则可能被误切断，放到下一批数据中
    if(tmpReadBuff.endsWith('\r')){
        unshowedRxBuff.append(tmpReadBuff.at(tmpReadBuff.size()-1));
        tmpReadBuff.remove(tmpReadBuff.size()-1,1);
        if(tmpReadBuff.size()==0)
            return;
    }

    //如果不是hex显示则要考虑中文处理
    if(ui->hexDisplay->isChecked()==false){
        //只需要保证上屏的最后一个字节的高位不是1即可
        if(tmpReadBuff.back() & 0x80){
            qint32 reversePos = tmpReadBuff.size()-1;
            while(tmpReadBuff.at(reversePos)&0x80){//不超过3次循环
                reversePos--;
                if(reversePos<0)
                    break;
            }
            unshowedRxBuff = tmpReadBuff.mid(reversePos+1);
            tmpReadBuff = tmpReadBuff.mid(0,reversePos+1);
        }
        //如果unshowedRxBuff正好是相关编码长度的倍数，则可以上屏
        if((ui->actionGBK->isChecked() && unshowedRxBuff.size()%2==0) ||
           (ui->actionUTF8->isChecked() && unshowedRxBuff.size()%3==0)){
            tmpReadBuff.append(unshowedRxBuff);
            unshowedRxBuff.clear();
        }
    }

    //数据交付给文本解析引擎(追加数据和解析分开防止高频解析带来的CPU压力)
    if(textExtractEnable)
        emit tee_appendData(tmpReadBuff);

    if(ui->actionPlotterSwitch->isChecked() ||
       ui->actionValueDisplay->isChecked() ||
       ui->actionFFTShow->isChecked())
    {
        ui->customPlot->appendDataWaitToParse(tmpReadBuff);
    }

    recordDataToFile(tmpReadBuff);

    //时间戳选项
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

    //更新收发统计
    statusStatisticLabel->setText(serial.getTxRxString());

    //允许数据刷新
    RefreshTextBrowser = true;
}

void MainWindow::parseFileSlot()
{
    readSerialPort();
    qApp->processEvents();
    if(ui->actionPlotterSwitch->isChecked()){
        ui->customPlot->replot();
    }
    if(parseFileBuffIndex!=parseFileBuff.size()){
        ui->statusBar->showMessage(tr("解析进度：") +
                                   QString::number(100*(parseFileBuffIndex+1)/parseFileBuff.size()) +
                                   "% ", 1000);
        emit parseFileSignal();
    }else{
        parseFile = false;
        parseFileBuffIndex = 0;
        parseFileBuff.clear();
        openInteractiveUI();
    }
}

static qint32 PAGING_SIZE = 8192; //TextBrowser显示大小
void MainWindow::printToTextBrowser()
{
    //当前窗口显示字符调整
    if(characterCount == 0 || windowSize != ui->textBrowser->size())
    {
        windowSize = ui->textBrowser->size();
        resizeEvent(nullptr);
    }

    //触发文本提取引擎解析
    if(textExtractEnable)
        emit tee_parseData();

    //触发绘图器解析
    if(ui->actionPlotterSwitch->isChecked() ||
       ui->actionValueDisplay->isChecked() ||
       ui->actionFFTShow->isChecked())
    {
        ui->customPlot->startParse(g_enableSumCheck);
    }

    //多显示一点
    if(ui->hexDisplay->isChecked())
        PAGING_SIZE = characterCount * 1.5; //hex模式性能慢
    else
        PAGING_SIZE = characterCount * 2;

    //且满足gbk/utf8编码长度的倍数
    if(ui->actionGBK->isChecked()){
        PAGING_SIZE = PAGING_SIZE - PAGING_SIZE % 2;
    }else if(ui->actionUTF8->isChecked()){
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
            ui->textBrowser->setPlainText(BrowserBuff.mid(BrowserBuff.size()-PAGING_SIZE));
            BrowserBuffIndex = PAGING_SIZE;
        }
    }

    ui->textBrowser->verticalScrollBar()->setValue(ui->textBrowser->verticalScrollBar()->maximum());
    ui->textBrowser->moveCursor(QTextCursor::End);
}

void MainWindow::serialBytesWritten(qint64 bytes)
{
    //发送速度统计
    statisticTxByteCnt += bytes;

    if(SendFileBuff.size()>0 && SendFileBuffIndex!=SendFileBuff.size()){
        ui->statusBar->showMessage(tr("发送进度：") +
                                   QString::number(100*(SendFileBuffIndex+1)/SendFileBuff.size()) +
                                   "%",1000);
        serial.write(SendFileBuff.at(SendFileBuffIndex++));
        if(SendFileBuffIndex == SendFileBuff.size()){
            SendFileBuffIndex = 0;
            SendFileBuff.clear();
            sendFile = false;
            ui->sendButton->setEnabled(true);
            ui->multiString->setEnabled(true);
            ui->cycleSendCheck->setEnabled(true);
            ui->clearWindows->setText(tr("清  空"));
        }
    }

    //更新收发统计
    statusStatisticLabel->setText(serial.getTxRxString());
}

void MainWindow::handleSerialError(QSerialPort::SerialPortError errCode)
{
    //故障检测
    if(errCode == QSerialPort::ResourceError){
        //记录故障的串口号
        QString portName = serial.portName();
        //关闭串口
        on_comSwitch_clicked(false);
        //强提醒也争取了时间，如果是短时间松动，则点击确定后可以恢复所选的端口
        QMessageBox::warning(this,tr("警告"),tr("检测到串口故障，已关闭串口。\n串口是否发生了松动？"));
        //【还要】再刷新一次
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

/*
 * Function:连续发送定时器槽，执行数据发送
*/
void MainWindow::cycleSendTimerSlot()
{
    on_sendButton_clicked();
}

//用于多字符串组件的序列发送功能，提取序列延时时间
qint32 extractSeqenceTime(QString &str)
{
    QString temp;
    qint32 seqTime = -1;
    bool ok;
    if(str.indexOf('[')==-1)
    {
        return -1;
    }
    temp = str.mid(str.indexOf('['));

    if(str.indexOf(']')==-1)
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

void MainWindow::multiStrSeqSendTimerSlot()
{
    if (!ui->actionMultiString->isChecked())
    {
        multiStrSeqSendTimer.stop();
        return;
    }
    if(ui->multiString->item(g_multiStr_cur_index + 1)==nullptr)
    {
        g_multiStr_cur_index = -1;
    }

    //十六进制发送下的输入格式检查，遇到非法字符串时及时停止发送，否则on_textEdit_textChanged()机制会引起数据重发
    QString tmp, noCut_tmp;
    noCut_tmp = tmp = ui->multiString->item(++g_multiStr_cur_index)->text();
    if(ui->actionSendComment->isChecked())
    {
        if(tmp.lastIndexOf("//") != -1)
        {
            tmp = tmp.mid(0, tmp.lastIndexOf("//"));
        }
        else if(tmp.lastIndexOf("/") != -1 && ui->hexSend->isChecked()) //注意顺序
        {
            tmp = tmp.mid(0, tmp.lastIndexOf("/"));
        }
    }
    if(ui->hexSend->isChecked()){
        if(!hexFormatCheck(tmp)){
            QMessageBox::warning(this, tr("警告"), QString("The %1th string has illegal hexadecimal format.").arg(g_multiStr_cur_index));
            multiStrSeqSendTimer.stop();
            ui->clearWindows->setText(tr("清  空"));
            //存在非法字符就及时停止发送
            return;
        }
    }
    //实际上填入数据后还会再触发一次on_textEdit_textChanged()进行格式检查，
    //但是on_textEdit_textChanged()会重置回上一次字符串，导致会发送上一次数据，有必要先行检查
    ui->textEdit->setText(noCut_tmp);
    on_sendButton_clicked();
}

/*
 * Function:发送数据
*/
void MainWindow::on_sendButton_clicked()
{
    QByteArray tmp;
    QString tail;

    if(!serial.isOpen()){
        QMessageBox::information(this,tr("提示"),tr("串口未打开"));
        return;
    }

    //不处理注释(发送注释功能)
    tmp = ui->textEdit->toPlainText().toLocal8Bit();
    if(ui->actionSendComment->isChecked())
    {
        if(tmp.lastIndexOf("//") != -1)
        {
            tail = tmp.mid(tmp.lastIndexOf("//"));
            tmp = tmp.mid(0, tmp.lastIndexOf("//"));
        }
        else if(tmp.lastIndexOf("/") != -1 && ui->hexSend->isChecked()) //注意顺序
        {
            tail = tmp.mid(tmp.lastIndexOf("/"));
            tmp = tmp.mid(0, tmp.lastIndexOf("/"));
        }
        //多字符串序列发送
        if(ui->actionMultiString->isChecked())
        {
            qint32 seqTime = extractSeqenceTime(tail);
            if(seqTime > 0)
            {
                ui->clearWindows->setText(tr("中  止"));
                multiStrSeqSendTimer.start(seqTime);
            }
            else
            {
                ui->clearWindows->setText(tr("清  空"));
                multiStrSeqSendTimer.stop();
            }
        }
        else
        {
            multiStrSeqSendTimer.stop();
        }
    }

    //回车风格转换，win风格补上'\r'，默认unix风格

    if(ui->action_winLikeEnter->isChecked()){
        //win风格
        while (tmp.indexOf('\n') != -1) {
            tmp = tmp.replace('\n', '\t');
        }
        while (tmp.indexOf('\t') != -1) {
            tmp = tmp.replace('\t', "\r\n");
        }
    }
    else{
        //unix风格
        while (tmp.indexOf('\r') != -1) {
            tmp = tmp.remove(tmp.indexOf('\r'),1);
        }
    }

    //十六进制检查
    QByteArray sendArr; //真正发送出去的数据
    if(ui->hexSend->isChecked()){
        //以hex发送数据
        //HexStringToByteArray函数必须传入格式化后的字符串，如"02 31"
        bool ok;
        sendArr = HexStringToByteArray(tmp,ok); //hex转发送数据流
        if(ok){
            serial.write(sendArr);
        }else{
            ui->statusBar->showMessage(tr("文本输入区数据转换失败，放弃此次发送！"), 2000);
        }
    }else {
        sendArr = tmp;
        //utf8编码
        serial.write(sendArr);
    }

    //周期发送开启则立刻发送
    if(ui->cycleSendCheck->isChecked())
        serial.flush();

    //若添加了时间戳则把发送的数据也显示在接收区
    if(ui->timeStampCheckBox->isChecked()){
        QString timeString;
        timeString = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        timeString = "\n["+timeString+"]Tx-> ";

        hexBrowserBuff.append(timeString + toHexDisplay(sendArr));
        BrowserBuff.append(timeString + QString::fromLocal8Bit(sendArr));

        //打印数据
        printToTextBrowser();
    }

    //给多字符串控件添加条目
    if(ui->actionMultiString->isChecked()){
        bool hasItem=false;
        for(qint32 i = 0; i < ui->multiString->count(); i++){
            if(ui->multiString->item(i)->text()==ui->textEdit->toPlainText())
                hasItem = true;
        }
        if(!hasItem)
            ui->multiString->addItem(ui->textEdit->toPlainText());
    }

    //更新收发统计
    statusStatisticLabel->setText(serial.getTxRxString());
}

void MainWindow::on_clearWindows_clicked()
{
    ui->clearWindows->setText(tr("清  空"));

    //多字符串序列发送定时器，第二次才清空
    if(multiStrSeqSendTimer.isActive())
    {
        multiStrSeqSendTimer.stop();
        return;
    }

    //文件解析中止，第二次才清空
    if(parseFile)
    {
        parseFileBuff.clear();
        parseFileBuffIndex = 0;
        parseFile = 0;
        openInteractiveUI();
        return;
    }

    //文件发送中止
    if(sendFile)
    {
        SendFileBuffIndex = 0;
        SendFileBuff.clear();
        sendFile = false;
        openInteractiveUI();
        return;
    }

    //定时器
    g_lastSecsSinceEpoch = QDateTime::currentSecsSinceEpoch();
    qint64 consumedTime = QDateTime::currentSecsSinceEpoch() - g_lastSecsSinceEpoch;
    statusTimer->setText("Timer:" + formatTime(consumedTime * 1000));

    //串口
    serial.resetCnt();
    if(serial.isOpen())
        serial.flush();

    //接收区
    ui->textBrowser->clear();
    RxBuff.clear();
    hexBrowserBuff.clear();
    hexBrowserBuffIndex = 0;
    BrowserBuff.clear();
    BrowserBuffIndex = 0;
    unshowedRxBuff.clear();
    emit tee_clearData("");
    for(qint32 i = 0; i < ui->tabWidget->count(); i++){
        if(ui->tabWidget->tabText(i) != MAIN_TAB_NAME){
            if(ui->tabWidget->widget(i) != nullptr)
            {
                delete ui->tabWidget->widget(i);
            }
//            ui->tabWidget->removeTab(i);
            emit tee_clearData(ui->tabWidget->tabText(i));
            i = 0;//重置计数器
        }
    }

    //绘图器相关
    ui->customPlot->protocol->clearBuff();
    ui->customPlot->plotControl->clearPlotter(-1);
    while(ui->customPlot->graphCount()>1){
        ui->customPlot->removeGraph(ui->customPlot->graphCount()-1);
    }
    if(g_xAxisSource == XAxis_Cnt)
    {
        ui->customPlot->yAxis->setRange(0, 5);
        ui->customPlot->xAxis->setRange(0, ui->customPlot->plotControl->getXAxisLength(), Qt::AlignRight);
    }
    else
    {
        ui->customPlot->yAxis->rescale(true);
        ui->customPlot->xAxis->rescale(true);
    }
    ui->customPlot->replot();

    //fft
    fft_window->clearGraphs();

    //数值显示器
    deleteValueDisplaySlot();

    //实时数据记录仪
    QFile recorderFile(RECORDER_FILE_PATH);
    recorderFile.remove();

    //更新收发统计
    statusStatisticLabel->setText(serial.getTxRxString());
}

void MainWindow::on_cycleSendCheck_clicked(bool checked)
{
    if(ui->sendInterval->text().toInt() < 15 && checked){
        QMessageBox::warning(this,tr("警告"),tr("发送间隔较小可能不够准确"));
    }

    if(!serial.isOpen()){
        QMessageBox::information(this,tr("提示"),tr("串口未打开"));
        ui->cycleSendCheck->setChecked(false);
        return;
    }

    //启停定时器
    if(checked){
        ui->cycleSendCheck->setChecked(true);
        cycleSendTimer.setTimerType(Qt::PreciseTimer);
        cycleSendTimer.start(ui->sendInterval->text().toInt());
    }
    else {
        ui->cycleSendCheck->setChecked(false);
        cycleSendTimer.stop();
    }
}

/*
 * Event:发送区文本变化
 * Function:检查十六进制发送模式下的发送区文本内容是否非法
*/
void MainWindow::on_textEdit_textChanged()
{
    QString tmp;
    QString noCut_tmp;
    //不处理注释
    noCut_tmp = tmp = ui->textEdit->toPlainText();
    if(ui->actionSendComment->isChecked())
    {
        if(tmp.lastIndexOf("//") != -1)
        {
            tmp = tmp.mid(0, tmp.lastIndexOf("//"));
        }
        else if(tmp.lastIndexOf("/") != -1 && ui->hexSend->isChecked()) //注意顺序
        {
            tmp = tmp.mid(0, tmp.lastIndexOf("/"));
        }
    }

    //十六进制发送下的输入格式检查
    static QString lastText;
    if(ui->hexSend->isChecked()){
        if(!hexFormatCheck(tmp)){
            QMessageBox::warning(this, tr("警告"), tr("hex发送模式下存在非法的十六进制格式。"));
            multiStrSeqSendTimer.stop();
            ui->clearWindows->setText(tr("清  空"));
            ui->textEdit->clear();
            ui->textEdit->insertPlainText(lastText);
            return;
        }
        //不能记录非空数据，因为clear操作也会触发本事件
        if(!noCut_tmp.isEmpty())
            lastText = noCut_tmp;
    }
}

/*
 * Event:十六进制格式发送按钮状态变化
 * Function:保存当前发送区的文本内容
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

/*
 * Event:十六进制显示按钮状态改变
 * Function:将当前接收框的内容转换为十六进制格式重新显示
*/
void MainWindow::on_hexDisplay_clicked(bool checked)
{
    checked = !checked;
    printToTextBrowser();
}

/*
 * Action:激活使用win风格回车（\r\n）
 * Function:
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
void MainWindow::on_action_unixLikeEnter_triggered(bool checked)
{
    if(checked){
        ui->action_winLikeEnter->setChecked(false);
    }else {
        ui->action_unixLikeEnter->setChecked(false);
    }
}

/*
 * Action:激活使用UTF8编码
 * Function:
*/
void MainWindow::on_actionUTF8_triggered(bool checked)
{
    ui->actionUTF8->setChecked(true);
    if(checked){
        //设置中文编码
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        QTextCodec::setCodecForLocale(codec);
        ui->actionGBK->setChecked(false);
    }
}

void MainWindow::on_actionGBK_triggered(bool checked)
{
    ui->actionGBK->setChecked(true);
    if(checked){
        //设置中文编码
        QTextCodec *codec = QTextCodec::codecForName("GBK");
        QTextCodec::setCodecForLocale(codec);
        ui->actionUTF8->setChecked(false);
    }
}

/*
 * Action:保存数据动作触发
 * Function:
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
                                                    lastFileDialogPath + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss") + "-[" + tabName + "]" +".dat",
                                                    "Dat File(*.dat);;All File(*.*)");
    //检查路径格式
    if(!savePath.endsWith(".dat")){
        if(!savePath.isEmpty())
            QMessageBox::information(this,tr("提示"),"尚未支持的文件格式，请选择dat文件。");
        return;
    }

    //子窗口数据由其线程负责存储
    if(tabName != MAIN_TAB_NAME){
        emit tee_saveData(savePath, tabName, true);
        return;
    }

    //保存数据
    QFile file(savePath);
    //删除旧数据形式写文件
    if(file.open(QFile::WriteOnly|QFile::Truncate)){
        file.write(RxBuff);//不用DataStream写入非文本文件，它会额外添加4个字节作为文件头
        file.flush();
        file.close();

        QString str = "\nTotal saved " + QString::number(file.size()) + " Bytes in " + savePath + "\n";
        BrowserBuff.append(str);
        hexBrowserBuff.append(toHexDisplay(str.toLocal8Bit()));
        printToTextBrowser();
    }else{
        QMessageBox::information(this,tr("提示"),tr("文件打开失败。"));
    }

    //记忆路径
    lastFileDialogPath = savePath;
    lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/')+1);
}

/*
 * Action:读取数据动作触发
 * Function:
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

    if (parseDatFile(readPath, false))
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

/*
 * Action:触发“关于”按钮
 * Function:弹出关于对话框
*/
void MainWindow::on_actionAbout_triggered()
{
    //创建关于我对话框资源
    About_Me_Dialog* p = new About_Me_Dialog(this);
    p->getVersionString(Config::getVersion());
    //设置close后自动销毁
    p->setAttribute(Qt::WA_DeleteOnClose);
    //非阻塞式显示
    p->show();
}

void MainWindow::on_actionCOM_Config_triggered()
{
    //创建串口设置对话框
    settings_dialog* p = new settings_dialog(this);
    //对话框读取原配置
    p->setStopBits(serial.stopBits());
    p->setDataBits(serial.dataBits());
    p->setParity(serial.parity());
    p->setFlowControl(serial.flowControl());
    p->exec();
    //对话框返回新配置并设置
    if(p->clickedOK()){
        if(!serial.moreSetting(p->getStopBits(),p->getParity(),p->getFlowControl(),p->getDataBits()))
            QMessageBox::information(this,tr("提示"),tr("串口设置失败，请关闭串口重试"));
    }

    delete p;
}

/*
 * Function:波特率框文本变化，检查输入合法性并重置波特率
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

/*
 * Function:选择了新的端口号，重新打开串口
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
                lastFileDialogPath + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss") + "-[" + tabName + "]" +".txt",
                "Text File(*.txt);;All File(*.*)");

    //检查路径
    if(!savePath.endsWith("txt")){
        if(!savePath.isEmpty())
            QMessageBox::information(this,tr("尚未支持的文件格式"),tr("请选择txt文本文件。"));
        return;
    }

    //标签页的数据保存由其线程自己负责
    if(tabName != MAIN_TAB_NAME){
        emit tee_saveData(savePath, tabName, false);
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
            stream<<hexBrowserBuff;
        }else{
            stream<<BrowserBuff;
        }
        file.close();

        QString str = "\nTotal saved " + QString::number(file.size()) + " Bytes in " + savePath + "\n";
        BrowserBuff.append(str);
        hexBrowserBuff.append(toHexDisplay(str.toLocal8Bit()));
        printToTextBrowser();
    }else{
        QMessageBox::information(this,tr("提示"),tr("文件打开失败。"));
    }
}

void MainWindow::on_actionUpdate_triggered()
{
    http->addTask(HTTP::GetVersion);
}

void MainWindow::on_sendInterval_textChanged(const QString &arg1)
{
    if(cycleSendTimer.isActive())
        cycleSendTimer.setInterval(arg1.toInt());
}

void MainWindow::on_actionSTM32_ISP_triggered()
{
    bool serialState = ui->comSwitch->isChecked();
    if(serialState)
        on_comSwitch_clicked(false);

    QFile file;
    file.copy(":/stm32isp.exe","stm32isp.exe");
    file.setFileName("stm32isp.exe");
    file.setPermissions(QFile::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner);

    STM32ISP_Dialog *p = new STM32ISP_Dialog(this);
    p->exec();
    delete p;

    file.remove();

    if(serialState)
        on_comSwitch_clicked(true);
}

/*
 * Event: 多字符串条目双击
*/
void MainWindow::on_multiString_itemDoubleClicked(QListWidgetItem *item)
{
    //十六进制发送下的输入格式检查（先剔除注释数据）
    QString tmp = item->text();
    if(ui->actionSendComment->isChecked())
    {
        if(tmp.lastIndexOf("//") != -1)
        {
            tmp = tmp.mid(0, tmp.lastIndexOf("//"));
        }
        else if(tmp.lastIndexOf("/") != -1 && ui->hexSend->isChecked()) //注意顺序
        {
            tmp = tmp.mid(0, tmp.lastIndexOf("/"));
        }
    }
    if(ui->hexSend->isChecked()){
        if(!hexFormatCheck(tmp)){
            QMessageBox::warning(this, tr("警告"), tr("hex发送模式下存在非法的十六进制格式。"));
            multiStrSeqSendTimer.stop();
            ui->clearWindows->setText(tr("清  空"));
            return;
        }
    }
    //实际上填入数据后还会再触发一次on_textEdit_textChanged()进行格式检查，
    //但是on_textEdit_textChanged()会重置回上一次字符串，导致会发送上一次数据，所以先进行检查
    g_multiStr_cur_index = ui->multiString->currentIndex().row();
    ui->textEdit->setText(item->text());
    on_sendButton_clicked();
}

/*
 * Action:multiString开关
*/
void MainWindow::on_actionMultiString_triggered(bool checked)
{
    if(checked){
        ui->multiString->show();
        //设置颜色交错
        ui->multiString->setAlternatingRowColors(true);
    }else {
        ui->multiString->hide();
    }
    adjustLayout();
}

/*
 * Function:多字符串右键菜单
*/
void MainWindow::on_multiString_customContextMenuRequested(const QPoint &pos)
{
    QListWidgetItem* curItem = ui->multiString->itemAt( pos );
    QAction *editSeed = nullptr;
    QAction *clearSeeds = nullptr;
    QAction *deleteSeed = nullptr;
    QMenu *popMenu = new QMenu( this );
    //添加右键菜单
    if( curItem != nullptr ){
        editSeed = new QAction(tr("编辑当前条目"), this);
        popMenu->addAction( editSeed );
        connect( editSeed, SIGNAL(triggered() ), this, SLOT( editSeedSlot()) );

        popMenu->addSeparator();

        deleteSeed = new QAction(tr("删除当前条目"), this);
        popMenu->addAction( deleteSeed );
        connect( deleteSeed, SIGNAL(triggered() ), this, SLOT( deleteSeedSlot()) );

        popMenu->addSeparator();
    }
    clearSeeds = new QAction(tr("清空所有条目"), this);
    popMenu->addAction( clearSeeds );
    connect( clearSeeds, SIGNAL(triggered() ), this, SLOT( clearSeedsSlot()) );
    popMenu->exec( QCursor::pos() );
    delete popMenu;
    delete clearSeeds;
    delete deleteSeed;
}

/*
 * Function:编辑multiString条目
*/
void MainWindow::editSeedSlot()
{
    QListWidgetItem * item = ui->multiString->currentItem();
    if( item == nullptr )
        return;

    qint32 curIndex = ui->multiString->row(item);
    bool ok = false;
    QString newStr = QInputDialog::getMultiLineText(this, tr("编辑条目"), tr("新的文本："),
                                                    ui->multiString->item(curIndex)->text(),
                                                    &ok, Qt::WindowCloseButtonHint);
    if(ok == true)
        ui->multiString->item(curIndex)->setText(newStr);
}

/*
 * Function:删除multiString条目
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

/*
 * Function:清除multiString条目
*/
void MainWindow::clearSeedsSlot()
{
    QListWidgetItem * item = ui->multiString->currentItem();
    if( item == nullptr )
        return;

    ui->multiString->clear();
}

//更新数据可视化按钮的标题
void MainWindow::setVisualizerTitle(void)
{
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
}

void MainWindow::resetVisualizerTitle(void)
{
    if(ui->customPlot->isVisible() ||
       ui->valueDisplay->isVisible() ||
       (fft_window && fft_window->isVisible()))
    {
        return;
    }

    ui->visualizer->setTitle(tr("数据可视化"));
}

/*
 * Function:绘图器开关
*/
void MainWindow::on_actionPlotterSwitch_triggered(bool checked)
{   
    if(checked){
        ui->customPlot->show();
        setVisualizerTitle();
    }else{
        ui->customPlot->hide();
        resetVisualizerTitle();
    }

    adjustLayout();
}

void MainWindow::plotterShowTimerSlot()
{
    QVector<double> oneRowData;

    if(!ui->actionPlotterSwitch->isChecked() &&
       !ui->actionValueDisplay->isChecked() &&
        (fft_window && !fft_window->isVisible())){
        return;
    }

    if(ui->customPlot->protocol->parsedBuffSize() == 0)
    {
        return;
    }

    //数据填充
    while(ui->customPlot->protocol->parsedBuffSize()>0){
        oneRowData = ui->customPlot->protocol->popOneRowData();
        if(fft_window->isVisible())
        {
            fft_window->appendData(oneRowData);
        }
        //绘图显示器
        if(ui->actionPlotterSwitch->isChecked()){
            //关闭刷新，数据全部填充完后统一刷新
            if(false == ui->customPlot->plotControl->addDataToPlotter(oneRowData, g_xAxisSource))
                ui->statusBar->showMessage(tr("出现一组异常绘图数据，已丢弃。"), 2000);
        }
    }
    //曲线刷新
	if(ui->actionPlotterSwitch->isChecked())
	{
        if(ui->actionAutoRefreshYAxis->isChecked())
        {
            ui->customPlot->yAxis->rescale(true);
        }
        ui->customPlot->replot();   //<10ms
    }
    if(fft_window->isVisible())
    {
        fft_window->startFFTCal();
    }

    //数值显示器
    if(ui->actionValueDisplay->isChecked()){
        //判断是否添加行
        if(ui->valueDisplay->rowCount() < oneRowData.size()){
            //设置行
            ui->valueDisplay->setRowCount(oneRowData.size());
            //设置列，固定的
            ui->valueDisplay->setColumnCount(2);
            ui->valueDisplay->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("名称")));
            ui->valueDisplay->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("值")));
            ui->valueDisplay->horizontalHeader()->setStretchLastSection(true);
            ui->valueDisplay->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
            ui->valueDisplay->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        }
        //添加数据
        qint32 min = oneRowData.size() < ui->customPlot->plotControl->getNameSetsFromPlot().size() ? oneRowData.size() : ui->customPlot->plotControl->getNameSetsFromPlot().size();
        for(qint32 i=0; i < min; i++){
            //这里会重复new对象导致内存溢出吗
            ui->valueDisplay->setItem(i, 0, new QTableWidgetItem(ui->customPlot->plotControl->getNameSetsFromPlot().at(i)));
            ui->valueDisplay->setItem(i, 1, new QTableWidgetItem(QString::number(oneRowData.at(i),'g')));
            //不可编辑
            ui->valueDisplay->item(i,0)->setFlags(ui->valueDisplay->item(i,0)->flags() & (~Qt::ItemIsEditable));
            ui->valueDisplay->item(i,1)->setFlags(ui->valueDisplay->item(i,1)->flags() & (~Qt::ItemIsEditable));
        }
    }
}

void MainWindow::on_actionAscii_triggered(bool checked)
{
    Q_UNUSED(checked)

    ui->customPlot->protocol->clearBuff();
    ui->customPlot->protocol->setProtocolType(DataProtocol::Ascii);
    ui->actionAscii->setChecked(true);
    ui->actionFloat->setChecked(false);

    setVisualizerTitle();
}

void MainWindow::on_actionFloat_triggered(bool checked)
{
    Q_UNUSED(checked)

    ui->customPlot->protocol->clearBuff();
    ui->customPlot->protocol->setProtocolType(DataProtocol::Float);
    ui->actionAscii->setChecked(false);
    ui->actionFloat->setChecked(true);

    setVisualizerTitle();
}

void MainWindow::on_actiondebug_triggered(bool checked)
{
    if(checked){
        debugTimerSlotCnt = 0;
        debugTimer.setTimerType(Qt::PreciseTimer);
        debugTimer.start(1000/DEBUG_TIMER_FRQ);
        connect(&debugTimer, SIGNAL(timeout()), this, SLOT(debugTimerSlot()));
    }else{
        debugTimer.stop();
        disconnect(&debugTimer, SIGNAL(timeout()), this, SLOT(debugTimerSlot()));
    }
}

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

        //是否显示完了
        if(ui->hexDisplay->isChecked()){
            res = hexBrowserBuffIndex != hexBrowserBuff.size();
        }else{
            res = BrowserBuffIndex != BrowserBuff.size();
        }
        //翻到顶部了，加载更多内容
        if(value == 0 && res){

            //直接显示全部
//            BrowserBuffIndex = BrowserBuff.size();
//            hexBrowserBuffIndex = hexBrowserBuff.size();
            //显示内容指数型增加
            if(BrowserBuffIndex*2 < BrowserBuff.size()){
                BrowserBuffIndex = BrowserBuffIndex*2;
            }
            else{
                BrowserBuffIndex = BrowserBuff.size();
            }
            if(hexBrowserBuffIndex*2 < hexBrowserBuff.size()){
                hexBrowserBuffIndex = hexBrowserBuffIndex*2;
            }
            else{
                hexBrowserBuffIndex = hexBrowserBuff.size();
            }

            if(ui->hexDisplay->isChecked()){
                ui->textBrowser->setPlainText(hexBrowserBuff.mid(hexBrowserBuff.size() - hexBrowserBuffIndex));
            }else{
                ui->textBrowser->setPlainText(BrowserBuff.mid(BrowserBuff.size() - BrowserBuffIndex));
            }

            //保持bar位置不动
            newValue = bar->maximum()-oldMax;
            bar->setValue(newValue);
        }
    }

}


void MainWindow::on_actionLinePlot_triggered()
{
    if(ui->customPlot->selectedGraphs().size() == 0)
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
    ui->customPlot->plotControl->setupLineType(QCustomPlotControl::Line);
}

void MainWindow::on_actionScatterLinePlot_triggered()
{
    if(ui->customPlot->selectedGraphs().size() == 0)
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
    ui->customPlot->plotControl->setupLineType(QCustomPlotControl::ScatterLine);
}

void MainWindow::on_actionScatterPlot_triggered()
{
    if(ui->customPlot->selectedGraphs().size() == 0)
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
    ui->customPlot->plotControl->setupLineType(QCustomPlotControl::Scatter);
}

void MainWindow::on_actionResetDefaultConfig_triggered(bool checked)
{
    if(checked)
    {
        QMessageBox::Button button = QMessageBox::warning(this,tr("警告：确认恢复默认设置吗？"),tr("该操作会重置软件初始状态！"),QMessageBox::Ok|QMessageBox::No);
        if(button == QMessageBox::No)
            return;
        needSaveConfig = false;
        QMessageBox::information(this, tr("提示"), tr("重置成功。请重启程序。"));
    }else{
        needSaveConfig = true;
    }
}

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

void MainWindow::on_actionSavePlotData_triggered()
{
    //打开保存文件对话框
    QString savePath = QFileDialog::getSaveFileName(this,
                                                    tr("保存绘图数据-选择文件路径"),
                                                    lastFileDialogPath + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")+".xlsx",
                                                    "XLSX File(*.xlsx);;CSV File(*.csv);;TXT File(*.txt);;All File(*.*)");
    //检查路径格式
    if(!savePath.endsWith(".xlsx") &&
       !savePath.endsWith(".csv") &&
       !savePath.endsWith(".txt")){
        if(!savePath.isEmpty())
            QMessageBox::information(this,tr("提示"),tr("尚未支持的文件格式。请选择xlsx或者csv或者txt格式文件。"));
        return;
    }

    //记录路径
    lastFileDialogPath = savePath;
    lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/')+1);

    bool ok = false;
    if(savePath.endsWith(".xlsx")){
        if(MyXlsx::write(ui->customPlot, savePath))
            ok = true;
    }else if(savePath.endsWith(".csv")){
        if(ui->customPlot->saveGraphAsTxt(savePath,','))
            ok = true;
    }else if(savePath.endsWith(".txt")){
        if(ui->customPlot->saveGraphAsTxt(savePath,' '))
            ok = true;
    }

    if(ok){
        QString str = "\nSave successful in " + savePath + "\n";
        BrowserBuff.append(str);
        hexBrowserBuff.append(toHexDisplay(str.toLocal8Bit()));
        printToTextBrowser();
    }else{
        QMessageBox::warning(this, tr("警告"), tr("保存失败。文件是否被其他软件占用？"));
    }

}

void MainWindow::on_actionSavePlotAsPicture_triggered()
{
    //打开保存文件对话框
    QString savePath = QFileDialog::getSaveFileName(this,
                                                    tr("曲线保存图片-选择文件路径"),
                                                    lastFileDialogPath + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"),
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
    //记录上次路径
    lastFileDialogPath = savePath;
    lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/')+1);

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

    if(ok){
        QString str = "\nSave successful in " + savePath + "\n";
        BrowserBuff.append(str);
        hexBrowserBuff.append(toHexDisplay(str.toLocal8Bit()));
        printToTextBrowser();
    }else{
        QMessageBox::warning(this, tr("警告"), tr("保存失败。文件是否被其他软件占用？"));
    }
}

void MainWindow::on_actionKeyWordHighlight_triggered(bool checked)
{
    if(checked){
        if(highlighter==nullptr)
            highlighter = new Highlighter(ui->textBrowser->document());
    }else{
        delete highlighter;
        highlighter = nullptr;
    }
}
/*
 * Funciont:显示使用统计
*/
void MainWindow::on_actionUsageStatistic_triggered()
{
    static uint32_t lastTotalTx = Config::getTotalTxCnt().toUInt();
    static uint32_t lastTotalRx = Config::getTotalRxCnt().toUInt();
    static uint32_t lastTotalRunTime = Config::getTotalRunTime().toUInt();
    double currentTx = serial.getTxCnt();
    double currentRx = serial.getRxCnt();
    double currentRunTime_f = currentRunTime;
    double totalTx = lastTotalTx + currentTx;
    double totalRx = lastTotalRx + currentRx;
    double totalRunTime = lastTotalRunTime + currentRunTime_f;
    double totalTxRx_MB = totalTx/1024/1024 + totalRx/1024/1024;
    //单位
    QString totalTxUnit;
    QString totalRxUnit;
    QString currentTxUnit;
    QString currentRxUnit;
    //时间换算
    QString days;
    QString hou;
    QString min;
    QString sec;
    QString currentRunTimeStr, totalRunTimeStr;
    long day;
    long hour;
    long minute;
    long second;
    qint32 nest = 0; //执行统计

    //单位换算
    nest = 0;
    while(totalTx>1024){
        totalTx = totalTx/1024;
        nest++;
    }
    switch(nest){
        case 0:totalTxUnit = " B";break;
        case 1:totalTxUnit = " KB";break;
        case 2:totalTxUnit = " MB";break;
        case 3:totalTxUnit = " GB";break;
        case 4:totalTxUnit = " TB";break;
    }
    //单位换算
    nest = 0;
    while(totalRx>1024){
        totalRx = totalRx/1024;
        nest++;
    }
    switch(nest){
        case 0:totalRxUnit = " B";break;
        case 1:totalRxUnit = " KB";break;
        case 2:totalRxUnit = " MB";break;
        case 3:totalRxUnit = " GB";break;
        case 4:totalRxUnit = " TB";break;
    }
    //单位换算
    nest = 0;
    while(currentTx>1024){
        currentTx = currentTx/1024;
        nest++;
    }
    switch(nest){
        case 0:currentTxUnit = " B";break;
        case 1:currentTxUnit = " KB";break;
        case 2:currentTxUnit = " MB";break;
        case 3:currentTxUnit = " GB";break;
        case 4:currentTxUnit = " TB";break;
    }
    //单位换算
    nest = 0;
    while(currentRx>1024){
        currentRx = currentRx/1024;
        nest++;
    }
    switch(nest){
        case 0:currentRxUnit = " B";break;
        case 1:currentRxUnit = " KB";break;
        case 2:currentRxUnit = " MB";break;
        case 3:currentRxUnit = " GB";break;
        case 4:currentRxUnit = " TB";break;
    }
    //时间常数
    qint32 mi = 60;
    qint32 hh = mi * 60;
    qint32 dd = hh * 24;
    //时间换算
    day = static_cast<long>(currentRunTime_f / dd);
    hour = static_cast<long>((currentRunTime_f - day * dd) / hh);
    minute = static_cast<long>((currentRunTime_f - day * dd - hour * hh) / mi);
    second = static_cast<long>((currentRunTime_f - day * dd - hour * hh - minute * mi));

    days = QString::number(day,10);
    hou = QString::number(hour,10);
    min = QString::number(minute,10);
    sec = QString::number(second,10);
    currentRunTimeStr = days + tr(" 天 ") + hou + tr(" 小时 ") + min + tr(" 分钟 ") + sec + tr(" 秒");
    //时间换算
    day = static_cast<long>(totalRunTime / dd);
    hour = static_cast<long>((totalRunTime - day * dd) / hh);
    minute = static_cast<long>((totalRunTime - day * dd - hour * hh) / mi);
    second = static_cast<long>((totalRunTime - day * dd - hour * hh - minute * mi));

    days = QString::number(day,10);
    hou = QString::number(hour,10);
    min = QString::number(minute,10);
    sec = QString::number(second,10);
    totalRunTimeStr = days + tr(" 天 ") + hou + tr(" 小时 ") + min + tr(" 分钟 ") + sec + tr(" 秒");

    QString rankStr;
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

    //上屏显示
    //ui->textBrowser->clear(); //如果清屏的话要做提示，可能用户数据还未保存
    QString str;
    str.append(tr("## 软件版本：")+Config::getVersion() + "\n");
    str.append("\n");
    str.append(tr("## 设备信息") + "\n");
    str.append(tr("   MAC地址：")+HTTP::getHostMacAddress() + "\n");
    str.append("\n");
    str.append(tr("## 软件使用统计") + "\n");
    str.append(tr("   ### 自本次启动软件以来，阁下：") + "\n");
    str.append(tr("   - 共发送数据：")+QString::number(currentTx,'f',2)+currentTxUnit + "\n");
    str.append(tr("   - 共接收数据：")+QString::number(currentRx,'f',2)+currentRxUnit + "\n");
    str.append(tr("   - 共运行本软件：")+currentRunTimeStr + "\n");
    str.append("\n");
    str.append(tr("   ### 自首次启动软件以来，阁下：") + "\n");
    str.append(tr("   - 共发送数据：")+QString::number(totalTx,'f',2)+totalTxUnit + "\n");
    str.append(tr("   - 共接收数据：")+QString::number(totalRx,'f',2)+totalRxUnit + "\n");
    str.append(tr("   - 共运行本软件：")+totalRunTimeStr + "\n");
    str.append(tr("   - 共启动本软件：")+QString::number(Config::getTotalRunCnt().toInt()+1)+tr(" 次") + "\n");
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

void MainWindow::on_actionSendFile_triggered()
{
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
    lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/')+1);
    //读取文件
    QFile file(readPath);

    //读文件
    if(file.open(QFile::ReadOnly)){
        //记录上一次文件名
        lastFileName = readPath;
        while(lastFileName.indexOf('/')!=-1){
            lastFileName = lastFileName.mid(lastFileName.indexOf('/')+1);
        }

        TxBuff.clear();
        TxBuff = file.readAll();
        file.close();

        //文件分包
        #define PACKSIZE_SENDFILE 256
        SendFileBuffIndex = 0;
        SendFileBuff.clear();
        sendFile = true;
        if(divideDataToPacks(TxBuff, SendFileBuff, PACKSIZE_SENDFILE, sendFile))
            return ;

        if(serial.isOpen()){
            ui->textBrowser->clear();
            ui->textBrowser->appendPlainText("File size: "+QString::number(file.size())+" Bytes");
            ui->textBrowser->appendPlainText("One pack size: "+QString::number(PACKSIZE_SENDFILE)+" Bytes");
            ui->textBrowser->appendPlainText("Total packs: "+QString::number(SendFileBuff.size())+" packs");
            ui->textBrowser->appendPlainText("");
            QString str = ui->textBrowser->toPlainText();
            BrowserBuff.clear();
            BrowserBuff.append(str);
            hexBrowserBuff.clear();
            hexBrowserBuff.append(toHexDisplay(str.toLocal8Bit()));

            serial.write(SendFileBuff.at(SendFileBuffIndex++));//后续缓冲的发送在串口发送完成的槽里
        }
        else{
            openInteractiveUI();
            sendFile = false;
            QMessageBox::information(this,tr("提示"),tr("请先打开串口。"));
        }
    }else{
        QMessageBox::information(this,tr("提示"),tr("文件打开失败。"));
        lastFileName.clear();
    }
}

void MainWindow::on_actionValueDisplay_triggered(bool checked)
{
    if(checked){
        ui->valueDisplay->show();
        setVisualizerTitle();
    }else{
        ui->valueDisplay->hide();
        resetVisualizerTitle();
    }

    adjustLayout();
}

//复制所选文本到剪贴板
void MainWindow::copySelectedTextBrowser_triggered(void)
{
    QClipboard *clipboard = QApplication::clipboard();
    if(ui->textBrowser->textCursor().selectedText().isEmpty())
        return;
    clipboard->setText(ui->textBrowser->textCursor().selectedText());
}
//复制全部显示文本到剪贴板
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
//复制全部原始数据到剪贴板
void MainWindow::copyAllData_triggered(void)
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(RxBuff);
}
void MainWindow::on_textBrowser_customContextMenuRequested(const QPoint &pos)
{
    QPoint noWarning = pos;
    noWarning.x();

    QAction *copyText = nullptr;
    QAction *copyAllText = nullptr;
    QAction *copyAllData = nullptr;
    QAction *clearTextBrowser = nullptr;
    QAction *saveOriginData = nullptr;
    QAction *saveShowedData = nullptr;

    QMenu *popMenu = new QMenu( this );
    //添加右键菜单
    copyText = new QAction(tr("复制所选文本"), this);
    if(ui->textBrowser->textCursor().selectedText().isEmpty())
        copyText->setEnabled(false);
    copyAllText = new QAction(tr("复制全部显示数据"), this);
    copyAllData = new QAction(tr("复制全部原始数据"), this);
    saveShowedData = new QAction(tr("保存显示数据"), this);
    saveOriginData = new QAction(tr("保存原始数据"), this);
    clearTextBrowser = new QAction(tr("清空数据显示区"), this);

    popMenu->addAction( copyText );
    popMenu->addSeparator();
    popMenu->addAction( copyAllText );
    popMenu->addAction( copyAllData );
    popMenu->addSeparator();
    popMenu->addAction( saveShowedData );
    popMenu->addAction( saveOriginData );
    popMenu->addSeparator();
    popMenu->addAction( clearTextBrowser );

    connect( copyText, SIGNAL(triggered() ), this, SLOT( copySelectedTextBrowser_triggered()) );
    connect( copyAllText, SIGNAL(triggered() ), this, SLOT( copyAllTextBrowser_triggered()) );
    connect( copyAllData, SIGNAL(triggered() ), this, SLOT( copyAllData_triggered()) );
    connect( saveOriginData, SIGNAL(triggered() ), this, SLOT( on_actionSaveOriginData_triggered()) );
    connect( saveShowedData, SIGNAL(triggered() ), this, SLOT( on_actionSaveShowedData_triggered()) );
    connect( clearTextBrowser, SIGNAL(triggered() ), this, SLOT( clearTextBrowserSlot()) );
    popMenu->exec( QCursor::pos() );
    delete popMenu;
    delete clearTextBrowser;
    delete saveOriginData;
    delete saveShowedData;
    delete copyAllData;
    delete copyAllText;
    delete copyText;
}

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

void MainWindow::on_valueDisplay_customContextMenuRequested(const QPoint &pos)
{
    //消除警告
    QPoint pp = pos;
    pp.isNull();

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

void MainWindow::deleteValueDisplayRowSlot()
{
    QList<QTableWidgetItem*> selectedItems = ui->valueDisplay->selectedItems();

    while(selectedItems.size()){
        ui->valueDisplay->removeRow(selectedItems.at(0)->row());
        selectedItems.pop_front();
    }
}

void MainWindow::deleteValueDisplaySlot()
{
    while(ui->valueDisplay->rowCount()>0){
        ui->valueDisplay->removeRow(0);
    }
}


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

void MainWindow::on_timeStampTimeOut_textChanged(const QString &arg1)
{
    timeStampTimer.setSingleShot(true);
    timeStampTimer.start(arg1.toInt());
}

void MainWindow::on_actionOpenGL_triggered(bool checked)
{
    if(checked){
        ui->customPlot->setOpenGl(true);
        fft_window->setupPlotterOpenGL(true); //实际未使能
    }
    else{
        ui->customPlot->setOpenGl(false);
        fft_window->setupPlotterOpenGL(false);
    }
    ui->customPlot->replot();
}

void MainWindow::on_actionFontSetting_triggered()
{
    bool ok;
    QFont font;
    font = QFontDialog::getFont(&ok, font, this, tr("选择字体"));
    if(ok){
        g_font = font;
        ui->textBrowser->document()->setDefaultFont(g_font);
        ui->textEdit->document()->setDefaultFont(g_font);
        ui->multiString->setFont(g_font);
        ui->customPlot->plotControl->setupFont(g_font);
        fft_window->setupPlotterFont(g_font);

        QPlainTextEdit *textEdit = nullptr;
        for(qint32 i = 0; i < ui->tabWidget->count(); i++){
            textEdit = dynamic_cast<QPlainTextEdit *>(ui->tabWidget->widget(i));
            if(textEdit){
                textEdit->setFont(g_font);
            }
        }
    }
}

void MainWindow::on_actionBackGroundColorSetting_triggered()
{
    QColor color;
    color = QColorDialog::getColor(Qt::white, this,
                                          tr("选择背景色"),
                                          QColorDialog::ShowAlphaChannel);
    if(!color.isValid())
        return;

    qint32 r,g,b;
    g_background_color = color;
    g_background_color.getRgb(&r,&g,&b);
    QString str = "QPlainTextEdit{ background-color: rgb(RGBR,RGBG,RGBB);}";
    str.replace("RGBR", QString::number(r));
    str.replace("RGBG", QString::number(g));
    str.replace("RGBB", QString::number(b));
    ui->textBrowser->setStyleSheet(str);

    QPlainTextEdit *textEdit = nullptr;
    for(qint32 i = 0; i < ui->tabWidget->count(); i++){
        textEdit = dynamic_cast<QPlainTextEdit *>(ui->tabWidget->widget(i));
        if(textEdit){
            textEdit->setStyleSheet(str);
        }
    }
}

void MainWindow::on_actionSumCheck_triggered(bool checked)
{
    ui->actionSumCheck->setChecked(checked);
    g_enableSumCheck = checked;
    setVisualizerTitle();
}

void MainWindow::on_actionPopupHotkey_triggered()
{
    bool ok;
    QString newKeySeq = QInputDialog::getText(this, tr("修改全局弹出热键"), tr("新的全局弹出热键："),
                                             QLineEdit::Normal, g_popupHotKeySequence , &ok, Qt::WindowCloseButtonHint);
    if(ok==false)
    {
        return;
    }
    registPopupHotKey(newKeySeq);
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
//    qDebug()<<"MainWindow::keyPressEvent"<<e->key();
    emit sendKeyToPlotter(e, true);
}

void MainWindow::keyReleaseEvent(QKeyEvent *e)
{
//    qDebug()<<"MainWindow::keyReleaseEvent"<<e->key();
    emit sendKeyToPlotter(e, false);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    //消除警告
    if(event){
       event->size();
    }

    //只响应显示主窗口时的窗口改变动作，其他类型的窗口只做记录，下次显示主窗口时进行响应
    if(ui->tabWidget->tabText(ui->tabWidget->currentIndex()) != MAIN_TAB_NAME)
    {
        return;
    }

    //首次启动不运行，防止卡死
    static uint8_t first_run = 1;
    if(first_run){
        first_run = 0;
        return;
    }

    //MainWindow尺寸改变时计算当前窗口能显示多少字符。
    //若MainWindow尺寸未变，内部控件尺寸变化的情况使用轮询解决
    ui->textBrowser->setPlainText("");
    //获取列数
    while (ui->textBrowser->document()->lineCount()<2) {
        ui->textBrowser->insertPlainText("0");
    }
    //获取行数
    while(ui->textBrowser->verticalScrollBar()->maximum()==0)
    {
        ui->textBrowser->appendPlainText("0");
    }

    ui->textBrowser->moveCursor(QTextCursor::Start);
    ui->textBrowser->moveCursor(QTextCursor::Down);
    ui->textBrowser->moveCursor(QTextCursor::Left);
//    qDebug()<<"resizeEvent"
//            <<ui->textBrowser->document()->characterCount()
//            <<ui->textBrowser->document()->lineCount()
//            <<ui->textBrowser->textCursor().columnNumber();
    characterCount = (ui->textBrowser->textCursor().columnNumber() + 1) *
                     (ui->textBrowser->document()->lineCount() - 1);
    ui->textBrowser->setPlainText("");

    printToTextBrowser();
}

void MainWindow::splitterMovedSlot(int pos, int index)
{
    pos = !!pos;
    index = !!index;
//    qDebug()<<"splitterMovedSlot"<<pos<<index;

    //计算可显示字符并刷新显示
    resizeEvent(nullptr);
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    //禁止删除主窗口
    if(ui->tabWidget->tabText(index) == MAIN_TAB_NAME){
        ui->statusBar->showMessage(tr("不允许删除主窗口"), 2000);
        return;
    }
    emit tee_clearData(ui->tabWidget->tabText(index));
    if(ui->tabWidget->widget(index) != nullptr)
    {
        delete ui->tabWidget->widget(index);
    }
//    ui->tabWidget->removeTab(index);
}

void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    ui->tabWidget->setCurrentIndex(index);
    if(ui->tabWidget->tabText(index) == MAIN_TAB_NAME){
        resizeEvent(nullptr);
        RefreshTextBrowser = true;
    }
}

void MainWindow::on_actionSendComment_triggered(bool checked)
{
    if(checked)
    {
        ui->function->setTitle(tr("功能：发送注释"));
    }
    else
    {
        if(ui->hexSend->isChecked() && ui->textEdit->toPlainText().indexOf("//")!=-1)
        {
            QMessageBox::information(this, tr("提示"), tr("hex发送模式下关闭发送注释功能前需要先删除注释"));
            ui->actionSendComment->setChecked(true);
            return;
        }
        ui->function->setTitle(tr("功能"));
    }
    ui->actionSendComment->setChecked(checked);
}

void MainWindow::on_actionAutoRefreshYAxis_triggered(bool checked)
{

    ui->actionAutoRefreshYAxis->setChecked(checked);
    autoRefreshYAxisFlag = checked;
}

//拖拽进入时
void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasFormat("text/uri-list")) //只能打开文本文件
        e->acceptProposedAction(); //可以在这个窗口部件上拖放对象
}

//拖拽松开时
void MainWindow::dropEvent(QDropEvent *e)
{
    //获取文件路径列表
    QStringList text = e->mimeData()->text().split('\n');
    if(text.size() != 1)
    {
        QMessageBox::information(this, tr("提示"), tr("仅支持单个dat文件解析。"));
        return;
    }
    //获取单个文件路径并剔除前缀
    QString path;
    path = text.at(0);
    path = path.mid(8);
    if(!path.endsWith("dat"))
    {
        QMessageBox::information(this, tr("提示"), tr("仅支持单个dat文件解析。"));
        return;
    }
    if(QMessageBox::information(this, tr("提示"),
                                tr("确认解析该文件？") + "\n" + path,
                                QMessageBox::Ok, QMessageBox::Cancel) != QMessageBox::Ok)
    {
        return;
    }
    parseDatFile(path, false);
}

void MainWindow::on_actionSelectXAxis_triggered(bool checked)
{
    Q_UNUSED(checked)

    static QString defaultXAxisLabel = ui->customPlot->xAxis->label();
    bool ok;
    QString name;
    QVector<QString> nameSets = ui->customPlot->plotControl->getNameSetsFromPlot();
    QStringList list;
    list.append(tr("递增计数值"));
    for (qint32 i = 0;i < ui->customPlot->graphCount(); i++) {
        list.append(nameSets.at(i));
    }
    name = QInputDialog::getItem(this, tr("选择X轴"), tr("名称"),
                                    list, 0, false, &ok, Qt::WindowCloseButtonHint);
    if(!ok)
        return;

    //选择了新的X轴,更新g_xAxisSource
    for (qint32 i = 0; i < list.size(); i++) {
        if(list.at(i) == name)
        {
            g_xAxisSource = i;
            //更改X轴标签和隐藏被选为X轴的曲线
            if(g_xAxisSource != XAxis_Cnt)
            {
                ui->customPlot->xAxis->setLabel(name);
                qint32 j = 0;
                for (j = 0; j < ui->customPlot->graphCount(); j++) {
                    if(ui->customPlot->graph(j)->name() == name)
                    {
                        ui->customPlot->graph(j)->setVisible(false);
                        ui->customPlot->legend->item(j)->setTextColor(Qt::gray);
                        continue;
                    }
                    ui->customPlot->graph(j)->setVisible(true);
                    ui->customPlot->legend->item(j)->setTextColor(Qt::black);
                }
            }else
            {
                //时域模式显示所有曲线
                qint32 j = 0;
                for (j = 0; j < ui->customPlot->graphCount(); j++) {
                    ui->customPlot->graph(j)->setVisible(true);
                    ui->customPlot->legend->item(j)->setTextColor(Qt::black);
                }
                ui->customPlot->xAxis->setLabel(defaultXAxisLabel);
            }
            //并清空图像(不删除句柄)
            ui->customPlot->protocol->clearBuff();
            ui->customPlot->plotControl->clearPlotter(-1);
//            while(ui->customPlot->graphCount()>1){
//                ui->customPlot->removeGraph(ui->customPlot->graphCount() - 1);
//            }
            ui->customPlot->yAxis->rescale(true);
            ui->customPlot->xAxis->rescale(true);
            ui->customPlot->replot();
            break;
        }
    }
}

void MainWindow::on_actionFFTShow_triggered(bool checked)
{
    ui->actionFFTShow->setChecked(checked);
    if(!fft_window)
        return;
    if(checked)
    {
        QPoint pos = QPoint(this->pos().x() + this->geometry().width(), this->pos().y());
        fft_window->move(pos);
        fft_window->setVisible(true);
        setVisualizerTitle();
        return;
    }
    fft_window->setVisible(false);
    resetVisualizerTitle();//要放在setVisible后面
    return;
}

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

void MainWindow::on_actionTeeLevel2NameSupport_triggered(bool checked)
{
    p_textExtract->setLevel2NameSupport(checked);
    ui->actionTeeLevel2NameSupport->setChecked(checked);
}

void MainWindow::on_actionTeeSupport_triggered(bool checked)
{
    ui->actionTeeSupport->setChecked(checked);
    ui->actionTeeLevel2NameSupport->setEnabled(checked);
    textExtractEnable = checked;
}
