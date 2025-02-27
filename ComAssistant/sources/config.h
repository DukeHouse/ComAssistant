/**
 * @brief   配置读写处理文件
 * @file    config.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-2月-11
 * @note    主要分了几个节和若干关键字
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <QSettings>
#include <QSerialPort>
#include <QString>
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QTextCodec>
#include <QDateTime>
#include <QFont>
#include <QColor>
#include "dataprotocol.h"
#include <QApplication>

//保存路径
#define SAVE_PATH                   (QCoreApplication::applicationDirPath() + "/ComAssistantConfig.ini")

//默认发送间隔
#define DEFAULT_SEND_INTERVAL       (100)
//节
#define SECTION_GLOBAL              QString("Global/")
#define SECTION_SERIAL              QString("Serial/")
#define SECTION_NETWORK             QString("Network/")
#define SECTION_MULTISTR            QString("MultiStr/")
#define SECTION_PLOTTER             QString("Plotter/")
#define SECTION_STATISTIC           QString("Statistic/")
#define SECTION_ABOUT               QString("About/")

//global键
#define KEY_WORKMODE                QString("WorkMode")
#define KEY_FIRSTRUN                QString("FirstRun")
#define KEY_CODERULE                QString("CodeRule")
#define KEY_ENTERSTYLE              QString("EnterStyle")
#define KEY_TIMESTAMPSTATE          QString("TimeStampState")
#define KEY_SENDINTERVAL            QString("SendInterval")
#define KEY_HEXSENDSTATE            QString("HexSendState")
#define KEY_HEXSHOWSTATE            QString("HexShowState")
#define KEY_HIGHLIGHTSTATE          QString("HighlightState")
#define KEY_TEXTSENDAREA            QString("TextSendArea")
#define KEY_LASTFILEDIALOGPATH      QString("LastFileDialogPath")
#define KEY_TIMESTAMP_TIMEOUT       QString("TimeStampTimeOut")
#define KEY_GUIFONT                 QString("GUIFont")
#define KEY_BACKGROUNDCOLOR         QString("BackGroudColor")
#define KEY_THEME_INDEX             QString("ThemeIndex")
#define KEY_POPUPHOTKEY             QString("PopUpHotKey")
#define KEY_TEE_Support             QString("TeeEnable")
#define KEY_TEE_LEVEL2_NAME         QString("TeeLevel2NameEnable")
#define KEY_LOG_RECORD              QString("LogRecord")
#define KEY_ACTIVATED_TAB           QString("ActivatedTab")
#define KEY_REG_MATCH_STR           QString("RegMatchStr")
#define KEY_SIMPLE_MODE             QString("SimpleMode")
#define KEY_DEFAULT_PLOT_TITLE      QString("DefaultPlotTitle")

//serial键
#define KEY_PORTNAME                QString("PortName")
#define KEY_BAUDRATE                QString("Baudrate")
#define KEY_STOPBIT                 QString("StopBit")
#define KEY_DATABIT                 QString("DataBit")
#define KEY_PARITY                  QString("Parity")
#define KEY_FLOWCONTROL             QString("FlowControl")

//network键
#define KEY_NET_SECRET_KEY          QString("Key")
#define KEY_NETWORK_MODE            QString("NetworkMode")
#define KEY_UDPS_REMOTE_ADDR        QString("UDPServerRemoteAddr")
#define KEY_CLIENT_TARGET_IP        QString("ClientTargetIp")
#define KEY_CLIENT_TARGET_PORT      QString("ClientTargetPort")

//multiStr键
#define KEY_MULTISTRINGSTATE        QString("MultiStringState")
#define KEY_MULTISTRING             QString("MultiString")

//plotter键
#define KEY_PLOTTERSTATE            QString("PlotterState")
#define KEY_PROTOCOLTYPE            QString("ProtocolType")
#define KEY_GRAPHNAME               QString("GraphName")
#define KEY_XAXISNAME               QString("XAxisName")
#define KEY_YAXISNAME               QString("YAxisName")
#define KEY_VALUEDISPLAYSTATE       QString("ValueDisplayState")
#define KEY_OPENGLSTATE             QString("OpenGLState")
#define KEY_REFRESHYAXIS            QString("RefreshYAxis")
#define KEY_LINETYPE                QString("LineType")
//#define KEY_XRANGELENGH         QString("xRangeLength")

//statistic键
#define KEY_FIRST_STARTTIME         QString("FirstStartTime")
#define KEY_STARTTIME               QString("StartTime")
#define KEY_LASTRUNTIME             QString("LastRunTime")
#define KEY_TOTALRUNTIME            QString("TotalRunTime")
#define KEY_LASTTXCNT               QString("LastTxCnt")
#define KEY_TOTALTXCNT              QString("TotalTxCnt")
#define KEY_LASTRXCNT               QString("LastRxCnt")
#define KEY_TOTALRXCNT              QString("TotalRxCnt")
#define KEY_TOTALRUNCNT             QString("TotalRunCnt")
#define KEY_TOTALPLOTTERUSE         QString("TotalPlotterUse")
#define KEY_TOTALPLOTTERNUM         QString("TotalPlotterNum")
#define KEY_TOTALVALUEDISPLAYUSE    QString("TotalValueDisplayUse")
#define KEY_TOTALFFTUSE             QString("TotalFFTUse")
#define KEY_TOTALMULTISTRUSE        QString("TotalMultiStrUse")
#define KEY_TOTALASCIITABLEUSE      QString("TotalASCIITableUse")
#define KEY_TOTALPRIORITYTABLEUSE   QString("TotalPriorityTableUse")
#define KEY_TOTALSTM32ISPUSE        QString("TotalSTM32ISPUse")
#define KEY_TOTALHEXTOOLUSE         QString("TotalHexToolUse")
#define KEY_TOTALASCIILUSE          QString("TotalASCIIUse")
#define KEY_TOTALFLOATUSE           QString("TotalFLOATUse")
#define KEY_TOTALCSVUSE             QString("TotalCSVUse")
#define KEY_TOTALMADUSE             QString("TotalMADUse")
#define KEY_TOTALTEEUSE             QString("TotalTeeUse")
#define KEY_TOTALTEEPARSE           QString("TotalTeeParseCnt")
#define KEY_TOTALREGPARSE           QString("TotalRegParseCnt")
#define KEY_TOTALRECORDUSE          QString("TotalRecordUse")

//about键
#define KEY_VERSION                 QString("Version")
#define KEY_AUTHER                  QString("Auther")
#define KEY_EMAIL                   QString("Email")

//值
typedef enum {
    GBK,
    UTF8
}CodeRule_e;

typedef enum {
    WinStyle = 0,
    UnixStyle = 1
}EnterStyle_e;

typedef enum {
    Ascii = 0,
    Ascii_SumCheck,
    Float,
    Float_SumCheck,
    CSV,
    CSV_SumCheck,
    MAD,
    MAD_SumCheck,
}ProtocolType_e;

typedef enum {
    Line = 0,
    ScatterLine,
    Scatter,
}LineType_e;

extern int32_t version_to_number(QString str);

class Config
{
public:
    #define defualtGraphName  "Graph 1;Graph 2;Graph 3;Graph 4;Graph 5;Graph 6;Graph 7;Graph 8;Graph 9;Graph 10;Graph 11;Graph 12;Graph 13;Graph 14;Graph 15;"
    //版本
    #define VERSION_STRING  "0.5.6"

    Config();
    static void writeCommentMsgAtFileTop();
    static void writeDefault();
    static void createDefaultIfNotExist();
    static bool isFileExist(QString path);

    static void setFirstRun(bool flag);
    static bool getFirstRun();

    static void setVersion(void);
    static QString getVersion();//这个是从软件本身读版本号
    static QString readVersion(void);//这个是从配置文件中读版本号
    static int32_t getVersionNumber();
    static bool isEvalVersionFromIniFile();
    static int32_t versionCompare(QString oldVersion, QString newVersion);
    //serial
    static void setPortName(QString name);
    static QString getPortName();
    static void setBaudrate(int baud);
    static int getBaudrate();
    static void setParity(QSerialPort::Parity parity);
    static QSerialPort::Parity getParity();
    static void setDataBits(QSerialPort::DataBits databits);
    static QSerialPort::DataBits getDataBits();
    static void setStopBits(QSerialPort::StopBits stopbits);
    static QSerialPort::StopBits getStopBits();
    static void setFlowControl(QSerialPort::FlowControl flowControl);
    static QSerialPort::FlowControl getFlowControl();

    //global
    static void setCodeRule(CodeRule_e rule);
    static CodeRule_e getCodeRule();
    static void setEnterStyle(EnterStyle_e style);
    static EnterStyle_e getEnterStyle();
    static void setTimeStampState(bool checked);
    static bool getTimeStampState();
    static void setTimeStampTimeOut(int32_t timeout);
    static int32_t getTimeStampTimeOut();
    static void setSendInterval(const int interval);
    static int getSendInterval();
    static void setHexSendState(bool checked);
    static bool getHexSendState();
    static void setHexShowState(bool checked);
    static bool getHexShowState();
    static void setMultiStringState(bool checked);
    static bool getMultiStringState();
    static bool setMultiString(QStringList multiStr);
    static QStringList getMultiString();
    static void setKeyWordHighlightState(bool checked);
    static bool getKeyWordHighlightState();
    static void setTextSendArea(QString str);
    static QString getTextSendArea();
    static void setLastFileDialogPath(QString str);
    static QString getLastFileDialogPath();
    static void setGUIFont(QFont font);
    static QFont getGUIFont();
    static void setBackGroundColor(QColor color);
    static QColor getBackGroundColor();
    static void setPopupHotKey(QString keySequence);
    static QString getPopupHotKey();
    static void setTeeSupport(bool enable);
    static bool getTeeSupport();
    static void setTeeLevel2NameSupport(bool enable);
    static bool getTeeLevel2NameSupport();
    static void setLogRecord(bool enable);
    static bool getLogRecord();

    //plotter
    static void setPlotterState(bool checked);
    static bool getPlotterState();
    static void setProtocolType(ProtocolType_e type);
    static ProtocolType_e getProtocolType();
    static void setPlotterGraphNames(QVector<QString> names);
    static QVector<QString> getPlotterGraphNames(int maxValidGraphNumber);
    static void setXAxisName(QString str);
    static QString getXAxisName();
    static void setYAxisName(QString str);
    static QString getYAxisName();
    static void setValueDisplayState(bool isOn);
    static bool getValueDisplayState();
    static void setOpengGLState(bool isOn);
    static bool getOpengGLState();
    static void setRefreshYAxisState(bool isOn);
    static bool getRefreshYAxisState();
    static void setLineType(LineType_e type);
    static LineType_e getLineType();

    //static
    static void setFirstStartTime(QString time);
    static QString getFirstStartTime(void);
    static void setStartTime(QString time);
    static QString getStartTime(void);
    static void setLastRunTime(int sec);
    static QString getLastRunTime(void);
    static void setTotalRunTime(int64_t sec);
    static QString getTotalRunTime(void);
    static void setLastTxCnt(int64_t cnt);
    static QString getLastTxCnt(void);
    static void setLastRxCnt(int64_t cnt);
    static QString getLastRxCnt(void);
    static void setTotalTxCnt(int64_t currentTxCnt);
    static QString getTotalTxCnt(void);
    static void setTotalRxCnt(int64_t currentRxCnt);
    static QString getTotalRxCnt(void);
    static void setTotalRunCnt(int64_t runCnt=1);
    static QString getTotalRunCnt(void);

    //general
    static void addCurrentStatistic(QString key, int64_t cnt);
    static int64_t getTotalStatistic(QString key);
    static void setConfigString(QString section, QString key, QString containt);
    static QString getConfigString(QString section, QString key, QString defaultStr);
    static void setConfigBool(QString section, QString key, bool flag);
    static bool getConfigBool(QString section, QString key, bool defaultBool);
    static void setConfigNumber(QString section, QString key, int64_t num);
    static int64_t getConfigNumber(QString section, QString key, int64_t defaultNum);
};

#endif // CONFIG_H
