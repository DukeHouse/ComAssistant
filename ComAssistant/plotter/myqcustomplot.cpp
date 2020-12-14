#include "myqcustomplot.h"

MyQCustomPlot::MyQCustomPlot(QWidget* parent)
    :QCustomPlot(parent)
{
    // connect slot that ties some axis selections together (especially opposite axes):
    connect(this, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChanged()));
    // connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
    connect(this, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress(QMouseEvent*)));
    connect(this, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel(QWheelEvent*)));

    // make bottom and left axes transfer their ranges to top and right axes:
    connect(this->xAxis, SIGNAL(rangeChanged(QCPRange)), this->xAxis2, SLOT(setRange(QCPRange)));
    connect(this->yAxis, SIGNAL(rangeChanged(QCPRange)), this->yAxis2, SLOT(setRange(QCPRange)));

    // connect some interaction slots:
    connect(this, SIGNAL(axisDoubleClick(QCPAxis*,QCPAxis::SelectablePart,QMouseEvent*)), this, SLOT(axisLabelDoubleClick(QCPAxis*,QCPAxis::SelectablePart)));
    connect(this, SIGNAL(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*,QMouseEvent*)), this, SLOT(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*)));

    connect(this, SIGNAL(plottableClick(QCPAbstractPlottable*,int,QMouseEvent*)), this, SLOT(graphClicked(QCPAbstractPlottable*,int)));

    // setup policy and connect slot for context menu popup:
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));
    // 坐标跟随
    connect(this, SIGNAL(mouseMove(QMouseEvent*)), this,SLOT(showTracer(QMouseEvent*)));

    m_Tracer = new MyTracer(this, this->graph(), TracerType::DataTracer);

}

MyQCustomPlot::~MyQCustomPlot()
{
    protocol_thread->quit();
    protocol_thread->wait();
//    delete protocol_thread; //deleteLater自动删除？
    delete protocol_thread;

    delete m_Tracer;
    delete plotControl;
//    delete protocol;
}

void MyQCustomPlot::init(QMenu* plotterSetting, QAction* saveGraphData, QAction* saveGraphPicture,
                         qint32 *xSource, bool *autoRescaleYAxisFlag, FFT_Dialog* window)
{
    setting = plotterSetting;
    saveData = saveGraphData;
    savePicture = saveGraphPicture;
    plotControl = new QCustomPlotControl;

    xAxisSource = xSource;
    autoRescaleYAxis = autoRescaleYAxisFlag;
    fft_dialog = window;
    plotControl->setupPlotter(this, window);

    protocol = new DataProtocol();
    protocol_thread = new QThread(this);
    protocol->moveToThread(protocol_thread);
    qRegisterMetaType<qint32>("qint32&");
    connect(protocol_thread, SIGNAL(finished()), protocol, SLOT(deleteLater()));
    connect(this, SIGNAL(appendData(const QByteArray &)),
            protocol, SLOT(appendData(const QByteArray &)));
    connect(this, SIGNAL(parseData(bool)),
            protocol, SLOT(parseData(bool)));

    protocol_thread->start();
}

void MyQCustomPlot::appendDataWaitToParse(const QByteArray &data)
{
    emit appendData(data);
}

void MyQCustomPlot::startParse(bool enableSumCheck)
{
    emit parseData(enableSumCheck);
}

/*plotter交互*/
void MyQCustomPlot::axisLabelDoubleClick(QCPAxis *axis, QCPAxis::SelectablePart part)
{
  // Set an axis label by double clicking on it
  if (part == QCPAxis::spAxisLabel) // only react when the actual axis label is clicked, not tick label or axis backbone
  {
      bool ok;
      QString newLabel = QInputDialog::getText(this,
                                               tr("更改轴标签"), tr("新的轴标签："),
                                               QLineEdit::Normal, axis->label(),
                                               &ok, Qt::WindowCloseButtonHint);
      if (ok)
      {
        axis->setLabel(newLabel);
        this->replot();
      }
  }else if(part == QCPAxis::spAxis){
      //选的Y轴
      if(axis == this->yAxis || axis == this->yAxis2){
          if(autoRescaleYAxis && *autoRescaleYAxis == true)
          {
              QMessageBox::information(this, tr("提示"), tr("请先关闭自动刷新Y轴功能"));
              return;
          }
          bool ok;
          double lower, upper;
          lower = QInputDialog::getDouble(this,
                                          tr("更改Y轴范围"), tr("新的Y轴下边界："),
                                          this->yAxis->range().lower,
                                          -2147483647, 2147483647, 1,
                                          &ok, Qt::WindowCloseButtonHint);
          if (ok)
          {
              this->yAxis->setRangeLower(lower);
              this->yAxis2->setRangeLower(lower);
              upper = QInputDialog::getDouble(this,
                                              tr("更改Y轴范围"), tr("新的Y轴上边界："),
                                              this->yAxis->range().upper,
                                              -2147483647, 2147483647, 1,
                                              &ok, Qt::WindowCloseButtonHint);
              if (ok)
              {
                  this->yAxis->setRangeUpper(upper);
                  this->yAxis2->setRangeUpper(upper);
              }
              this->replot();
          }
         return;
      }
      //选的X轴
      if(!xAxisSource)
      {
          qDebug() << "unregistered xAxisSource in axisLabelDoubleClick";
          return;
      }
      //X轴在时间戳模式和非时间戳模式下具有不同的调整方式
      if(*xAxisSource == XAxis_Cnt || plotControl->getEnableTimeStampMode())
      {
          bool ok;
          double newLength = QInputDialog::getDouble(this, tr("更改X轴长度"), tr("新的X轴长度："),plotControl->getXAxisLength(),
                                                     0, 2147483647, 1, &ok, Qt::WindowCloseButtonHint);
          if (ok)
          {
            plotControl->setXAxisLength(newLength);
            this->replot();
          }
      }
      else//不是时间戳模式且数据源变了
      {
          bool ok;
          double lower, upper;
          lower = QInputDialog::getDouble(this,
                                          tr("更改X轴范围"), tr("新的X轴左边界："),
                                          this->xAxis->range().lower,
                                          -2147483647, 2147483647, 1,
                                          &ok, Qt::WindowCloseButtonHint);
          if (ok)
          {
              this->xAxis->setRangeLower(lower);
              this->xAxis2->setRangeLower(lower);
              upper = QInputDialog::getDouble(this,
                                              tr("更改X轴范围"), tr("新的X轴右边界："),
                                              this->xAxis->range().upper,
                                              -2147483647, 2147483647, 1,
                                              &ok, Qt::WindowCloseButtonHint);
              if (ok)
              {
                  this->xAxis->setRangeUpper(upper);
                  this->xAxis2->setRangeUpper(upper);
              }
              this->replot();
          }
      }
  }
}

void MyQCustomPlot::legendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item)
{
  // Rename a graph by double clicking on its legend item
  Q_UNUSED(legend)
  if (item) // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
  {
    QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
    bool ok;
    QString newName = QInputDialog::getText(this, tr("更改曲线名称"), tr("新的曲线名称"),
                                            QLineEdit::Normal, plItem->plottable()->name(), &ok, Qt::WindowCloseButtonHint);
    if (ok)
    {
      plItem->plottable()->setName(newName);
      plotControl->setNameSet(plotControl->getNameSetsFromPlot());
      this->replot();

      //把新名字传递给fft窗口
      if(fft_dialog)
      {
          fft_dialog->setNameSet(this->plotControl->getNameSetsFromPlot());
      }
    }
  }
}

void MyQCustomPlot::selectionChanged()
{
  /*
   normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
   the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
   and the axis base line together. However, the axis label shall be selectable individually.

   The selection state of the left and right axes shall be synchronized as well as the state of the
   bottom and top axes.

   Further, we want to synchronize the selection of the graphs with the selection state of the respective
   legend item belonging to that graph. So the user can select a graph by either clicking on the graph itself
   or on its legend item.
  */

  // make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
  if (this->xAxis->selectedParts().testFlag(QCPAxis::spAxis) || this->xAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
      this->xAxis2->selectedParts().testFlag(QCPAxis::spAxis) || this->xAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    this->xAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    this->xAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }
  // make left and right axes be selected synchronously, and handle axis and tick labels as one selectable object:
  if (this->yAxis->selectedParts().testFlag(QCPAxis::spAxis) || this->yAxis->selectedParts().testFlag(QCPAxis::spTickLabels) ||
      this->yAxis2->selectedParts().testFlag(QCPAxis::spAxis) || this->yAxis2->selectedParts().testFlag(QCPAxis::spTickLabels))
  {
    this->yAxis2->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
    this->yAxis->setSelectedParts(QCPAxis::spAxis|QCPAxis::spTickLabels);
  }

  // synchronize selection of graphs with selection of corresponding legend items:
  for (int i=0; i<this->graphCount(); ++i)
  {
    QCPGraph *graph = this->graph(i);
    QCPPlottableLegendItem *item = this->legend->itemWithPlottable(graph);
    if (item->selected() || graph->selected())
    {
      item->setSelected(true);
      graph->setSelection(QCPDataSelection(graph->data()->dataRange()));
    }
  }
}

void MyQCustomPlot::mousePress(QMouseEvent *m)
{
    // if an axis is selected, only allow the direction of that axis to be dragged
    // if no axis is selected, both directions may be dragged
    Q_UNUSED(m)

    if (this->xAxis->selectedParts().testFlag(QCPAxis::spAxis))
    this->axisRect()->setRangeDrag(this->xAxis->orientation());
    else if (this->yAxis->selectedParts().testFlag(QCPAxis::spAxis))
    this->axisRect()->setRangeDrag(this->yAxis->orientation());
    else
    this->axisRect()->setRangeDrag(Qt::Horizontal|Qt::Vertical);
}

void MyQCustomPlot::mouseWheel(QWheelEvent *w)
{
    // if an axis is selected, only allow the direction of that axis to be zoomed
    // if no axis is selected, both directions may be zoomed
    if (this->xAxis->selectedParts().testFlag(QCPAxis::spAxis) ||
        this->xAxis->selectTest(w->pos(), true)  != -1 ||
        this->xAxis2->selectTest(w->pos(), true) != -1)
    {
        this->axisRect()->setRangeZoom(this->xAxis->orientation());
        plotControl->setXAxisLength(this->xAxis->range().upper - this->xAxis->range().lower);
    }
    else if (this->yAxis->selectedParts().testFlag(QCPAxis::spAxis) ||
             this->yAxis->selectTest(w->pos(), true) != -1 ||
             this->yAxis2->selectTest(w->pos(), true) != -1 )
    {
        this->axisRect()->setRangeZoom(this->yAxis->orientation());
    }
    else{
        switch(key)
        {
        case Qt::Key_X:
            //只调X轴
            this->axisRect()->setRangeZoom(Qt::Horizontal);
            break;
        case Qt::Key_V:
            //只调Y轴
            this->axisRect()->setRangeZoom(Qt::Vertical);
            break;
        case Qt::Key_Control:
            //调XY轴
            this->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
            break;
        default:
            if(autoRescaleYAxis && *autoRescaleYAxis == true)
            {
                //调X轴
                this->axisRect()->setRangeZoom(Qt::Horizontal);
            }
            else
            {
                //调XY轴
                this->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
            }
            //记忆X轴长度
            plotControl->setXAxisLength(this->xAxis->range().upper - this->xAxis->range().lower);
            break;
        }
//        qDebug()<<"wheel key"<<key<<"w->delta()"<<w->delta();
    }
}

void MyQCustomPlot::keyPressEvent(QKeyEvent *e)
{
//    qDebug()<<"keyPressEvent"<<e->key();
    key = e->key();
}

void MyQCustomPlot::keyReleaseEvent(QKeyEvent *e)
{
//    qDebug()<<"keyReleaseEvent"<<e->key();
    if(key == e->key())
    {
        key = 0;
    }
}

void MyQCustomPlot::recvKey(QKeyEvent *e, bool isPressAct)
{
    if(isPressAct){
        keyPressEvent(e);
        return;
    }
    keyReleaseEvent(e);
}

void MyQCustomPlot::removeSelectedGraph()
{
    QMessageBox::Button res;
    res = QMessageBox::warning(this, tr("警告"), tr("确定要移除所选曲线吗？"), QMessageBox::Ok|QMessageBox::No);
    if(res == QMessageBox::No)
        return;

    if (this->selectedGraphs().size() > 0)
    {
        this->removeGraph(this->selectedGraphs().first());
        this->replot();
    }
}

void MyQCustomPlot::rescaleXYAxis()
{
    this->yAxis->rescale(true);
    //XY模式
    if(xAxisSource && (*xAxisSource != XAxis_Cnt) &&
       this->plotControl->getEnableTimeStampMode() != true)
    {
        this->xAxis->rescale(true);
    }
    else    //YT模式
    {
        QCPRange range = this->plotControl->getXRange();
        int64_t rightEdge = this->plotControl->getRightEdge();
        //YT模式时间没有负数
        if(rightEdge > 0)
        {
            this->xAxis->setRange(rightEdge, range.upper - range.lower, Qt::AlignRight);
        }
    }
    this->replot();
}

void MyQCustomPlot::removeAllGraphs()
{
    QMessageBox::Button res;
    res = QMessageBox::warning(this,tr("警告"), tr("确定要移除所有曲线吗？"), QMessageBox::Ok|QMessageBox::No);
    if(res == QMessageBox::No)
        return;

    protocol->clearBuff();
    plotControl->clearPlotter(-1);
    while(this->graphCount()>1){
        this->removeGraph(this->graphCount() - 1);
    }
    this->yAxis->setRange(0, 5);
    this->xAxis->setRange(0, plotControl->getXAxisLength(), Qt::AlignRight);
    this->replot();
}

void MyQCustomPlot::reNameSelectedGraph()
{
    for (int i = 0; i < this->graphCount(); ++i)
    {
        QCPGraph *graph = this->graph(i);
        QCPPlottableLegendItem *item = this->legend->itemWithPlottable(graph);
        if (item->selected() || graph->selected())
        {
            legendDoubleClick(nullptr, qobject_cast<QCPAbstractLegendItem*>(item));
        }
    }
}

void MyQCustomPlot::reColorSelectedGraph()
{
    QColor oldColor;
    QVector<QColor> colorSet = plotControl->getColorSet();
    for (int i = 0; i < this->graphCount(); ++i)
    {
        if (this->graph(i)->selected())
        {
            if(i >= colorSet.size())
            {
                qDebug() << "index is larger than color set size";
                return;
            }

            oldColor = colorSet.at(i);
            QColor newColor = QColorDialog::getColor(oldColor, this);
            if(!newColor.isValid())
            {
                qDebug() << "invalid color";
                return;
            }
            colorSet.replace(i, newColor);
            plotControl->setColorSet(colorSet);

            QPen pen;
            pen.setColor(newColor);
            pen.setWidthF(this->graph(i)->pen().widthF());
            this->graph(i)->setPen(pen);
        }
    }
}

void MyQCustomPlot::hideSelectedGraph()
{
    if (this->selectedGraphs().size() > 0)
    {
        //获取图像编号
        int index = 0;
        for(;index < this->graphCount(); index++){
            if(this->graph(index)->name() == this->selectedGraphs().first()->name()){
                break;
            }
        }
        //可见性控制
        if(this->selectedGraphs().first()->visible()){
            this->selectedGraphs().first()->setVisible(false);
            this->legend->item(index)->setTextColor(Qt::gray);
        }
        else{
            this->selectedGraphs().first()->setVisible(true);
            this->legend->item(index)->setTextColor(Qt::black);
        }
        this->replot();
    }
}

void MyQCustomPlot::hideAllGraph()
{
    int index = 0;
    for(;index < this->graphCount(); index++){
        this->graph(index)->setVisible(false);
        this->legend->item(index)->setTextColor(Qt::gray);
    }
    this->replot();
}

void MyQCustomPlot::showAllGraph()
{
    int index = 0;
    for(;index < this->graphCount(); index++){
        this->graph(index)->setVisible(true);
        this->legend->item(index)->setTextColor(Qt::black);
    }
    this->replot();
}

bool MyQCustomPlot::isAllGraphHide(void)
{
    int index = 0;
    for(;index < this->graphCount(); index++){
        if(this->graph(index)->visible()){
            return false;
        }
    }
    return true;
}

void MyQCustomPlot::contextMenuRequest(QPoint pos)
{
  QMenu *menu = new QMenu(this);
  menu->setAttribute(Qt::WA_DeleteOnClose);

  if (this->legend->selectTest(pos, false) >= 0) // context menu on legend requested
  {
    menu->addAction(tr("移动到左上角"), this, SLOT(moveLegend()))->setData(static_cast<int>(Qt::AlignTop|Qt::AlignLeft));
    menu->addAction(tr("移动到右上角"), this, SLOT(moveLegend()))->setData(static_cast<int>(Qt::AlignTop|Qt::AlignRight));
    menu->addAction(tr("移动到右下角"), this, SLOT(moveLegend()))->setData(static_cast<int>(Qt::AlignBottom|Qt::AlignRight));
    menu->addAction(tr("移动到左下角"), this, SLOT(moveLegend()))->setData(static_cast<int>(Qt::AlignBottom|Qt::AlignLeft));
  }
  // general context menu on graphs requested
  if (this->graphCount() > 0){
      menu->addSeparator();
      menu->addAction(tr("寻找曲线"), this, SLOT(rescaleXYAxis()));
      menu->addSeparator();
      if(setting)
      {
          menu->addMenu(setting);
      }
      menu->addSeparator();
      if(saveData)
      {
          menu->addAction(saveData);
      }
      if(savePicture)
      {
          menu->addAction(savePicture);
      }
      menu->addSeparator();
      if (this->selectedGraphs().size() > 0){
          menu->addAction(tr("移除所选曲线"), this, SLOT(removeSelectedGraph()));
      }
      menu->addAction(tr("移除所有曲线"), this, SLOT(removeAllGraphs()));
  }
  //选择了曲线
  if (this->selectedGraphs().size() > 0){
    menu->addSeparator();
    menu->addAction(tr("更改所选曲线颜色"), this, SLOT(reColorSelectedGraph()));
    menu->addAction(tr("重命名所选曲线"), this, SLOT(reNameSelectedGraph()));
    menu->addSeparator();
    //所选曲线是否可见
    if(this->selectedGraphs().first()->visible()){
        menu->addAction(tr("隐藏所选曲线"), this, SLOT(hideSelectedGraph()));
    }else{
        menu->addAction(tr("显示所选曲线"), this, SLOT(hideSelectedGraph()));
    }
  }
  if(isAllGraphHide())
  {
      menu->addAction(tr("显示所有曲线"), this, SLOT(showAllGraph()));
  }else
  {
      menu->addAction(tr("隐藏所有曲线"), this, SLOT(hideAllGraph()));
  }
  menu->popup(this->mapToGlobal(pos));
}

void MyQCustomPlot::moveLegend()
{
  if (QAction* contextAction = qobject_cast<QAction*>(sender())) // make sure this slot is really called by a context menu action, so it carries the data we need
  {
    bool ok;
    int dataInt = contextAction->data().toInt(&ok);
    if (ok)
    {
      this->axisRect()->insetLayout()->setInsetAlignment(0, static_cast<Qt::Alignment>(dataInt));
      this->replot();
    }
  }
}

void MyQCustomPlot::graphClicked(QCPAbstractPlottable *plottable, int dataIndex)
{
  // since we know we only have QCPGraphs in the plot, we can immediately access interface1D()
  // usually it's better to first check whether interface1D() returns non-zero, and only then use it.
    Q_UNUSED(plottable)
    Q_UNUSED(dataIndex)
//  double dataValue = plottable->interface1D()->dataMainValue(dataIndex);
//  QString message = QString("Clicked on graph '%1' at data point #%2 with value %3.").arg(plottable->name()).arg(dataIndex).arg(dataValue);
//  qDebug()<<message;
}

void MyQCustomPlot::showTracer(QMouseEvent *event)
{
    if(this->selectedGraphs().size() <= 0){
        if(m_Tracer->getVisible())
        {
            m_Tracer->setVisible(false);
            this->replot();
        }
        return;
    }
    m_Tracer->setVisible(true);

    //获取容器
    QSharedPointer<QCPGraphDataContainer> tmpContainer;
    tmpContainer = this->selectedGraphs().first()->data();

    //获取x,y轴坐标
    double x = 0;
    double y = 0;
    if(xAxisSource && (*xAxisSource != XAxis_Cnt))
    {
        x = this->xAxis->pixelToCoord(event->pos().x());
        y = this->yAxis->pixelToCoord(event->pos().y());
    }else
    {
        x = this->xAxis->pixelToCoord(event->pos().x());
        x = static_cast<int>(x + 0.5);// 四舍五入取整
        if(x < 0)
        {
            y = 0;
        }
        else
        {
            int32_t offset = static_cast<int>(x - tmpContainer->constBegin()->mainKey() + 0.5);
            if(offset < 0 || offset > tmpContainer->size())
            {
                y = 0;
            }
            else
            {
                y = (tmpContainer->constBegin() + offset)->mainValue();
            }
        }
    }

    //范围约束
    QCPRange xRange = this->axisRect()->axis(QCPAxis::atBottom, 0)->range();
    QCPRange yRange = this->axisRect()->axis(QCPAxis::atLeft, 0)->range();
    if(x > xRange.upper)
        x = xRange.upper;
    if(x < xRange.lower)
        x = xRange.lower;
    if(y > yRange.upper)
        y = yRange.upper;
    if(y < yRange.lower)
        y = yRange.lower;

    //更新Tracer
    QString text = "X:" + QString::number(x, 'g', 6) + " Y:" + QString::number(y, 'g', 6);
    m_Tracer->updatePosition(x, y);
    m_Tracer->setText(text);

    this->replot();
}

/*
 * Function:保存图像为文本格式，可以选择不同的分隔符，csv文件可以用,
*/
bool MyQCustomPlot::saveGraphAsTxt(const QString& filePath, char separate)
{
    //列表头和值
    double value;
    QString txtBuff;
    QSharedPointer<QCPGraphDataContainer> tmpContainer;

    //构造表头
    for(int j = 0; j < this->graphCount(); j++){
        txtBuff += this->graph(j)->name() + separate;
    }
    txtBuff += "\n";

    //构造数据行
    int dataLen = this->graph(0)->data()->size();
    if(dataLen == 0)
    {
        qDebug() << __FUNCTION__ << "failed due to empty data";
        return false;
    }
    for(int i = 0; i < dataLen; i++){
        for(int j = 0; j < this->graphCount(); j++){
            tmpContainer = this->graph(j)->data();
            value = (tmpContainer->constBegin()+i)->mainValue();
            txtBuff += QString::number(value,'f') + separate;
        }
        txtBuff += "\n";
    }

    QFile file(filePath);
    if(!file.open(QFile::WriteOnly|QFile::Text))
        return false;
    file.write(txtBuff.toLocal8Bit());
    file.flush();
    file.close();
    return true;
}
