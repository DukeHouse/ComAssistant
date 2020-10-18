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

#include <QStatusBar>
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
    void init(QStatusBar* pBar, QMenu* plotterSetting, QAction* saveGraphData, QAction* saveGraphPicture,
              qint32 *xSource, bool *autoRescaleYAxisFlag, FFT_Dialog* window);
    bool saveGraphAsTxt(const QString& filePath, char separate=' ');
    QCustomPlotControl *plotControl = nullptr;
    DataProtocol *protocol = nullptr;
    QThread *protocol_thread = nullptr;
    void appendDataWaitToParse(const QByteArray &data);
    void startParse(bool enableSumCheck=false);
signals:
    void appendData(const QByteArray &data);
    void parseData(bool enableSumCheck=false);
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
    void rescaleYAxis();
    void removeAllGraphs();
    void hideSelectedGraph();
    void hideAllGraph();
    void showAllGraph();
    bool isAllGraphHide();
    void contextMenuRequest(QPoint pos);
    void moveLegend();
    void graphClicked(QCPAbstractPlottable *plottable, int dataIndex);
    void showTracer(QMouseEvent *event);

private:
    QStatusBar* bar = nullptr;
    QMenu* setting = nullptr;
    QAction* saveData = nullptr;
    QAction* savePicture = nullptr;
    qint32 key = 0;
    qint32 *xAxisSource = nullptr;
    bool *autoRescaleYAxis = nullptr;
    FFT_Dialog* fft_dialog = nullptr;
};

#endif // MYQCUSTOMPLOT_H
