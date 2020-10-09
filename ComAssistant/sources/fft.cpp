#include "fft.h"

fft_trans::fft_trans(QObject *parent) : QObject(parent)
{
    W = (complex *)malloc(sizeof(complex) * size);
}

fft_trans::~fft_trans()
{
    free(W);
}

int32_t fft_trans::init_size(int32_t new_size)
{
    if(new_size > MAX_N)
    {
        return -1;
    }
    size = new_size;
    return 0;
}

void fft_trans::change()
{
    complex temp;
    unsigned short i = 0, j = 0, k = 0;
    double t;
    for(i = 0; i < size; i++)
    {
        k=i;
        j=0;
        t=(log(size) / log(2));
        while( (t--) > 0 )
        {
            j = j << 1;
            j |=(k & 1);
            k = k >> 1;
        }
        if(j > i)
        {
            temp = x[i];
            x[i] = x[j];
            x[j] = temp;
        }
    }
}

void fft_trans::transform()
{
    int i;
    memset(W, 0, sizeof(complex) * size);
    for(i = 0; i < size; i++)
    {
        W[i].real = cos(2 * PI / size * i);
        W[i].imag = -1 * sin(2 * PI / size * i);
    }
}

inline void fft_trans::add(complex a,complex b,complex *c)
{
    c->real = a.real + b.real;
    c->imag = a.imag + b.imag;
}

inline void fft_trans::sub(complex a,complex b,complex *c)
{
    c->real = a.real - b.real;
    c->imag = a.imag - b.imag;
}

inline void fft_trans::mul(complex a,complex b,complex *c)
{
    c->real = a.real * b.real - a.imag * b.imag;
    c->imag = a.real * b.imag + a.imag * b.real;
}

void fft_trans::fft()
{
    int i = 0, j = 0, k = 0, m=0;
    complex q, y, z;
    change();
    for(i = 0; i < log(size) / log(2); i++)
    {
        m = 1 << i;
        for(j = 0; j < size; j += 2 * m)
        {
            for(k = 0; k < m; k++)
            {
                mul(x[k + j + m], W[size * k / 2 / m], &q);
                add(x[j + k], q, &y);
                sub(x[j + k], q, &z);
                x[j + k] = y;
                x[j + k + m] = z;
            }
        }
    }
}

// 计算向量模长并寻找最大模长
double fft_trans::cal_norm_and_find_max(complex* x, int32_t size_of_x, double *norm, int32_t size_of_norm)
{
    double max_norm = 0;
    int32_t i = 0;
    if(size_of_x != size_of_norm)
        return -1;
    for(i = 0; i < size_of_x; i++)  //计算变换结果的模长
    {
        norm[i] = sqrt(x[i].imag * x[i].imag + x[i].real * x[i].real);
        if(i == 0)
        {
            norm[i] = norm[i] / 2;
        }
        if(norm[i] > max_norm)
        {
            max_norm = norm[i];
        }
    }
    return max_norm;
}

void fft_trans::fft_calculate(qint8 index, qint32 sample_frq, QVector<double> data)
{
//    qDebug()<<"ThreadID:"<<QThread::currentThreadId()<<"fft_calculate";
    int i;
    //对齐
    while(data.size() < size)
    {
        data.append(0);
    }
    while(data.size() > size)
    {
        data.pop_front();
    }

    for(i = 0; i < size; i++)
    {
        x[i].real = data[i];
        x[i].imag = 0;
    }
    transform();//变换序列顺序
    fft();//蝶形运算
    double max = 0;
    max = cal_norm_and_find_max(x, size, l, size);

    //幅值转换
    fft_data.clear();
    for(i = 0; i < size; i++)
    {
        fft_data << l[i] / (size / 2);
    }
    fft_data = fft_data.mid(0, fft_data.size() / 2);//由于对称性只要一半数据即可

    //频率转换
    QVector<double> x_ticks;
    for(int i = 0; i < size / 2; i++)
    {
        x_ticks << (double)i / size * sample_frq; //X轴由无单位序列转化为频率单位
    }
    emit fft_result(index, x_ticks, fft_data);
}
