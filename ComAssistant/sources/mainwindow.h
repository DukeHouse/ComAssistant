/**
 * @brief   主窗口交互处理文件
 * @file    mainwindow.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-2月-11
 * @note    各种UI交互和数据刷新
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextCodec>
#include <QString>
#include <QDebug>
#include <QtCore/QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QTimer>
#include <QDateTime>
#include <QFileDialog>
#include <QTextStream>
#include <QDataStream>
#include <QFile>
#include <QRegExp>
#include <QListWidgetItem>
#include <QDesktopWidget>
#include <QVector>
#include <QMenu>
#include <QInputDialog>
#include <QList>
#include <QThread>
#include <QRegExpValidator>
//绘图器类
#include "myqcustomplot.h"
#include "dataprotocol.h"
//自定义类
#include <QHotkey>
#include "common.h"
#include "dataprotocol.h"
#include "myxlsx.h"
#include "highlighter.h"
#include "myserialport.h"
#include "baseconversion.h"
#include "config.h"
#include "http.h"
#include "data_logger.h"
#include "reg_match_engine.h"
#include "plotter_manager.h"
#include "tee_manager.h"
#include "network_comm.h"
#include "file_unpacker.h"
//界面类
#include "stm32isp_dialog.h"
#include "about_me_dialog.h"
#include "settings_dialog.h"
#include "text_browser_dialog.h"
#include "hex_tool_dialog.h"
//文本提取引擎
#include "text_extract_engine.h"
//FFT显示
#include "fft_dialog.h"
#include "fft.h"

namespace Ui {
class MainWindow;
}

extern bool g_agree_statement;
extern bool g_log_record;
extern bool g_debugger;

#define BIRTHDAY_YEAR               "2020"
#define BIRTHDAY_DATE               "02-16"

#define RECOVERY_FILE_PATH          (QCoreApplication::applicationDirPath() + "/ComAssistantRecovery.dat")
#define BACKUP_RECOVERY_FILE_PATH   (QCoreApplication::applicationDirPath() + "/ComAssistantRecovery_back.dat")

#define UNPACK_SIZE_OF_RX           (4096)
#define UNPACK_SIZE_OF_TX           (256)

#define TRY_REFRESH_BROWSER_CNT     (10)    //500ms内持续刷新
#define DO_NOT_REFRESH_BROWSER      (0)

#define NETWORK_SECRET_KEY          "HELLONETWORK"

//菜单栏的设置按钮的实现宏，保留功能：点击后可以自动对所有绘图器进行设置。
//由于感觉必要性不是很强，在考虑是删除还是使用
//先暂时保留，里面的代码也没怎么调。
#define SHOW_PLOTTER_SETTING        (0)

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void parseFileSignal();
private slots:
    void on_refreshCom_clicked();
    void tryOpenSerial();
    void readSerialPort();
    void serialBytesWritten(qint64 bytes);
    void handleSerialError(QSerialPort::SerialPortError errCode);

    void printToTextBrowser();

    //main gui
    void on_comSwitch_clicked(bool checked);
    void on_sendButton_clicked();
    void on_clearWindows_clicked();
    void on_clearWindows_simple_clicked();
    void on_clearWindows_simple_net_clicked();
    void on_cycleSendCheck_clicked(bool checked);
    void on_textEdit_textChanged();
    void on_hexSend_stateChanged(int arg1);
    void on_hexDisplay_clicked(bool checked);
    void on_baudrateList_currentTextChanged(const QString &arg1);
    void on_comList_textActivated(const QString &arg1);
    void on_sendInterval_textChanged(const QString &arg1);
    void on_multiString_itemDoubleClicked(QListWidgetItem *item);
    void verticalScrollBarActionTriggered(int action);
    void splitterMovedSlot(int pos, int index);
    void on_tabWidget_tabCloseRequested(int index);
    void on_tabWidget_plotter_tabCloseRequested(int index);
    void on_tabWidget_tabBarClicked(int index);
    void on_tabWidget_plotter_tabBarClicked(int index);
    void on_timeStampCheckBox_stateChanged(int arg1);
    void on_timeStampTimeOut_textChanged(const QString &arg1);
    void disableRefreshWindow_triggered(bool checked);
    void showAllTextBrowser_triggered();
    void editMode_triggered(bool checked);
    void copySelectedTextBrowser_triggered(void);
    void copyAllTextBrowser_triggered(void);
    void copyAllData_triggered(void);
    void on_regMatchEdit_textChanged(const QString &arg1);
    void on_regMatchSwitch_clicked(bool checked);

    //file
    void on_actionSaveOriginData_triggered();
    void on_actionOpenOriginData_triggered();
    void on_actionSaveShowedData_triggered();
    void on_actionSavePlotData_triggered();
    void on_actionSavePlotAsPicture_triggered();
    void on_actionSendFile_triggered();

    //function
    void on_actionSimpleMode_triggered(bool checked);
    void on_actionMultiString_triggered(bool checked);
    void on_actionSTM32_ISP_triggered();
    void on_actionPopupHotkey_triggered();
    void on_actionTeeSupport_triggered(bool checked);
    void on_actionTeeLevel2NameSupport_triggered(bool checked);
    void on_actionASCIITable_triggered();
    void on_actionRecordRawData_triggered(bool checked);
    void on_actionRecordGraphData_triggered(bool checked);
    void on_actionHexConverter_triggered(bool checked);
    void on_actionPriorityTable_triggered();
    void on_actionSelectTheme_triggered();
    void on_networkModeBox_activated(const QString &arg1);
    void on_networkSwitch_clicked(bool checked);
    void on_actionNetworkMode_triggered(bool checked);
    void on_comboBox_remoteAddr_activated(const QString &arg1);
    void on_comboBox_remoteAddr_currentTextChanged(const QString &arg1);

    //setting
    void on_actionCOM_Config_triggered();
    void on_actionGBK_triggered(bool checked);
    void on_actionUTF8_triggered(bool checked);
    void on_action_winLikeEnter_triggered(bool checked);
    void on_action_unixLikeEnter_triggered(bool checked);
    void on_actionKeyWordHighlight_triggered(bool checked);
    void on_actionFontSetting_triggered();
    void on_actionBackGroundColorSetting_triggered();
    void on_actionResetDefaultConfig_triggered(bool checked);

    //visualization
    void on_actionPlotterSwitch_triggered(bool checked);
    void on_actionValueDisplay_triggered(bool checked);
    void on_actionFFTShow_triggered(bool checked);
    void on_actionAscii_triggered(bool checked);
    void on_actionFloat_triggered(bool checked);
    void on_actionCSV_triggered(bool checked);
    void on_actionMAD_triggered(bool checked);
    void on_actiondebug_triggered(bool checked);
    void on_actionSumCheck_triggered(bool checked);
#if SHOW_PLOTTER_SETTING
    void on_actionLinePlot_triggered();
    void on_actionScatterLinePlot_triggered();
    void on_actionScatterPlot_triggered();
    void on_actionAutoRefreshYAxis_triggered(bool checked);
    void on_actionOpenGL_triggered(bool checked);
    void on_actionSelectXAxis_triggered(bool checked);
    void on_actionTimeStampMode_triggered(bool checked);
#endif
    void on_actionSetDefaultPlotterTitle_triggered();

    //help
    void on_actionManual_triggered();
    void on_actionUsageStatistic_triggered();
    void on_actionUpdate_triggered();
    void on_actionAbout_triggered();
    void on_actionLogRecord_triggered(bool checked);

    //timer
    void secTimerSlot();
    void debugTimerSlot();
    void cycleSendTimerSlot();
    void printToTextBrowserTimerSlot();
    void plotterShowTimerSlot();
    void multiStrSeqSendTimerSlot();
    void parseTimer100hzSlot();

    //contextMenuRequested
    void on_textBrowser_customContextMenuRequested(const QPoint &pos);
    void clearTextBrowserSlot();
    void on_valueDisplay_customContextMenuRequested(const QPoint &pos);
    void deleteValueDisplayRowSlot();
    void deleteValueDisplaySlot();
    void on_multiString_customContextMenuRequested(const QPoint &pos);
    void editSeedSlot();
    void editCommentSeedSlot();
    void moveUpSeedSlot();
    void moveDownSeedSlot();
    void deleteSeedSlot();
    void clearSeedsSlot();
    void addSeedSlot();

private:
    QString formatTime(int ms);
    bool needSaveConfig = true;
    void refreshCom();
    void readConfig();
    bool registPopupHotKey(QString keySequence);
    void layoutConfig();
    void adjustLayout();
    void quickHelp();
    void openInteractiveUI();
    void closeInteractiveUI();
    int32_t divideDataToPacks(QByteArray &input, QByteArrayList &output, int32_t pack_size, bool &divideFlag);
    int32_t parseDatFile(QString path, bool removeAfterRead);
    int32_t appendDataToFile(QString path, QByteArray &buff);
    void readRecoveryFile();
    void setVisualizerTitle(void);
    void resetVisualizerTitle(void);
    void addTextToMultiString(const QString &text, bool forceAdd);
    void updateProgressBar(QString preStr, double percent);
    void parsePlotterAndTee();
    void updateUIPanelBackground(QColor itsColor);
    void updateUIPanelFont(QFont font);
    int32_t firstRunNotify();
    void updateFunctionButtonTitle();
    QString sta_ConvertHuman_Byte(double num);
    QString sta_ConvertHuman_Time(double sec);
    QString statisticConvertRank(double totalTx, double totalRx);
    MyQCustomPlot* selectCurrentPlotter();
    MyQCustomPlot* createNewPlotter(QString plotterTitle);
    void fillDataToValueDisplay(MyQCustomPlot *plotter);
    void TxRxSpeedStatisticAndDisplay();
    int32_t recordGraphDataToFile(const QString& recordPlotTitle,
                                  const QString& plotterTitle,
                                  const QVector<double>& oneRowData);
    void calcCharacterNumberInWindow();
    int32_t checkBlankProblem();
    int32_t checkScrollBarTooLarge();
    void setWindowTheme(int32_t themeIndex);
    void setAllNetControlPanelEnabled(bool enable);
    bool checkIPAddrIsValid(void);
    void addNetAddrToComBoBox(void);
    void changeCommMode(bool isNetworkComm);
    void updateNetworkSwitchText(const QString &networkMode, bool pressed);
    void appendMsgLogToBrowser(QString str);
    int32_t unpack_file(bool &actionType, QString path, bool deleteIfSuccess, int32_t pack_size);
    int32_t readWriteAuthorityTest(QString testPath);
    void debuggerModeControl();
    int32_t writeDataToDevice(const QByteArray &data);
    int32_t deviceIsOpen();
    int32_t remindDeviceIsOpen();
    void textBrowser_rightClickContext_init();
    bool networkAuthorityCheck(bool checked, bool remind);
    QByteArray popDataSafety(QByteArray &data);
    void SendTmpBuffDataToFunctionModule();
    Ui::MainWindow *ui;
    mySerialPort serial;

    int32_t  g_network_comm_mode = 0;    // 串口网络切换开关
    qint32   g_multiStr_cur_index = -1;  // -1 means closed this function
    QColor   g_background_color;
    QFont    g_font;
    bool     g_enableSumCheck = false;
    qint64   g_lastSecsSinceEpoch = 0;
    QString  g_popupHotKeySequence;
    QHotkey  *g_popupHotkey = nullptr;
    int32_t  g_theme_index = 0;
    bool initOK = false;

    //绘图解析器
    DataProtocol *plotProtocol = nullptr;
    QThread *plotProtocol_thread = nullptr;

    QProgressBar *progressBar;
    QLabel *statusSpeedLabel, *statusStatisticLabel, *statusRemoteMsgLabel, *statusTimer; //状态栏标签

    bool sendFile = false;
    bool readFile = false;
    QByteArray readFileBuff;   //解析文件分包缓冲

    QByteArray RxBuff, TxBuff;          //原始数据的收发缓冲
    QByteArray hexBrowserBuff;          //十六进制格式的浏览器缓冲
    int32_t hexBrowserBuffIndex = 0;    //显示指示(逆序)
    QByteArray BrowserBuff;             //浏览器缓冲
    int32_t BrowserBuffIndex = 0;       //显示指示(逆序)
    QByteArray unshowedRxBuff;          //未上屏的接收缓冲
    QByteArray networkRxBuff;           //网络接收缓冲

    QByteArray plotterTmpBuff;      //数据绘图功能中转缓冲
    QByteArray teeTmpBuff;          //数据分窗功能中转缓冲
    QByteArray regMTmpBuff;         //数据过滤功能中转缓冲
    QByteArray recoveryTmpBuff;     //数据恢复功能中转缓冲
    QByteArray rawRecordTmpBuff;    //数据记录功能中转缓冲

    const int32_t PLOTTER_SHOW_PERIOD = 40;  //绘图器显示频率25FPS（解析频率由parseTimer100hz控制）
    const int32_t TEXT_SHOW_PERIOD    = 55;  //文本显示频率18FPS

    bool is_multi_str_double_click = false;

    QTimer cycleSendTimer;  //循环发送定时器
    QTimer debugTimer;      //调试定时器
    QTimer secTimer;        //秒定时器
    QTimer timeStampTimer;  //时间戳定时器
    QTimer printToTextBrowserTimer; //刷新文本显示区的定时器
    QTimer parseTimer100hz;         //解析定时器（文本和绘图）
    QTimer plotterShowTimer;        //绘图显示定时器
    QTimer multiStrSeqSendTimer;    //多字符串序列发送定时器

    QString lastFileDialogPath; //上次文件对话框路径

    Highlighter *highlighter = nullptr; //高亮器
    Highlighter *highlighter1 = nullptr;

    HTTP *http = nullptr;

    int32_t TryRefreshBrowserCnt = TRY_REFRESH_BROWSER_CNT; //数据显示区刷新标记，大于0的时候会继续刷新

    //文件解包（分包）器
    FileUnpacker* fileUnpacker = nullptr;

    //textBrowser右键菜单
    QMenu *popMenu = nullptr;
    QAction *stopRefresh = nullptr;
    QAction *showAllText = nullptr;
    QAction *editMode = nullptr;
    QAction *copyText = nullptr;
    QAction *copyAllText = nullptr;
    QAction *copyAllData = nullptr;
    QAction *clearTextBrowser = nullptr;
    QAction *saveOriginData = nullptr;
    QAction *saveShowedData = nullptr;

    //数据记录
    QString rawDataRecordPath       = "";
    QString lastRawDataRecordPath   = "";
    QString graphDataRecordPath     = "";
    QString lastGraphDataRecordPath = "";
    QString recordPlotTitle         = "";
    QThread *p_logger_thread;
    Data_Logger *p_logger;

    //暂停刷新flag与暂停刷新时的Index
    bool disableRefreshWindow = false;
    int32_t disableRefreshBrowserSize = 0;
    int32_t disableRefreshHexBrowserSize = 0;

    //统计
    int64_t currentRunTime = 0; //运行时间
    double rxSpeedKB = 0;
    double txSpeedKB = 0;
    int64_t statisticRxByteCnt = 0;
    int64_t statisticTxByteCnt = 0;
    int64_t statisticPlotterNumCnt = 0;
    int64_t statisticPlotterRxByteCnt = 0;
    int64_t statisticValueDisplayUseCnt = 0;
    int64_t statisticPlotterUseCnt  = 0;
    int64_t statisticFFTUseCnt = 0;
    int64_t statisticMultiStrUseCnt = 0;
    int64_t statisticAsciiTableUseCnt = 0;
    int64_t statisticPriorityTableUseCnt = 0;
    int64_t statisticStm32IspUseCnt = 0;
    int64_t statisticHexToolUseCnt = 0;
    int64_t statisticASCIIUseCnt = 0;
    int64_t statisticFLOATUseCnt = 0;
    int64_t statisticCSVUseCnt = 0;
    int64_t statisticMADUseCnt = 0;
    int64_t statisticTeeUseCnt = 0;
    int64_t statisticTeeParseCnt = 0;
    int64_t statisticRegParseCnt = 0;
    int64_t statisticRecordCnt = 0;

    //布局
    QSplitter *splitter_output = NULL;
    QSplitter *splitter_io = NULL;
    QVBoxLayout *central = NULL;

    //窗口显示字符统计
    QSize windowSize;
    int32_t characterCount = 0; //可显示字符数
    int32_t characterCount_bak = 0;
    int32_t characterCount_Row = 0;
    int32_t characterCount_Col = 0;

    //文本提取引擎
    bool textExtractEnable = true;
    QThread *p_textExtractThread;
    TextExtractEngine *p_textExtract;

    //正则匹配引擎
    QThread *p_regMatchThread = nullptr;
    RegMatchEngine *p_regMatch = nullptr;
    QByteArray regMatchBuffer;//正则匹配缓冲（防止高频刷新带来的CPU压力）
    QMutex regMatchBufferLock;

    //网络通信模块
    NetworkComm* p_networkComm = nullptr;
    QThread *p_networkCommThread = nullptr;
    QStringList client_targetIP_backup_List;    //Client的目的地址记忆
    QStringList server_remoteAddr_backup_list;  //其实就是UDP Server的远端地址记忆
    QString networkSecretKey;

    //fft window
    FFT_Dialog *fft_window = nullptr;

    //绘图器集合管理
    PlotterManager plotterManager;
    //Tee窗口集合管理
    TeeManager teeManager;

signals:
    void protocol_appendData(const QByteArray &data);
    void protocol_parseData(bool enableSumCheck = false);
    void protocol_clearBuff(const QString &name);
    void tee_appendData(const QByteArray &str);
    void tee_parseData(void);
    void tee_clearData(const QString &name);
    qint32 tee_saveData(const QString &path, const QString &name, const bool& savePackBuff);
    void sendKeyToPlotter(QKeyEvent *e, bool isPressAct);
    void logger_append(uint8_t type, const QByteArray &data);
    void logger_flush(uint8_t type);
    void regM_appendData(const QByteArray &str);
    void regM_parseData(void);
    void regM_clearData(void);
    qint32 regM_saveData(const QString &path);
    void initNetwork();
    void writeToNetwork(const QByteArray &str);
    int32_t connectToNetwork(qint32 mode = TCP_CLIENT, QString ip = DEFAULT_IP, quint16 port = DEFAULT_PORT);
    int32_t disconnectFromNetwork();

public slots:
    void recvNewFilePack(const QByteArray &pack, qint32 current_cnt, qint32 total_cnt);
    void recvUnpackResult(bool success, QString details);
    void tee_textGroupsUpdate(const QString &name, const QByteArray &data);
    void tee_saveDataResult(const qint32& result, const QString &path, const qint32 fileSize);
    void regM_dataUpdated(const QByteArray &packData);
    void regM_saveDataResult(const qint32& result, const QString &path, const qint32 fileSize);
    void errorNetwork(qint32 err, QString details);
    void msgNetwork(qint32 type, QString msg);
    void readDataNetwork(const QByteArray &data);

protected:
    void resizeEvent(QResizeEvent* event);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    void closeEvent(QCloseEvent*event);
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    void moveEvent(QMoveEvent *event);
};

#endif // MAINWINDOW_H
