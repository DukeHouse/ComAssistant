#ifndef FFT_DIALOG_H
#define FFT_DIALOG_H

#include <QDialog>
#include <QObject>
#include <QDebug>
#include <QLabel>
#include "qcustomplot.h"
#include "fft.h"
#include <QThread>
#include "dataprotocol.h"
#include "config.h"

namespace Ui {
class FFT_Dialog;
}

class FFT_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit FFT_Dialog(QAction *checkHandler, QWidget *parent = nullptr);
    ~FFT_Dialog();
    void setColorSet(const QVector<QColor> &color_set);
    void setNameSet(const QVector<QString> &name_set);
    void setupPlotterFont(QFont font);
    void setupPlotterOpenGL(bool enable);
    void appendData(DataProtocol::RowData_t &oneRowData);
    void clearGraph(int8_t index);
    uint8_t startFFTCal();

private:
    void plotter_init();
    void plotter_show(QVector<double> &x, QVector<double> &y);
    int32_t find_num_closest_power_of_2(int a);
    void update_x_ticks();
    bool isAllGraphHide(void);
    int32_t findSelectedGraph();
    Ui::FFT_Dialog *ui;
    QVector<QColor> colorSet;
    QVector<QString> nameSet;
    QVector<QVector<double>> graphsData;    //用于fft计算的数据，按曲线分组
    QThread *fft_thread = nullptr;
    fft_trans *fft = nullptr;
    QAction *__checkHandler = nullptr;
    QVector<QCPBars*> fft_layer;
    QTimer *__100msTimer = nullptr;
    uint8_t dataLock = 0;
    int32_t inputCntPerSec = 0;
    int32_t estimated_sample_frq = 0;
    int32_t fft_sample_frq = 0;
    int32_t fft_cal_point;
    uint8_t needReplot = 0;
    int32_t last_fft_cal_point = 0;
    int32_t fft_cal_point_changed_tips = 0;
    int8_t autoRescaleAxisFlag = 1;
    QString lastFileDialogPath;
protected:
    void closeEvent(QCloseEvent *event);
signals:
    void start_fft_cal(qint8 index, qint32 sample_frq, QVector<double> data);
private slots:
    void __100msTimerSlot();
    void get_fft_result(qint8 index, QVector<double> x_ticks, QVector<double> result);
    void on_cal_point_currentTextChanged(const QString &arg1);
    void on_sample_frq_currentTextChanged(const QString &arg1);
    void contextMenuRequest(QPoint pos);
    void moveLegend();
    void rescaleXYAxis();
    void autoRescaleXYAxis();
    void savePicture();
    void hideSelectedGraph(void);
    void hideAllGraph(void);
    void showAllGraph(void);
    void mouseWheel(QWheelEvent *w);
};

#endif // FFT_DIALOG_H
