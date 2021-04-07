#include "fft_dialog.h"
#include "ui_fft_dialog.h"

FFT_Dialog::FFT_Dialog(QAction *checkHandler, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FFT_Dialog)
{
    ui->setupUi(this);

    __checkHandler = checkHandler;
    //文件对话框路径
    lastFileDialogPath = Config::getLastFileDialogPath();

    ui->cal_point->setValidator(new QIntValidator(0,9999999,this));
    ui->sample_frq->setValidator(new QIntValidator(0,9999999,this));
    fft_cal_point = ui->cal_point->currentText().toInt();
    fft_sample_frq = ui->sample_frq->currentText().toInt();
    plotter_init();
    last_fft_cal_point = fft_cal_point;
    fft_cal_point_changed_tips = fft_cal_point;
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

    fft_thread = new QThread(this);
    fft = new fft_trans();
    fft->init_size(fft_cal_point);
    fft->moveToThread(fft_thread);
    qRegisterMetaType<QVector<double>>("QVector<double>");
    connect(fft_thread, SIGNAL(finished()), fft, SLOT(deleteLater()));
    connect(this, SIGNAL(start_fft_cal(qint8, qint32, const QVector<double> &)), fft, SLOT(fft_calculate(qint8, qint32, const QVector<double> &)));
    connect(fft, SIGNAL(fft_result(qint8, const QVector<double> &, const QVector<double> &)), this, SLOT(get_fft_result(qint8, const QVector<double> &, const QVector<double> &)));
    fft_thread->start();

    __100msTimer = new QTimer(this);
    connect(__100msTimer, SIGNAL(timeout()), this, SLOT(__100msTimerSlot()));
    __100msTimer->setTimerType(Qt::PreciseTimer);
    __100msTimer->start(100);
}

FFT_Dialog::~FFT_Dialog()
{
    __100msTimer->stop();
    delete __100msTimer;

    fft_thread->quit();
    fft_thread->wait();
//    delete fft; //deleteLater自动删除？
    delete fft_thread;

    delete ui;
}

void FFT_Dialog::setColorSet(const QVector<QColor> &color_set)
{
    //限制一下数量
    if(color_set.size() != 15)
        return;
    colorSet = color_set;
}

void FFT_Dialog::setNameSet(const QVector<QString> &name_set)
{
    if(name_set.size() != 15)
        return;
    nameSet = name_set;
    for (int i = 0; i < fft_layer.size(); i++) {
         fft_layer.at(i)->setName(nameSet[i]);
    }
}

void FFT_Dialog::setupPlotterFont(QFont font)
{
    QCustomPlot *customPlot = ui->plotter;
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
    customPlot->yAxis2->setLabelFont(font);

//    customPlot->xAxis->setTickLengthIn(6);
//    customPlot->yAxis->setTickLengthIn(6);
//    customPlot->xAxis->setSubTickLengthIn(3);
//    customPlot->yAxis->setSubTickLengthIn(3);

    customPlot->replot();
}

void FFT_Dialog::setupPlotterOpenGL(bool enable)
{
    Q_UNUSED(enable)
    //似乎不支持2个图像同时使用GPU的情况，会有数据错位的情况
//    ui->plotter->setOpenGl(enable);
//    ui->plotter->replot();
}

void FFT_Dialog::plotter_init()
{

    //设置末尾箭头样式
//    ui->plotter->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
//    ui->plotter->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);

    if(ui->sample_frq->currentText().toUpper() != "AUTO")
        ui->plotter->xAxis->setRange(0, ui->sample_frq->currentText().toInt() / 2);
    ui->plotter->yAxis->setRange(0, 2);

    ui->plotter->legend->setVisible(true);

    //设置轴标签
    ui->plotter->xAxis->setLabel(tr("Freq (Hz)"));
    ui->plotter->yAxis->setLabel(tr("Amp"));

    //设置交互功能：范围可拖拽、范围可缩放、图例可选择、绘图区可选择
    ui->plotter->setInteractions(QCP::iRangeDrag |
                                QCP::iRangeZoom |   //缩放自己实现
                                QCP::iSelectLegend |
                                QCP::iSelectPlottables);
    //设置仅图例框中的条目可被选择
    ui->plotter->legend->setSelectableParts(QCPLegend::spItems);

    //设置自定义右键
    ui->plotter->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->plotter, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuRequest(QPoint)));
    connect(ui->plotter, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel(QWheelEvent*)));
}

void FFT_Dialog::moveLegend()
{
  if (QAction* contextAction = qobject_cast<QAction*>(sender())) // make sure this slot is really called by a context menu action, so it carries the data we need
  {
    bool ok;
    int dataInt = contextAction->data().toInt(&ok);
    if (ok)
    {
      ui->plotter->axisRect()->insetLayout()->setInsetAlignment(0, static_cast<Qt::Alignment>(dataInt));
      ui->plotter->replot();
    }
  }
}

void FFT_Dialog::rescaleXYAxis()
{
    ui->plotter->rescaleAxes(true);
    ui->plotter->replot();
}

void FFT_Dialog::autoRescaleXYAxis()
{
    autoRescaleAxisFlag = !autoRescaleAxisFlag;
}

void FFT_Dialog::savePicture()
{
    //打开保存文件对话框
    QString savePath = QFileDialog::getSaveFileName(this,
                                                    tr("曲线保存图片-选择文件路径"),
                                                    lastFileDialogPath + QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"),
                                                    "Bmp File(*.bmp);;Pdf File(*.pdf);;Jpeg File(*.jpg);;Png File(*.png);;All File(*.*)");
    //检查路径格式
    if(!savePath.endsWith(".jpg") &&
       !savePath.endsWith(".bmp") &&
       !savePath.endsWith(".png") &&
       !savePath.endsWith(".pdf")){
        if(!savePath.isEmpty())
            QMessageBox::information(this,tr("提示"),tr("尚未支持的文件格式。请选择jpg/bmp/png/pdf文件。"));
        return;
    }
    //记录上次路径
    lastFileDialogPath = savePath;
    lastFileDialogPath = lastFileDialogPath.mid(0, lastFileDialogPath.lastIndexOf('/')+1);

    //保存
    bool ok = false;
    if(savePath.endsWith(".jpg")){
        if(ui->plotter->saveJpg(savePath,0,0,1,100))
            ok = true;
    }
    if(savePath.endsWith(".bmp")){
        if(ui->plotter->saveBmp(savePath))
            ok = true;
    }
    if(savePath.endsWith(".png")){
        if(ui->plotter->savePng(savePath,0,0,1,100))
            ok = true;
    }
    if(savePath.endsWith(".pdf")){
        if(ui->plotter->savePdf(savePath))
            ok = true;
    }

    if(!ok){
        QMessageBox::warning(this, tr("警告"), tr("保存失败。文件是否被其他软件占用？"));
    }
}

bool FFT_Dialog::isAllGraphHide(void)
{
    int index = 0;
    for(;index < fft_layer.size(); index++){
        if(fft_layer.at(index)->visible()){
            return false;
        }
    }
    return true;
}

int32_t FFT_Dialog::findSelectedGraph()
{
    for(int32_t i = 0; i < fft_layer.size(); i++)
    {
        if(fft_layer.at(i)->selected() || ui->plotter->legend->item(i)->selected() )
            return i;
    }
    return -1;
}

void FFT_Dialog::hideSelectedGraph(void)
{
    int32_t index = findSelectedGraph();
    if(index >= 0 && index < fft_layer.size())
    {
        if(fft_layer.at(index)->visible())
        {
            fft_layer.at(index)->setVisible(false);
            ui->plotter->legend->item(index)->setTextColor(Qt::gray);
        }
        else
        {
            fft_layer.at(index)->setVisible(true);
            ui->plotter->legend->item(index)->setTextColor(Qt::black);
        }
    }
    ui->plotter->replot();
}

void FFT_Dialog::hideAllGraph(void)
{
    for(int i = 0; i < fft_layer.size(); i++)
    {
        fft_layer.at(i)->setVisible(false);
        ui->plotter->legend->item(i)->setTextColor(Qt::gray);
    }
    ui->plotter->replot();
}

void FFT_Dialog::showAllGraph(void)
{
    for(int i = 0; i < fft_layer.size(); i++)
    {
        fft_layer.at(i)->setVisible(true);
        ui->plotter->legend->item(i)->setTextColor(Qt::black);
    }
    ui->plotter->replot();
}

void FFT_Dialog::removeAllGraph(void)
{
    clearGraphs();
}

void FFT_Dialog::contextMenuRequest(QPoint pos)
{

    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    if (ui->plotter->legend->selectTest(pos, false) >= 0) // context menu on legend requested
    {
      menu->addAction(tr("移动到左上角"), this, SLOT(moveLegend()))->setData(static_cast<int>(Qt::AlignTop|Qt::AlignLeft));
      menu->addAction(tr("移动到右上角"), this, SLOT(moveLegend()))->setData(static_cast<int>(Qt::AlignTop|Qt::AlignRight));
      menu->addAction(tr("移动到右下角"), this, SLOT(moveLegend()))->setData(static_cast<int>(Qt::AlignBottom|Qt::AlignRight));
      menu->addAction(tr("移动到左下角"), this, SLOT(moveLegend()))->setData(static_cast<int>(Qt::AlignBottom|Qt::AlignLeft));
    }
    // general context menu on graphs requested
    menu->addSeparator();
    if (fft_layer.size() > 0){
        QAction *act = new QAction(tr("自动缩放"), this);
        act->setCheckable(true);
        if(autoRescaleAxisFlag)
        {
            act->setChecked(true);
            menu->addAction( act );
        }
        else
        {
            menu->addAction(tr("寻找频谱"), this, SLOT(rescaleXYAxis()));
            menu->addSeparator();
            act->setChecked(false);
            menu->addAction( act );
        }
        connect( act, SIGNAL(triggered() ), this, SLOT( autoRescaleXYAxis()) );
        menu->addSeparator();
        menu->addAction(tr("保存图片"), this, SLOT(savePicture()));
    }
    //选择了曲线
    if (findSelectedGraph() >= 0){
      menu->addSeparator();
      //所选曲线是否可见
      if(fft_layer.at(findSelectedGraph())->visible()){
          menu->addAction(tr("隐藏所选曲线"), this, SLOT(hideSelectedGraph()));
      }else{
          menu->addAction(tr("显示所选曲线"), this, SLOT(hideSelectedGraph()));
      }
    }
    menu->addSeparator();
    if(isAllGraphHide())
    {
        menu->addAction(tr("显示所有曲线"), this, SLOT(showAllGraph()));
    }else
    {
        menu->addAction(tr("隐藏所有曲线"), this, SLOT(hideAllGraph()));
    }
    menu->addSeparator();
    menu->addAction(tr("删除所有曲线"), this, SLOT(removeAllGraph()));
    menu->popup(ui->plotter->mapToGlobal(pos));
}

void FFT_Dialog::mouseWheel(QWheelEvent *w)
{
    if (ui->plotter->xAxis->selectTest(w->pos(), false) != -1){
        ui->plotter->axisRect()->setRangeZoom(Qt::Horizontal);
    }
    else if (ui->plotter->yAxis->selectTest(w->pos(), false) != -1){
        ui->plotter->axisRect()->setRangeZoom(Qt::Vertical);
    }
    else{
        ui->plotter->axisRect()->setRangeZoom(Qt::Horizontal|Qt::Vertical);
    }
}

//追加用于FFT计算的数据，注意追加数据期间数据长度会变化此时不要触发FFT计算，否则出错
void FFT_Dialog::appendData(DataProtocol::RowData_t &oneRowData)
{
    dataLock = 1;
    if(fft_cal_point_changed_tips)
    {
        fft_cal_point_changed_tips--;
    }
    for(int32_t i = 0; i < oneRowData.size(); i++)
    {
        //增删对象
        while(oneRowData.size() > graphsData.size())
        {
            graphsData.append(QVector<double>(fft_cal_point - 1));
            fft_layer.append(new QCPBars(ui->plotter->xAxis, ui->plotter->yAxis));
            fft_layer.last()->setAntialiased(true);
            if(nameSet.size() >= fft_layer.size())
                fft_layer.last()->setName(nameSet.at(fft_layer.size()-1));
            else
                fft_layer.last()->setName("unknown");
            if(colorSet.size() >= fft_layer.size())
            {
                fft_layer.last()->setBrush(colorSet.at(fft_layer.size()-1));
                fft_layer.last()->setPen(colorSet.at(fft_layer.size()-1));
            }
            else
            {
                fft_layer.last()->setBrush(QColor(0,0,0));
                fft_layer.last()->setPen(QColor(0,0,0));
            }
        }
        while(oneRowData.size() < graphsData.size())
        {
            graphsData.pop_back();
            fft_layer.last()->setData(QVector<double>(), QVector<double>());
            fft_layer.last()->removeFromLegend(ui->plotter->legend);
            fft_layer.pop_back();
        }
        graphsData[i].append(oneRowData.at(i));
        while(graphsData[i].size() > fft_cal_point)
        {
            graphsData[i].pop_front();
        }
    }
    inputCntPerSec++;
    needReplot = 1;
    dataLock = 0;
}

void FFT_Dialog::clearGraphs()
{
    //删除指定曲线会引起数据错位
    graphsData.clear();
    while(fft_layer.size())
    {
        fft_layer.last()->setData(QVector<double>(), QVector<double>());
        fft_layer.last()->removeFromLegend(ui->plotter->legend);
        fft_layer.pop_back();
    }
    fft_cal_point_changed_tips = ui->cal_point->currentText().toInt();
    ui->plotter->replot();
}

//追加数据和计算分开，批量填充数据周期计算可以减轻CPU负担,用信号的方式确保计算在另一个线程中
uint8_t FFT_Dialog::startFFTCal()
{
    if(dataLock)
        return -1;

    for(int32_t i = 0; i < graphsData.size(); i++)
    {
        emit start_fft_cal(i, fft_sample_frq, graphsData.at(i));
    }
    return 0;
}

void FFT_Dialog::disableRePlot(bool flag)
{
    disableReplotFlag = flag;
}

void FFT_Dialog::setFFTPlotterTitle(QString title)
{
    fftPlotterTitle = title;
    this->setWindowTitle("Actived FFT(beta): " + title);
}
QString FFT_Dialog::getFFTPlotterTitle()
{
    return fftPlotterTitle;
}

//寻找最接近的2的幂
int32_t FFT_Dialog::find_num_closest_power_of_2(int a)
{
    int n = a -1;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n+1;
}

void FFT_Dialog::__100msTimerSlot()
{
    static uint32_t cnt = 0;
    //fft序列不足提示
    if(fft_cal_point_changed_tips)
    {
        ui->statusBar->showMessage(tr("正在收集信号序列：") +
                                   QString::number(fft_cal_point_changed_tips));
    }
    else
    {
        ui->statusBar->showMessage("");
    }
    //秒处理任务
    if(cnt % 10 == 0)
    {
        //自动计算采样频率
        if(ui->sample_frq->currentText().toUpper() == "AUTO")
        {
            estimated_sample_frq = find_num_closest_power_of_2(inputCntPerSec);
            fft_sample_frq = estimated_sample_frq;
        }
        inputCntPerSec = 0;
    }
    //刷新曲线
    if(needReplot && !disableReplotFlag)
    {
        if(autoRescaleAxisFlag)
            ui->plotter->rescaleAxes(true);
        ui->plotter->replot();
        needReplot = 0;
    }
    cnt++;
}

void FFT_Dialog::get_fft_result(qint8 index, const QVector<double> &x_ticks, const QVector<double> &result)
{
    if(index >= fft_layer.size())
    {
        return;
    }
    if(dataLock)
    {
        return;
    }
    if(x_ticks.isEmpty() || result.size() != x_ticks.size())
    {
        return;
    }

    fft_layer[index]->setData(x_ticks, result);
}

void FFT_Dialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    if(__checkHandler && __checkHandler->isChecked())
        __checkHandler->triggered(false);
}

void FFT_Dialog::on_cal_point_currentTextChanged(const QString &arg1)
{
    //输入合法性判断是否是2的幂
    if(arg1.toInt() & (arg1.toInt()-1))
    {
        int32_t tmp = find_num_closest_power_of_2(arg1.toInt());
        ui->cal_point->setCurrentText(QString::number(tmp));
        return;
    }
    fft_cal_point = arg1.toInt();
    //限幅
    if(fft_cal_point > MAX_N)
    {
        fft_cal_point = MAX_N;
        ui->cal_point->setCurrentText(QString::number(MAX_N));
        QMessageBox::information(this, tr("提示"),
                                 tr("FFT计算点数不得超过：") + QString::number(MAX_N));
    }

    //fft点变化提示
    if(last_fft_cal_point < fft_cal_point)
    {
        fft_cal_point_changed_tips = fft_cal_point - last_fft_cal_point;
    }else
    {
        fft_cal_point_changed_tips = 0;
    }
    last_fft_cal_point = fft_cal_point;

    //立刻重新计算
    fft->init_size(fft_cal_point);
    startFFTCal();
}

void FFT_Dialog::on_sample_frq_currentTextChanged(const QString &arg1)
{
    fft_sample_frq = arg1.toInt();
    //note:auto是自动处理的
    if(ui->sample_frq->currentText().toUpper() != "AUTO")
    {
        ui->plotter->xAxis->setRange(0, fft_sample_frq / 2);
        ui->plotter->replot();
    }
}
