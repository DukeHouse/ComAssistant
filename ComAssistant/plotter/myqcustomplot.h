#ifndef MYQCUSTOMPLOT_H
#define MYQCUSTOMPLOT_H

#include <QWidget>
#include <QObject>
#include <QInputDialog>
#include <QMessageBox>
#include <QtDebug>
#include <QString>
#include <QFile>
#include <QSharedPointer>
#include <QThread>

#include "qcustomplot.h"

#include "dataprotocol.h"
#include "qcustomplotcontrol.h"
#include "axistag.h"
#include "mytracer.h"

#include "fft_dialog.h"

#define    XAxis_Cnt        0
#define    XAxis_Graph0     1
#define    XAxis_Graph1     2
#define    XAxis_Graph2     3
#define    XAxis_Graph3     4
#define    XAxis_Graph4     5
#define    XAxis_Graph5     6
#define    XAxis_Graph6     7
#define    XAxis_Graph7     8
#define    XAxis_Graph8     9
#define    XAxis_Graph9     10
#define    XAxis_Graph10    11
#define    XAxis_Graph11    12
#define    XAxis_Graph12    13
#define    XAxis_Graph13    14
#define    XAxis_Graph14    15
//        XAxis_Graph15,

class QCustomPlotControl;

class MyQCustomPlot:public QCustomPlot
{
    Q_OBJECT
public:

    MyQCustomPlot(QWidget* parent = nullptr);
    ~MyQCustomPlot();

    MyTracer *m_Tracer; //坐标跟随鼠标
    void init(QAction* saveGraphData,
              QAction* saveGraphPicture,
              int32_t xSource, 
              bool autoRescaleYAxisFlag, 
              FFT_Dialog* window, 
              DataProtocol* protocol,
              QString plotterTitle);
    bool saveGraphAsTxt(const QString& filePath, char separate=' ');
    QCustomPlotControl *plotControl = nullptr;
    QString getPlotterTitle();
    void setAutoRescaleYAxis(bool autoRescaleYAxisFlag);
    bool getAutoRescaleYAxis();
    void setxAxisSource(int32_t xSource);
    int32_t getxAxisSource();
    int32_t useOpenGL(bool flag);
    bool getUseOpenGLState();
    void clear();
public slots:
    void recvKey(QKeyEvent *e, bool isPressAct);
private slots:
    void axisLabelDoubleClick(QCPAxis* axis, QCPAxis::SelectablePart part);
    void legendDoubleClick(QCPLegend* legend, QCPAbstractLegendItem* item);
    void selectionChanged();
    void mousePress(QMouseEvent* m);
    void mouseWheel(QWheelEvent* w);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    void removeSelectedGraph();
    void rescaleXYAxis();
    void removeAllGraphs();
    void reNameSelectedGraph();
    void reColorSelectedGraph();
    void hideSelectedGraph();
    void hideAllGraph();
    void showAllGraph();
    bool isAllGraphHide();
    void contextMenuRequest(QPoint pos);
    void moveLegend();
    void graphClicked(QCPAbstractPlottable *plottable, int dataIndex);
    void showTracer(QMouseEvent *event);

    //setting menu slots
    void on_actionLineType_triggered();
    void on_actionAutoRefreshYAxis_triggered(bool checked);
    void on_actionOpenGL_triggered(bool checked);
    void on_actionSelectXAxis_triggered();
    void on_actionTimeStampMode_triggered(bool checked);
signals:
    void protocol_clearBuff(const QString &name);

private:
    void organizeSettingMenu(QMenu* menu);
    QAction* saveData = nullptr;
    QAction* savePicture = nullptr;
    qint32 key = 0;
    int32_t xAxisSource = XAxis_Cnt;
    bool autoRescaleYAxis = true;
    FFT_Dialog* fft_dialog = nullptr;
    DataProtocol* p_protocol = nullptr;
    QString titleName;
    bool useGPUFlag = false;
};

#endif // MYQCUSTOMPLOT_H
