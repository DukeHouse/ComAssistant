#ifndef QCUSTOMPLOTCONTROL_H
#define QCUSTOMPLOTCONTROL_H

#include "qcustomplot.h"
#include "myqcustomplot.h"
#include <QColor>
#include <QVector>
#include <QMenu>
#include <QInputDialog>
#include <algorithm>
#include "axistag.h"
#include "fft_dialog.h"

using namespace std;

class MyQCustomPlot;

class QCustomPlotControl
{

public:
    typedef enum {
        Line,
        ScatterLine,
        Scatter
    }LineType_e;
    QCustomPlotControl();
    ~QCustomPlotControl();
    QCustomPlotControl(MyQCustomPlot* plot, FFT_Dialog *window);//不知道为啥用了会崩溃
    void setEnableTimeStampMode(bool enable);
    bool getEnableTimeStampMode();
    bool addGraph(int num=1);
    //清除指定曲线，-1清除所有曲线
    void clearPlotter(int index);
    //调整x轴范围
//    void adjustXRange(const QCPRange& qcpRange);
//    void adjustXRange(bool enlarge);
    //把数据添加到绘图器里（不刷新图像）
    bool addDataToPlotter(QVector<double> rowData, qint32 xSource);
//    设置字体
    void setupFont(QFont font);
    //设置openGL
    void setupOpenGL(bool enabled);
    //设置笔宽度
    void setupPenWidth(double width = 2);
    //设置轴间距
    void setupAxisTick(int axisTickLenth = 6, int axisSubTickLenth = 3);
    //设置轴标签
    void setupAxisLabel(QString xLabel="Point number", QString yLabel="Value");
    //设置曲线名字
    bool setupPlotName(QVector<QString> nameStr=QVector<QString>());
    //设置线型（线图、点线图、点图）: 点风格+线风格
    void setupLineType(LineType_e type);
    //设置图例可见性
    void setupLegendVisible(bool visible=true);
    //设置图像可见性
    void setupGraphVisible(bool visible);
    //设置交互功能
    void setupInteractions();
    //设置轴盒子
    void setupAxesBox(bool connectRanges=false);
    //设置绘图器（总设置）
    void setupPlotter(MyQCustomPlot* plot, FFT_Dialog *window);
    //读写名字集
    QVector<QString> getNameSetsFromPlot();
    void setNameSet(QVector<QString> names);
    //最大图像数量
    int getMaxValidGraphNumber();
    //返回横坐标长度
    double getXAxisLength();
    bool setXAxisLength(double length);
    //读写颜色集
    QVector<QColor> getColorSet();
    void setColorSet(QVector<QColor> colors);

private slots:

private:
    FFT_Dialog *fft_dialog = nullptr;
    MyQCustomPlot* customPlot;
    QVector<QColor> colorSet;
    QVector<QString> nameSet;
    QCPRange xRange; //
    const double zoomScale = 0.2;
    QVector<QCPScatterStyle::ScatterShape> scatterShapeSet;
//    AxisTag *mTag1;   //动态标签
    long int xAxisCnt = 0;
    LineType_e lineType = Line; //线型种类
    bool enableTimeStampMode = false;

    //设置点风格
    void setupScatterStyle(bool enable=false);
    void setupScatterStyle(QCPScatterStyle::ScatterShape shape=QCPScatterStyle::ssNone);
    //设置线风格
    void setupLineStyle(QCPGraph::LineStyle style=QCPGraph::lsLine);
};

#endif // QCUSTOMPLOTCONTROL_H
