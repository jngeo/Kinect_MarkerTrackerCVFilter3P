#include "mainwindow.h"
#include "ui_mainwindow.h"

//#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <vector>

#include <QDebug>
#include <QTimer>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>

#include <fstream>
#include <iostream>
#include <stdio.h>

#include "types.h"
#include "kinect.h"
#include "calibration.h"
#include "pointcloud.h"

using std::vector;
using namespace cv;
using namespace std;

Mat rgbImage, img, dep, dep8bit, colorImage, buffImage;
QImage outputImage, outputRGB, outputDepth, outputColor;
vector<Point3D> clickPoints, calibCoordPoints;
Vector3D color;         //, collarColor;
std::vector<cv::Mat> ch;
double angle, absVel, absVelFilt;
MatrixXd cloudImage, colorBlobPos(1,3), colorBlobPosFilt(1,3), buffPos(3,3), buffPosFilt(2,3),
                     colorBlobPosPrev(1,3), colorBlobPosPrevFilt(1,3),
                     colorBlobVel(1,3), colorBlobVelFilt(1,3);
MatrixXd calibCoorMat;
Mat calibCvMat, transformMatrix;

MatrixXd rotmat(3,3), transvect(3,1);

ofstream fid;
ifstream fid1;

bool runFlag = false;
bool saveFlag = false;
QString fileComplete;
QFile csvfile;

__int64 last_frame, cnt, freq, first_get;
int frameNum;



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    kinectInit();
    timer = new QTimer(this);       timer->setInterval(5);

    frameNum = 0;   last_frame = 0;     cnt =  0;   first_get = 0;
    //get frequency of CPU
    QueryPerformanceFrequency((LARGE_INTEGER *) &freq);
    qDebug() << "CPU frequency: " << freq;

    QImage image("C:/Users/Jim/Pictures/ice3.jpg");
        rgbImage = QImage2Mat(image);

    connect(ui->imageLabel, SIGNAL(Mouse_Pressed()), this, SLOT(Mouse_Pressed()));
    connect(ui->startButton, SIGNAL(clicked()), SLOT(onStartButtonClicked()));
    connect(ui->calibrateButton, SIGNAL(clicked()), SLOT(onCalibrateClicked()));
        connect(ui->saveColorButton, SIGNAL(clicked()), SLOT(onSaveColorClicked()));
        connect(ui->saveAngleButton, SIGNAL(clicked()), SLOT(onSaveAngleClicked()));
    connect(ui->saveFileButton, SIGNAL(clicked()), SLOT(onSaveFileButtonClicked()));
    connect(ui->fileDirectoryButton, SIGNAL(clicked()), SLOT(onFileDirectoryButtonClicked() ));
    connect(ui->loadImButton, SIGNAL(clicked()), SLOT(onLoadImageClicked() ));
    connect(ui->getTransformButton, SIGNAL(clicked()), SLOT(getTransformClicked() ));

    connect(timer, SIGNAL(timeout()), this, SLOT(onReadyRead()));

    kinectUpdate(img,dep);
        outputImage = Mat2QImage(img);
        ui->imageLabel->setPixmap(QPixmap::fromImage(outputImage));

    setWindowTitle(tr("Image Viewer"));
        resize(980, 540);
}

MainWindow::~MainWindow()
{
    delete ui;
}


//Button to start/stop kinect pressed
void MainWindow::onStartButtonClicked()
{
    frameNum = 0;   last_frame = 0;     cnt =  0;
    QueryPerformanceFrequency((LARGE_INTEGER *) &freq);

    colorBlobVel(0,0) = 0;      colorBlobVel(0,1) = 0;      colorBlobVel(0,2) = 0;
    colorBlobPosPrev(0,0) = 0;  colorBlobPosPrev(0,1) = 0;  colorBlobPosPrev(0,2) = 0;

    if(!runFlag)
    {
       kinectInit();
       //Color values
            fid1.open("Color");
            fid1 >> color(0) >> color(1) >> color(2);
            fid1.close();
       //Angle values
            fid1.open("Angle");
            fid1 >> angle;
            fid1.close();
        //Rotation matrix
             fid1.open("rotmat");
             fid1 >> rotmat(0,0) >> rotmat(0,1) >> rotmat(0,2)
                  >> rotmat(1,0) >> rotmat(1,1) >> rotmat(1,2)
                  >> rotmat(2,0) >> rotmat(2,1) >> rotmat(2,2) ;
             fid1.close();
             qDebug() << "rotmat: " << rotmat(0,0) << rotmat(0,1) << rotmat(0,2);
             qDebug() << "rotmat: " << rotmat(1,0) << rotmat(1,1) << rotmat(1,2);
             qDebug() << "rotmat: " << rotmat(2,0) << rotmat(2,1) << rotmat(2,2);
         //translation vector
              fid1.open("transvect");
              fid1 >> transvect(0,0) >> transvect(1,0) >> transvect(2,0);
              fid1.close();
              qDebug() << "transvect: " <<  transvect(0,0) <<  transvect(1,0) <<  transvect(2,0);

       ui->startButton->setText("Stop Kinect");
       ui->startButton->setStyleSheet("* { background-color: rgb(255,0,0) }");

       QueryPerformanceCounter( (LARGE_INTEGER *)&first_get );
            last_frame = first_get;
       timer->start();
       runFlag = 1;
       qDebug() << "timer has started";
    }
    else
    {
       ui->startButton->setText(tr("&Start Kinect"));
       ui->startButton->setStyleSheet("* { background-color: rgb(240,240,240) }");

       timer->stop();
       context.Shutdown();
       runFlag = 0;
       qDebug() << "timer has stopped";
    }
}

void MainWindow::onCalibrateClicked()
{
    qDebug() << "Calibration has started!";
    kinectInit();
    clickPoints.clear();
    calibCoordPoints.clear();
    color = Vector3D::Zero();

    kinectUpdate(img,dep);
        outputImage = Mat2QImage(img);
        ui->imageLabel->setPixmap(QPixmap::fromImage(outputImage));

    //extract color
    fid1.open("Color");
    fid1 >> color(0) >> color(1) >> color(2);
    fid1.close();
    colorImage = colorExtraction(img, color);
        //test: grayscale
        Mat gray;
        cvtColor(colorImage, buffImage, CV_BGR2GRAY);
        buffImage.convertTo(buffImage, CV_8UC3);        //have to convert to 8UC3 for proper
            outputImage = Mat2QImage(buffImage);
            //ui->imageLabel->setPixmap(QPixmap::fromImage(outputImage));

    cloudImage = cloudExtraction(colorImage, dep, 0);
        qDebug() << "size: " << cloudImage.rows() << " x " << cloudImage.cols();

    if(cloudImage.rows() != 0 || cloudImage.cols() != 0)
        angle = calibrate(cloudImage);
    else
        angle = 0;
    qDebug() << "angle: " << angle;

    //context.Shutdown();
    //delay(100);
}

void MainWindow::onLoadImageClicked()
{
    qDebug() << "New Image frame has been loaded!";
    kinectInit();
    clickPoints.clear();
    calibCoordPoints.clear();

    kinectUpdate(img,dep);
        //dep8bit = dep;
        //dep8bit.convertTo(dep8bit, CV_8UC1);
        //outputImage = Mat2QImage(dep8bit);
        outputImage = Mat2QImage(img);
        ui->imageLabel->setPixmap(QPixmap::fromImage(outputImage));
}

void MainWindow::onSaveColorClicked()
{
    color = colorCalib(img, clickPoints);
        qDebug() << "color: " << color(0) << "   " << color(1) << "  " << color(2);
    fid.open("Color");
        fid << color.transpose() << endl;
        fid.close();
}

void MainWindow::getTransformClicked()
{
    calibCvMat = Mat(3,3, CV_32F);  //, dummy_data0);

    calibCoorMat = rgbPoint2Cloud(img, calibCoordPoints, dep);
    fid.open("Coordinates");
    for (int i = 0; i < calibCoorMat.rows() ; i++)
    {
        fid << (float) calibCoorMat(i,0) << "   " << (float) calibCoorMat(i,1) << "   " << (float) calibCoorMat(i,2) << "   " << endl;

        for (int j = 0; j < 3 ; j++)
            calibCvMat.at<float>(i,j) = (float) calibCoorMat(i,j);
    }
    fid.close();

    transformMatrix = getTransMatrix(calibCvMat);

}

void MainWindow::onSaveAngleClicked()
{
    color = colorCalib(img, clickPoints);
        qDebug() << "angle: " << angle;
    fid.open("Angle");
        fid << angle << endl;
        fid.close();
}

void MainWindow::onReadyRead()
{
    QueryPerformanceCounter((LARGE_INTEGER *) &cnt);
    qDebug() << "cnt : " << cnt;
    qDebug() << "lastframe : " << last_frame;
    qDebug() << "freq : " << freq;
    qDebug() << "time : " << ((double)(cnt - last_frame) / freq);

    kinectUpdate(img,dep);
        //dep8bit = dep;
        //dep8bit.convertTo(dep8bit, CV_8UC1);
        outputRGB = Mat2QImage(img);
        //outputDepth = Mat2QImage(dep8bit);
    colorImage = colorExtraction(img, color);
        outputImage = Mat2QImage(colorImage);
    cloudImage = cloudExtraction(colorImage, dep, angle);

    if(cloudImage.rows() != 0 || cloudImage.cols() != 0)
    {
        //qDebug() << "frame: " << frameNum;
        colorBlobPos = colorCenterTrack(cloudImage);
            colorBlobVel.row(0) = colorBlobPos.row(0) - colorBlobPosPrev.row(0);
            absVel = absoluteVelocity(colorBlobVel);

        //rotate
        colorBlobPos = (rotmat*colorBlobPos.transpose() + transvect).transpose();
            //checkPlane = (rotmat*calibMatTrans) + transvectMat;

        if(frameNum < 3)
        {
            buffPos.row(frameNum) = colorBlobPos.row(0);
            colorBlobPosFilt =  colorBlobPos.row(0);

            if(frameNum > 0)
                buffPosFilt.row(frameNum-1) = colorBlobPos.row(0);
        }
        else
        {
            buffPos.row(0) = buffPos.row(1);
            buffPos.row(1) = buffPos.row(2);
            buffPos.row(2) = colorBlobPos.row(0);

            colorBlobPosFilt = pointLowPassFilt(buffPos, buffPosFilt);

            //low pass filter
            buffPosFilt.row(0) = buffPosFilt.row(1);
            buffPosFilt.row(1) = colorBlobPosFilt.row(0);
        }

        //colorBlobPosFilt = pointLowPassFilt(colorBlobPos);
            colorBlobVelFilt.row(0) = colorBlobPosFilt.row(0) - colorBlobPosPrevFilt.row(0);
            absVelFilt = absoluteVelocity(colorBlobVelFilt);

        //qDebug() << "absolute velocity: " << absVel;
        //qDebug() << "colorBlobVel: " << colorBlobVel(0,0) << "  " << colorBlobVel(0,1) << "  " << colorBlobVel(0,2);

        if (saveFlag)
        {
            QTextStream outCom(&csvfile);
            outCom << frameNum << ", " << ((double)(cnt - last_frame) / freq) << ", "
                   << colorBlobPosFilt(0,0) << ", "
                   << colorBlobPosFilt(0,1) << ", "
                   << colorBlobPosFilt(0,2) << ", "
                   << absVelFilt << ", "
                   << colorBlobVelFilt(0,0) << ", " << colorBlobVelFilt(0,1) << ", " << colorBlobVelFilt(0,2) << endl;
            /*
            outCom << frameNum << ", " << ((double)(cnt - last_frame) / freq) << ", "
                   << colorBlobPos(0,0) << ", " << colorBlobPosFilt(0,0) << ", "
                   << colorBlobPos(0,1) << ", " << colorBlobPosFilt(0,1) << ", "
                   << colorBlobPos(0,2) << ", " << colorBlobPosFilt(0,2) << ", "
                   << absVel << ", " << absVelFilt << ", "
                   << colorBlobVel(0,0) << ", " << colorBlobVel(0,1) << ", " << colorBlobVel(0,2) << endl;
                   */
        }
    }


    ui->imageLabel->setPixmap(QPixmap::fromImage(outputImage));

    ui->imageKinectLabel->setScaledContents(true);
    ui->imageKinectLabel->setPixmap(QPixmap::fromImage(outputRGB));

    colorBlobPosPrev = colorBlobPos;
    colorBlobPosPrevFilt = colorBlobPosFilt;
    last_frame = cnt;
    frameNum++;
}

void MainWindow::onSaveFileButtonClicked()
{
    frameNum = 0;   cnt =  0;
    QueryPerformanceCounter( (LARGE_INTEGER *)&first_get );
         last_frame = first_get;

    if(!saveFlag && runFlag)
    {
        saveFlag = true;
        ui->saveFileButton->setText("Stop Saving");
        ui->saveFileButton->setStyleSheet("* { background-color: rgb(255,0,0) }");

        //data_saveTime.clear();

        int fileNameValue = ui->fileNameSpinBox->value();
        QString fileDirectory = ui->fileLocationTextEdit->displayText();
        QString fileName = ui->fileNameTextEdit->displayText();
            fileComplete = fileDirectory.append("/");
            fileComplete = fileComplete.append(fileName);
            fileComplete = fileComplete.append(QString::number(fileNameValue));
            fileComplete = fileComplete.append(".csv");
        if (fileComplete.isEmpty() || fileDirectory.isEmpty() || fileName.isEmpty())
        {
            qDebug() << "No valid filename or file directory is detected!";
            return;
        }
        else
        {
            csvfile.setFileName(fileComplete);
            if (!csvfile.open(QIODevice::WriteOnly))
            {
                QMessageBox::information(this, tr("Unable to open file"),
                    csvfile.errorString());
                return;
            }
            csvfile.open(QIODevice::Append | QIODevice::Text);
            QTextStream outcsvfile(&csvfile);
            outcsvfile << "Frame, " << "Time_def, " << "MFilt_x, "
                                                    << "MFilt_y, "
                                                    << "MFilt_z, "
                                                    << "VelFilt_abs, "
                                                    << "Vel_x, " << "Vel_y, " << "Vel_z" << endl;
            /*
            outcsvfile << "Frame, " << "Time_def, " << "Mk_x, " << "MFilt_x, "
                                                    << "Mk_y, " << "MFilt_y, "
                                                    << "Mk_z, " << "MFilt_z, "
                                                    << "Vel_abs, " << "VelFilt_abs, "
                                                    << "Vel_x, " << "Vel_y, " << "Vel_z" << endl;
            */
        }
    }

    else if(saveFlag )
    {
        csvfile.close();
        saveFlag = false;
        int fileNameValue = ui->fileNameSpinBox->value();
        fileNameValue = fileNameValue + 1;
        ui->fileNameSpinBox->setValue(fileNameValue);
        ui->receiveEditText->setText(fileComplete);

        ui->saveFileButton->setText(tr("&Save..."));
        ui->saveFileButton->setToolTip(tr("Save contacts to a file"));      //ui->saveButton->setEnabled(false);
        ui->saveFileButton->setStyleSheet("* { background-color: rgb(240,240,240) }");
    }
    else
    {
    }
}

void MainWindow::onFileDirectoryButtonClicked()
{
    QString fileDirectory = QFileDialog::getExistingDirectory(this,"Caption",QString(),QFileDialog::ShowDirsOnly);
    qDebug()<<fileDirectory;
    ui->fileLocationTextEdit->setText(fileDirectory);
}

void MainWindow::Mouse_Pressed()
{
  Point3D p = Point3D::Zero();
      p(0) = ui->imageLabel->x;
      p(1) = ui->imageLabel->y;
  clickPoints.push_back(p);
  calibCoordPoints.push_back(p);
      qDebug() << "x-position: " << p(0) << "y-position: " << p(1);
}

void MainWindow::Mouse_Current_Pos()
{
}

void MainWindow::Mouse_left()
{
}

