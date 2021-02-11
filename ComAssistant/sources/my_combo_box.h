/**
 * @brief   自定义ComBoBox
 * @file    my_combo_box.h
 * @author  inhowe
 * @version 0.0.1
 * @date    2021-2月-11
 * @note    实现了下拉刷新功能
 */
#ifndef MY_COMBO_BOX_H
#define MY_COMBO_BOX_H

#include <QObject>
#include <QWidget>
#include <QComboBox>
#include <QDebug>
#include <QMouseEvent>

class My_Combo_Box : public QComboBox
{
    Q_OBJECT
public:
    explicit My_Combo_Box(QWidget *parent = nullptr);
    void mousePressEvent(QMouseEvent *e);

signals:
    void leftPressed(void);
};

#endif // MY_COMBO_BOX_H
