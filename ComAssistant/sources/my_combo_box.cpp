#include "my_combo_box.h"

My_Combo_Box::My_Combo_Box(QWidget *parent) : QComboBox(parent)
{

}

void My_Combo_Box::mousePressEvent(QMouseEvent *e)
{
    if(this->count())
        this->showPopup();
    if(e->button() == Qt::LeftButton)
    {
        emit leftPressed();
    }
}
