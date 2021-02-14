#ifndef PLOTTER_MANAGER_H
#define PLOTTER_MANAGER_H

#include <QObject>
#include <QDebug>
#include <QString>
#include "myqcustomplot.h"
#include <QMap>
#include <QMenu>
#include <QAction>
#include "fft_dialog.h"

/*
 * 本文件对各个绘图器对象进行管理，用于实现多窗口绘图
 */

class PlotterManager : public QObject
{
    Q_OBJECT
public:

public:
    explicit PlotterManager(QObject *parent = nullptr);
    ~PlotterManager();
    int32_t init();
    int32_t addPlotter(QString plotterTitle, MyQCustomPlot* plotter);
    int32_t removePlotter(QString plotterTitle);
    MyQCustomPlot* selectPlotter(QString plotterTitle);
    int32_t clearPlotter(QString title);
    int32_t clearAllPlotter();
    int32_t updateAllPlotterFont(QFont font);
    QVector<MyQCustomPlot*> getAllPlotters();
    int32_t setDefaultPlotter(MyQCustomPlot* plotter);
    int32_t setDefaultPlotter(QString plotterTitle);
    MyQCustomPlot* getDefaultPlotter();
public slots:
private:
    QMap<QString, MyQCustomPlot*> plotterMap;
    MyQCustomPlot* defaultPlotter = nullptr;
    QString        defaultPlotterTitle;
};

#endif // PLOTTER_MANAGER_H
