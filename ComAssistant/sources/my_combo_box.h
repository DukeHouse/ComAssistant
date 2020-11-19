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
