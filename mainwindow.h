#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QScrollArea>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:

private slots:
   void onStartButtonClicked();
   void onReadyRead();
   void onCalibrateClicked();
   void onSaveColorClicked();
   void onSaveAngleClicked();
   void onSaveFileButtonClicked();
   void onFileDirectoryButtonClicked();
   void onLoadImageClicked();
   void getTransformClicked();

   void Mouse_Current_Pos();
   void Mouse_Pressed();
   void Mouse_left();

private:
    Ui::MainWindow *ui;
    QTimer *timer;
    QLabel *imageLabel;
    QScrollArea *scrollArea;

};

#endif // MAINWINDOW_H
