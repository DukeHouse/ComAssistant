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
//绘图器类
#include "myqcustomplot.h"
#include "dataprotocol.h"
//自定义类
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
//界面类
#include "stm32isp_dialog.h"
#include "about_me_dialog.h"
#include "settings_dialog.h"
#include "ascii_table_dialog.h"
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

#define TRY_REFRESH_BROWSER_CNT     (10)    //500ms内持续刷新
#define DO_NOT_REFRESH_BROWSER      (0)

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
    void parseFileSlot();

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
    void copySelectedTextBrowser_triggered(void);
    void copyAllTextBrowser_triggered(void);
    void copyAllData_triggered(void);
    void on_regMatchEdit_textChanged(const QString &arg1);

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
    void addTextToMultiString(const QString &text);
    void updateProgressBar(QString preStr, double percent);
    void parsePlotterAndTee();
    void updateUIPanelBackground(QString background);
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
    void reduceShowedText();
    void calcCharacterNumberInWindow();
    Ui::MainWindow *ui;
    mySerialPort serial;

    //绘图解析器
    DataProtocol *plotProtocol = nullptr;
    QThread *plotProtocol_thread = nullptr;

    QProgressBar *progressBar;
    QLabel *statusSpeedLabel, *statusStatisticLabel, *statusRemoteMsgLabel, *statusTimer; //状态栏标签

    bool sendFile = false;
    bool parseFile = false;
    QByteArrayList parseFileBuff;   //解析文件分包缓冲
    int parseFileBuffIndex = 0;     //即单次处理最多2G的数据
    QByteArrayList SendFileBuff;    //发送文件分包缓冲
    int SendFileBuffIndex = 0;

    QByteArray RxBuff, TxBuff;      //原始数据的收发缓冲
    QByteArray hexBrowserBuff;      //十六进制格式的浏览器缓冲
    int hexBrowserBuffIndex = 0;    //显示指示(逆序)
    QByteArray BrowserBuff;         //浏览器缓冲
    int BrowserBuffIndex = 0;       //显示指示(逆序)
    QByteArray unshowedRxBuff;      //未上屏的接收缓冲

    const int32_t PLOTTER_SHOW_PERIOD = 40;  //绘图器显示频率25FPS（解析频率由parseTimer100hz控制）
    const int32_t TEXT_SHOW_PERIOD    = 55;  //文本显示频率18FPS

    bool is_multi_str_double_click = false;

    int32_t forceTrigParse = 0;     //强制触发解析

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
    bool autoRefreshYAxisFlag;

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
    QThread *p_regMatchThread;
    RegMatchEngine *p_regMatch;
    QByteArray regMatchBuffer;//正则匹配缓冲（防止高频刷新带来的CPU压力）
    QMutex regMatchBufferLock;

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

public slots:
    void tee_textGroupsUpdate(const QString &name, const QByteArray &data);
    void tee_saveDataResult(const qint32& result, const QString &path, const qint32 fileSize);
    void regM_dataUpdated(const QByteArray &packData);
    void regM_saveDataResult(const qint32& result, const QString &path, const qint32 fileSize);

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
