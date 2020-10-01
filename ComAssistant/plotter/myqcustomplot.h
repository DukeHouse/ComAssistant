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

#include "qcustomplot.h"

#include "dataprotocol.h"
#include "qcustomplotcontrol.h"
#include "axistag.h"
#include "mytracer.h"

#include <QStatusBar>
class QCustomPlotControl;

class MyQCustomPlot:public QCustomPlot
{
    Q_OBJECT
public:
    MyQCustomPlot(QWidget* parent = nullptr);
    ~MyQCustomPlot();

    MyTracer *m_Tracer; //坐标跟随鼠标
    void init(QStatusBar* pBar, QMenu* plotterSetting, QAction* saveGraphData, QAction* saveGraphPicture);
    bool saveGraphAsTxt(const QString& filePath, char separate=' ');
    QCustomPlotControl *plotControl;
    DataProtocol *protocol;
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
};

#endif // MYQCUSTOMPLOT_H
