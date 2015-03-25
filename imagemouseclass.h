#ifndef IMAGEMOUSECLASS_H
#define IMAGEMOUSECLASS_H

#include <QLabel>
#include <QMouseEvent>
#include <QEvent>

class imageMouseClass : public QLabel
{
    Q_OBJECT
public:
    explicit imageMouseClass(QWidget *parent = 0);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void leaveEvent(QEvent *);
    int x, y;

signals:
    void Mouse_Pressed();
    void Mouse_Pos();
    void Mouse_Left();

public slots:

};

#endif // IMAGEMOUSECLASS_H

