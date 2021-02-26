#include "plotter_manager.h"

PlotterManager::PlotterManager(QObject *parent)
: QObject(parent)
{
}

PlotterManager::~PlotterManager()
{
}

int32_t PlotterManager::init()
{
    return 0;
}

int32_t PlotterManager::addPlotter(QString plotterTitle, MyQCustomPlot* plotter)
{
    if(!plotter || plotterTitle.isEmpty())
    {
        return -1;
    }
    plotterMap.insert(plotterTitle, plotter);
    return 0;
}

int32_t PlotterManager::removePlotter(QString plotterTitle)
{
    if(plotterTitle.isEmpty())
    {
        return -1;
    }

    //这个函数只是从plotterMap移除对象不会释放这个对象资源
    MyQCustomPlot* plotter = nullptr;
    plotter = plotterMap.value(plotterTitle);
    if(plotter)
    {
        // plotter->plotControl->resetRightEdge();
        // plotter->plotControl->clearPlotter(-1);
        // plotter->replot();
        clearPlotter(plotter->getPlotterTitle());
        // delete plotter;
        plotterMap.remove(plotterTitle);
        return 0;
    }
    return -1;
}

MyQCustomPlot* PlotterManager::selectPlotter(QString plotterTitle)
{
    if(plotterTitle.isEmpty())
    {
        return nullptr;
    }

    MyQCustomPlot* plotter = nullptr;
    plotter = plotterMap.value(plotterTitle);
    return plotter;
}

int32_t PlotterManager::clearPlotter(QString title)
{
    if(title.isEmpty())
    {
        return -1;
    }

    MyQCustomPlot* plotter = nullptr;
    plotter = plotterMap.value(title);

    if(!plotter)
        return -1;

    plotter->plotControl->resetRightEdge();
    plotter->plotControl->clearPlotter(-1);
    while(plotter->graphCount() > 1)
    {
        plotter->removeGraph(plotter->graphCount() - 1);
    }
    if(plotter->getxAxisSource() == XAxis_Cnt)
    {
        //关闭自动刷新Y轴时不重置Y轴
        if(plotter->getAutoRescaleYAxis())
        {
            plotter->yAxis->setRange(0, 5);
        }
        plotter->xAxis->setRange(0, plotter->plotControl->getXAxisLength(), Qt::AlignRight);
    }
    else
    {
        plotter->yAxis->rescale(true);
        plotter->xAxis->rescale(true);
    }
    return 0;
}


int32_t PlotterManager::clearAllPlotter()
{
    QMap<QString, MyQCustomPlot*>::iterator it;
    for(it = plotterMap.begin(); it != plotterMap.end(); it++)
    {
        clearPlotter(it.key());
    }
    return 0;
}

int32_t PlotterManager::updateAllPlotterFont(QFont font)
{
    MyQCustomPlot* plotter = nullptr;
    QMap<QString, MyQCustomPlot*>::iterator it;
    for(it = plotterMap.begin(); it != plotterMap.end(); it++)
    {
        plotter = nullptr;
        plotter = it.value();
        if(plotter)
            plotter->plotControl->setupFont(font);
    }
    return 0;
}

int32_t PlotterManager::updateAllPlotterBackGround(QColor color)
{
    MyQCustomPlot* plotter = nullptr;
    QMap<QString, MyQCustomPlot*>::iterator it;
    for(it = plotterMap.begin(); it != plotterMap.end(); it++)
    {
        plotter = nullptr;
        plotter = it.value();
        if(plotter)
        {
            plotter->setBackground(color);
            plotter->legend->setBrush(color);
        }
    }
    return 0;
}

QVector<MyQCustomPlot*> PlotterManager::getAllPlotters()
{
    QVector<MyQCustomPlot*> list;
    QMap<QString, MyQCustomPlot*>::iterator it;
    for(it = plotterMap.begin(); it != plotterMap.end(); it++)
    {
        list << it.value();
    }
    return list;
}

int32_t PlotterManager::setDefaultPlotter(MyQCustomPlot* plotter)
{
    if(!plotter)
        return -1;

    //TODO:确认下如果没找到是会进这个逻辑吗？
    if(plotterMap.value(plotter->getPlotterTitle()) == nullptr)
    {
        plotterMap.insert(plotter->getPlotterTitle(), plotter);
    }

    defaultPlotter = plotter;
    defaultPlotterTitle = plotter->getPlotterTitle();
    return 0;
}

int32_t PlotterManager::setDefaultPlotter(QString plotterTitle)
{
    if(plotterTitle.isEmpty())
    {
        return -1;
    }

    MyQCustomPlot* plotter = nullptr;
    plotter = plotterMap.value(plotterTitle);
    if(plotter)
    {
        defaultPlotter = plotter;
        defaultPlotterTitle = plotter->getPlotterTitle();
        return 0;
    }

    return -1;
}

MyQCustomPlot* PlotterManager::getDefaultPlotter()
{
    return defaultPlotter;
}
