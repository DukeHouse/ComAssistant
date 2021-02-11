/**
 * @brief   FFT处理文件
 * @file    fft.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-2月-11
 * @note    使用蝶形运算
 */
#ifndef FFT_H
#define FFT_H

#include <QObject>
#include <QVector>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <QDebug>
#include <QThread>

#define MAX_N 8192    //FFT点数

typedef struct{         //定义一个结构体表示复数的类型
    double real;
    double imag;
}complex;

class fft_trans:public QObject
{
    Q_OBJECT
public:
    explicit fft_trans(QObject *parent = nullptr);
    ~fft_trans();
    /*
     * 初始化FFT信号序列大小（参与计算的数据长度，真实输入的信号序列会进行多删少补）
    */
    int32_t init_size(int32_t new_size);
    QVector<double> fft_data;

private:
    void change();
    void transform();
    void fft();
    double cal_norm_and_find_max(complex* x, int32_t size_of_x, double *norm, int32_t size_of_norm);
    void add(complex a, complex b, complex *c);
    void sub(complex a, complex b, complex *c);
    void mul(complex a, complex b, complex *c);
    complex x[MAX_N], *W;   //定义输入序列和旋转因子
    double l[MAX_N] = {0};
    int size = MAX_N;   //参与计算的信号序列长度，必须是2的幂次方
    const double PI = atan(1)*4; //定义π 因为tan(π/4)=1 所以arctan（1）* 4=π，增加π的精度
public slots:
    /*
     * index: 序号，用于区分不同信号序列的计算结果
     * sample_frq: 信号序列的采样频率，错误的采样频率会导致频谱图的映射关系出错
     * data: 参与计算的信号序列
    */
    void fft_calculate(qint8 index, qint32 sample_frq, QVector<double> data);//index只是区分是哪个曲线的FFT结果
signals:
    /*
     * 信号需要连接到槽才可使用
     * index: 序号，用于区分不同信号序列的计算结果
     * x_ticks: 频谱图的X轴
     * result : 频谱图的Y轴
    */
    void fft_result(qint8 index, QVector<double> x_ticks, QVector<double> result);
};

#endif // FFT_H
