#include "qcustomplotcontrol.h"

QCustomPlotControl::QCustomPlotControl()
{

    xRange.lower = -200;
    xRange.upper = 0;
    //名字集合
    nameSet << "Graph 1"
            << "Graph 2"
            << "Graph 3"
            << "Graph 4"
            << "Graph 5"
            << "Graph 6"
            << "Graph 7"
            << "Graph 8"
            << "Graph 9"
            << "Graph 10"
            << "Graph 11"
            << "Graph 12"
            << "Graph 13"
            << "Graph 14"
            << "Graph 15";

    //填充颜色，Candy色集
    colorSet << QColor(0xEF, 0x00, 0x00)
             << QColor(0x33, 0x66, 0x99)
             << QColor(0xFE, 0xC2, 0x11)
             << QColor(0x3B, 0xC3, 0x71)
             << QColor(0x66, 0x66, 0x99)
             << QColor(0x99, 0x99, 0x99)
             << QColor(0xFF, 0x66, 0x66)
             << QColor(0x66, 0x99, 0xCC)
             << QColor(0xCC, 0x66, 0x00)
             << QColor(0x00, 0x99, 0x99)
             << QColor(0x6B, 0x67, 0xBC)
             << QColor(0x99, 0x86, 0x7A)
             << QColor(0xCC, 0x33, 0x33)
             << QColor(0x66, 0x99, 0x99)
             << QColor(0xCC, 0x99, 0x00);

    scatterShapeSet << QCPScatterStyle::ssCross     ///< \enumimage{ssCross.png} a cross
                    << QCPScatterStyle::ssPlus      ///< \enumimage{ssPlus.png} a plus
                    << QCPScatterStyle::ssCircle    ///< \enumimage{ssCircle.png} a circle
                    << QCPScatterStyle::ssDisc      ///< \enumimage{ssDisc.png} a circle which is filled with the pen's color (not the brush as with ssCircle)
                    << QCPScatterStyle::ssSquare    ///< \enumimage{ssSquare.png} a square
                    << QCPScatterStyle::ssDiamond   ///< \enumimage{ssDiamond.png} a diamond
                    << QCPScatterStyle::ssStar      ///< \enumimage{ssStar.png} a star with eight arms, i.e. a combination of cross and plus
                    << QCPScatterStyle::ssTriangle  ///< \enumimage{ssTriangle.png} an equilateral triangle, standing on baseline
                    << QCPScatterStyle::ssTriangleInverted ///< \enumimage{ssTriangleInverted.png} an equilateral triangle, standing on corner
                    << QCPScatterStyle::ssCrossSquare      ///< \enumimage{ssCrossSquare.png} a square with a cross inside
                    << QCPScatterStyle::ssPlusSquare       ///< \enumimage{ssPlusSquare.png} a square with a plus inside
                    << QCPScatterStyle::ssCrossCircle      ///< \enumimage{ssCrossCircle.png} a circle with a cross inside
                    << QCPScatterStyle::ssPlusCircle       ///< \enumimage{ssPlusCircle.png} a circle with a plus inside
                    << QCPScatterStyle::ssPeace     ///< \enumimage{ssPeace.png} a circle, with one vertical and two downward diagonal lines
                    << QCPScatterStyle::ssPixmap    ///< a custom pixmap specified by \ref setPixmap, centered on the data point coordinates
                    << QCPScatterStyle::ssCustom;    ///< custom painter operations are performed per scatter (As QPainterPath, see \ref setCustomPath)
}

QCustomPlotControl::QCustomPlotControl(MyQCustomPlot* plot, FFT_Dialog *window)
{
    QCustomPlotControl();
    customPlot = plot;
    fft_dialog = window;
    setupPlotter(plot, window);
}

QCustomPlotControl::~QCustomPlotControl()
{

}

void QCustomPlotControl::setEnableTimeStampMode(bool enable)
{
    enableTimeStampMode = enable;
}

bool QCustomPlotControl::getEnableTimeStampMode()
{
    return enableTimeStampMode;
}

int QCustomPlotControl::getMaxValidGraphNumber()
{
    return colorSet.size();
}

double QCustomPlotControl::getXAxisLength()
{
    return xRange.upper - xRange.lower;
}

bool QCustomPlotControl::setXAxisLength(double length)
{
    if(length < 0)
        return false;
    xRange.upper = 0;
    xRange.lower = -length;
//    customPlot->xAxis->setRange(xAxisCnt, xRange.upper - xRange.lower, Qt::AlignRight);
    return true;
}

QVector<QColor> QCustomPlotControl::getColorSet()
{
    return colorSet;
}

//此函数会改变nameSet要注意
QVector<QString> QCustomPlotControl::getNameSetsFromPlot()
{
    nameSet.clear();
    //从plot控件中读取名字
    for(int i = 0; i < customPlot->graphCount(); i++){
        nameSet << customPlot->graph(i)->name();
    }
    //名字集合应等于颜色集合
    if(nameSet.size() < colorSet.size()){
        for(int i = nameSet.size(); i < colorSet.size(); i++){
            nameSet << "Graph " + QString::number(i+1);
        }
    }
    if(nameSet.size() > colorSet.size()){
        while(nameSet.size() > colorSet.size()){
            nameSet.pop_back();
        }
    }
    return nameSet;
}

void QCustomPlotControl::setNameSet(QVector<QString> names)
{
    int min = (names.size() - colorSet.size()) > 0 ? colorSet.size() : names.size();
    for (int i = 0; i < min ; i++) {
        nameSet[i] = names.at(i);
    }
    for (int i = 0; i < customPlot->graphCount(); i++) {
         customPlot->graph(i)->setName(nameSet[i]);
    }

    if(fft_dialog)
    {
        fft_dialog->setNameSet(nameSet);
    }
}

bool QCustomPlotControl::addGraph(int num)
{
    if(num<1 || num > colorSet.size())
        return false;

    if(customPlot->graphCount() >= colorSet.size())
        return false;

    int min = num > (colorSet.size() - customPlot->graphCount()) ? (colorSet.size() - customPlot->graphCount()) : num;

    for(int i = 0; i < min; i++){
        customPlot->addGraph();
    }

    //设置自适应采样，提高大数据性能
    for(int i = 0; i < customPlot->graphCount(); i++){
        customPlot->graph(i)->setAdaptiveSampling(true);
    }
    //设置画笔
    setupPenWidth();

    //设置线型
    setupLineType(lineType);
    setNameSet(nameSet);
    customPlot->replot();

    return true;
}

/*
 * Function:把数据取出来显示到绘图器上
*/
bool QCustomPlotControl::addDataToPlotter(QVector<double> rowData, qint32 xSource)
{
    QElapsedTimer time;
    time.start();
    if(rowData.size() <= 0 || rowData.size()>colorSet.size())
        return false;

    //判断是否需要添加曲线
    if(rowData.size() > customPlot->graphCount()){
        if(customPlot->graphCount() < colorSet.size()){
            if(!addGraph(rowData.size() - customPlot->graphCount()))
            {
                qDebug() << "addDataToPlotter addGraph failed";
                return false;
            }
        }
    }
    //曲线数据减少时，进行补0填充，否则tracer会由于数据未对齐无法映射准确坐标
    while(rowData.size() < customPlot->graphCount())
    {
        rowData.append(0);
    }

    //填充数据
    int minCnt = colorSet.size()>rowData.size()?rowData.size():colorSet.size();
    switch(xSource)
    {
    case XAxis_Cnt:
        for(int i = 0; i < minCnt; i++){
            customPlot->graph(i)->addData(xAxisCnt, rowData.at(i));
        }
        // make key axis range scroll with the data (at a constant range size of 200):
        customPlot->xAxis->setRange(xAxisCnt, xRange.upper - xRange.lower, Qt::AlignRight);
        xAxisCnt++;
        break;
    default:
        for(int i = 0; i < minCnt; i++){
            customPlot->graph(i)->addData(rowData.at(xSource - 1), rowData.at(i)); //注意 xSource - 1是因为xSource=1时表示选择第0条曲线，xSource = 0 时表示时间
        }
        if(enableTimeStampMode)
        {
            customPlot->xAxis->setRange(rowData.at(xSource - 1), xRange.upper - xRange.lower, Qt::AlignRight);
        }
        break;
    }

    //Graph1的动态标签
//    double graph1Value = customPlot->graph(0)->dataMainValue(customPlot->graph(0)->dataCount()-1);
//    mTag1->updatePosition(graph1Value);
//    mTag1->setText(QString::number(graph1Value, 'f', 2));


    //刷新操作和调Y轴在数据量大时都很耗时，不要加一个数据就刷新一次

//    qDebug()<<"displayToPlotter "<<time.elapsed();
    return true;
}

void QCustomPlotControl::clearPlotter(int index)
{
    if(index > customPlot->graphCount()-1)
        return;

    xAxisCnt = 0;
    QVector<double> empty;
    if(index == -1){
        for(int i = 0; i < customPlot->graphCount(); i++){
            customPlot->graph(i)->setData(empty, empty);
        }
    }else{
        customPlot->graph(index)->setData(empty, empty);
    }
    customPlot->replot();
}

void QCustomPlotControl::setupFont(QFont font)
{
    customPlot->legend->setFont(font);
    customPlot->legend->setSelectedFont(font);
    customPlot->xAxis->setTickLabelFont(font);
    customPlot->xAxis->setSelectedTickLabelFont(font);
    customPlot->xAxis->setSelectedLabelFont(font);
    customPlot->xAxis->setLabelFont(font);
    customPlot->yAxis->setTickLabelFont(font);
    customPlot->yAxis->setSelectedTickLabelFont(font);
    customPlot->yAxis->setSelectedLabelFont(font);
    customPlot->yAxis->setLabelFont(font);
    customPlot->yAxis2->setTickLabelFont(font);
    customPlot->yAxis2->setSelectedTickLabelFont(font);
    customPlot->yAxis2->setSelectedLabelFont(font);
    customPlot->yAxis2->setLabelFont(font);
    customPlot->m_Tracer->setLabelFont(font);

    customPlot->replot();
}

void QCustomPlotControl::setupOpenGL(bool enabled)
{
    customPlot->setOpenGl(enabled);
}

void QCustomPlotControl::setupPenWidth(double width)
{
    for(int i = 0; i < customPlot->graphCount(); i++){
        //设置画笔
        QPen pen;
        pen.setColor(colorSet.at(i));
        pen.setWidthF(width);
        customPlot->graph(i)->setPen(pen);
    }
    customPlot->replot();
}

void QCustomPlotControl::setupAxisTick(int axisTickLenth, int axisSubTickLenth)
{
    customPlot->xAxis->setTickLengthIn(axisTickLenth);
    customPlot->yAxis->setTickLengthIn(axisTickLenth);
    customPlot->xAxis->setSubTickLengthIn(axisSubTickLenth);
    customPlot->yAxis->setSubTickLengthIn(axisSubTickLenth);

    customPlot->xAxis2->setTickLengthIn(axisTickLenth);
    customPlot->xAxis2->setSubTickLengthIn(axisSubTickLenth);
    customPlot->yAxis2->setTickLengthIn(axisTickLenth);
    customPlot->yAxis2->setSubTickLengthIn(axisSubTickLenth);
}

void QCustomPlotControl::setupAxisLabel(QString xLabel, QString yLabel)
{
    customPlot->xAxis->setLabel(xLabel);
    customPlot->yAxis->setLabel(yLabel);
}

bool QCustomPlotControl::setupPlotName(QVector<QString> nameStr)
{
    if(nameStr.size() <= 0)
        return false;
    QString name;
    int size;
    size = nameStr.size()>customPlot->graphCount() ? customPlot->graphCount() : nameStr.size();
    for(int i = 0; i < size; i++){
        customPlot->graph(i)->setName(nameStr.at(0));
        nameStr.pop_front();
    }
    return true;
}

void QCustomPlotControl::setupScatterStyle(bool enable)
{
    if(customPlot->selectedGraphs().size() > 0)
    {
        //获取图像编号
        int index = 0;
        for(;index < customPlot->graphCount(); index++){
            if(customPlot->graph(index)->name() == customPlot->selectedGraphs().first()->name()){
                break;
            }
        }
        if(enable)
        {
            customPlot->selectedGraphs().first()->setScatterStyle(QCPScatterStyle(scatterShapeSet.at(index), 4));
        }else{
            customPlot->selectedGraphs().first()->setScatterStyle(QCPScatterStyle::ssNone);
        }
        customPlot->replot();
        return;
    }
    if(enable){
        for(int i = 0; i < customPlot->graphCount(); i++){
            customPlot->graph(i)->setScatterStyle(QCPScatterStyle(scatterShapeSet.at(i), 4));
        }
    }else{
        for(int i = 0; i < customPlot->graphCount(); i++){
            customPlot->graph(i)->setScatterStyle(QCPScatterStyle::ssNone);
        }
    }
    customPlot->replot();
}

//not used yet
void QCustomPlotControl::setupScatterStyle(QCPScatterStyle::ScatterShape shape)
{
    if(shape != QCPScatterStyle::ssNone){
        for(int i = 0; i < customPlot->graphCount(); i++){
            customPlot->graph(i)->setScatterStyle(QCPScatterStyle(shape,4));
        }
    }else{
        for(int i = 0; i < customPlot->graphCount(); i++){
            customPlot->graph(i)->setScatterStyle(shape);
        }
    }
    customPlot->replot();
}

void QCustomPlotControl::setupLineStyle(QCPGraph::LineStyle style)
{
    if(customPlot->selectedGraphs().size() > 0)
    {
        customPlot->selectedGraphs().first()->setLineStyle(style);
        customPlot->replot();
        return;
    }
    for(int i = 0; i < customPlot->graphCount(); i++){
        customPlot->graph(i)->setLineStyle(style);
    }
    customPlot->replot();
}


void QCustomPlotControl::setupLineType(LineType_e type)
{
    switch(type){
        case Line:
            setupLineStyle(QCPGraph::lsLine);
            setupScatterStyle(false);
            lineType = Line;
            break;
        case ScatterLine:
            setupLineStyle(QCPGraph::lsLine);
            setupScatterStyle(true);
            lineType = ScatterLine;
            break;
        case Scatter:
            setupLineStyle(QCPGraph::lsNone);
            setupScatterStyle(true);
            lineType = Scatter;
            break;
    }

    customPlot->replot();
}

void QCustomPlotControl::setupLegendVisible(bool visible)
{
    if(visible){
        customPlot->legend->setVisible(true);
    }else{
        customPlot->legend->setVisible(false);
    }
}

void QCustomPlotControl::setupGraphVisible(bool visible)
{
    if(visible){
        customPlot->legend->setVisible(true);
    }else{
        customPlot->legend->setVisible(false);
    }
}

void QCustomPlotControl::setupInteractions()
{
    //设置交互功能：范围可拖拽、范围可缩放、轴可选择、图例可选择、绘图区可选择
    customPlot->setInteractions(QCP::iRangeDrag |
                                QCP::iRangeZoom | //缩放自己实现
                                QCP::iSelectAxes |
                                QCP::iSelectLegend |
                                QCP::iSelectPlottables);
    //设置仅图例框中的条目可被选择
    customPlot->legend->setSelectableParts(QCPLegend::spItems);

    //与槽相关的许多函数放在了MainWindow里
    //...
}

//修改自源码的setupFullAxesBox
void QCustomPlotControl::setupAxesBox(bool connectRanges)
{
  QCPAxis *xAxis, *yAxis, *xAxis2, *yAxis2;
  if (customPlot->axisRect()->axisCount(QCPAxis::atBottom) == 0)
    xAxis = customPlot->axisRect()->addAxis(QCPAxis::atBottom);
  else
    xAxis = customPlot->axisRect()->axis(QCPAxis::atBottom);

  if (customPlot->axisRect()->axisCount(QCPAxis::atLeft) == 0)
    yAxis = customPlot->axisRect()->addAxis(QCPAxis::atLeft);
  else
    yAxis = customPlot->axisRect()->axis(QCPAxis::atLeft);

  if (customPlot->axisRect()->axisCount(QCPAxis::atTop) == 0)
    xAxis2 = customPlot->axisRect()->addAxis(QCPAxis::atTop);
  else
    xAxis2 = customPlot->axisRect()->axis(QCPAxis::atTop);

  if (customPlot->axisRect()->axisCount(QCPAxis::atRight) == 0)
    yAxis2 = customPlot->axisRect()->addAxis(QCPAxis::atRight);
  else
    yAxis2 = customPlot->axisRect()->axis(QCPAxis::atRight);

  xAxis->setVisible(true);
  yAxis->setVisible(true);
  xAxis2->setVisible(true);
  yAxis2->setVisible(true);
  xAxis2->setTickLabels(false);
  yAxis2->setTickLabels(true);

  xAxis2->setRange(xAxis->range());
  xAxis2->setRangeReversed(xAxis->rangeReversed());
  xAxis2->setScaleType(xAxis->scaleType());
  xAxis2->setTicks(xAxis->ticks());
  xAxis2->setNumberFormat(xAxis->numberFormat());
  xAxis2->setNumberPrecision(xAxis->numberPrecision());
  xAxis2->ticker()->setTickCount(xAxis->ticker()->tickCount());
  xAxis2->ticker()->setTickOrigin(xAxis->ticker()->tickOrigin());

  yAxis2->setRange(yAxis->range());
  yAxis2->setRangeReversed(yAxis->rangeReversed());
  yAxis2->setScaleType(yAxis->scaleType());
  yAxis2->setTicks(yAxis->ticks());
  yAxis2->setNumberFormat(yAxis->numberFormat());
  yAxis2->setNumberPrecision(yAxis->numberPrecision());
  yAxis2->ticker()->setTickCount(yAxis->ticker()->tickCount());
  yAxis2->ticker()->setTickOrigin(yAxis->ticker()->tickOrigin());

  if (connectRanges)
  {
    QObject::connect(xAxis, SIGNAL(rangeChanged(QCPRange)), xAxis2, SLOT(setRange(QCPRange)));
    QObject::connect(yAxis, SIGNAL(rangeChanged(QCPRange)), yAxis2, SLOT(setRange(QCPRange)));
  }
}

void QCustomPlotControl::setupPlotter(MyQCustomPlot* plot, FFT_Dialog *window)
{
    //设置一个指针直接控制customPlot解决槽函数要访问ui类下的customPlot的问题
    customPlot = plot;
    fft_dialog = window;

    //添加一条示例曲线。更多曲线的添加动态完成。
    if(customPlot->graphCount()==0){
        customPlot->addGraph();
        customPlot->graph(0)->setPen(QPen(colorSet.at(0)));
        // 动态标签
//        mTag1 = new AxisTag(customPlot->graph(customPlot->graphCount()-1)->valueAxis());
//        mTag1->setPen(customPlot->graph(customPlot->graphCount()-1)->pen());
//        mTag1->setText("0");
    }

    //设置自适应采样，提高大数据性能
    for(int i = 0; i < customPlot->graphCount(); i++){
        customPlot->graph(0)->setAdaptiveSampling(true);
    }
    //设置OpenGL
    setupOpenGL(false);
    //设置画笔
    setupPenWidth();
    //设置轴间隔
    setupAxisTick();
    //设置轴标签
    setupAxisLabel();
    //设置曲线名称
    setupPlotName();
    //设置线型
    setupLineType(lineType);
    //设置图例可见性
    setupLegendVisible(true);
    //设置图例位置
    customPlot->axisRect()->insetLayout()->setInsetAlignment(0,Qt::AlignLeft|Qt::AlignTop);
    //设置交互功能
    setupInteractions();

    //设置轴
    setupAxesBox();
    //设置X轴范围
    customPlot->xAxis->setRange(xRange);
    //设置曲线名称
    setNameSet(nameSet);
    //设置间隔给tag留出显示位置
    customPlot->axisRect()->axis(QCPAxis::atRight, 0)->setPadding(40);

    // make left and bottom axes transfer their ranges to right and top axes:
    QObject::connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
    QObject::connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));

    customPlot->replot();
}
