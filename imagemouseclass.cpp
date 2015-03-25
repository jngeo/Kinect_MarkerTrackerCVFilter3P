#include "imagemouseclass.h"

imageMouseClass::imageMouseClass(QWidget *parent) :
    QLabel(parent)
{
}

void imageMouseClass::mouseMoveEvent(QMouseEvent *event)
{
    this->x = event->x();
    this->y = event->y();
    emit Mouse_Pos();
}

void imageMouseClass::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        this->x = event->x();
        this->y = event->y();
        emit Mouse_Pressed();
    }

}

void imageMouseClass::leaveEvent(QEvent *)
{
    emit Mouse_Left();
}
