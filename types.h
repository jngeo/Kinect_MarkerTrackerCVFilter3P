#ifndef TYPES_H
#define TYPES_H

#define PI 3.1415
#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 480

#include <windows.h>
#include <math.h>
#include <vector>
#include <Eigen/Dense>
#include <Eigen/Eigen>

//#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <QImage>
#include <QDebug>

using namespace Eigen;
using namespace cv;

typedef Eigen::Matrix<double, 1, 3> Point3D;
typedef Eigen::Matrix<double, 3, 1> Vector3D;


cv::Mat QImage2Mat(QImage & inImage, bool inCloneImageData = true)
{
    switch ( inImage.format() )
      {
         case QImage::Format_RGB32:     // 8-bit, 4 channel
         {
            cv::Mat  mat( inImage.height(), inImage.width(), CV_8UC4, const_cast<uchar*>(inImage.bits()), inImage.bytesPerLine() );
            return (inCloneImageData ? mat.clone() : mat);
         }
         case QImage::Format_RGB888:    // 8-bit, 3 channel
         {
            if ( !inCloneImageData )
               qWarning() << "ASM::QImageToCvMat() - Conversion requires cloning since we use a temporary QImage";
            QImage   swapped = inImage.rgbSwapped();
            return cv::Mat( swapped.height(), swapped.width(), CV_8UC3, const_cast<uchar*>(swapped.bits()), swapped.bytesPerLine() ).clone();
         }
        case QImage::Format_Indexed8:   // 8-bit, 1 channel
         {
            cv::Mat  mat( inImage.height(), inImage.width(), CV_8UC1, const_cast<uchar*>(inImage.bits()), inImage.bytesPerLine() );
            return (inCloneImageData ? mat.clone() : mat);
         }
         default:
            qWarning() << "ASM::QImageToCvMat() - QImage format not handled in switch:" << inImage.format();
            break;
    }
    return cv::Mat();

    /*
     cv::Mat tmp(src.height(),src.width(),CV_8UC3,(uchar*)src.bits(),src.bytesPerLine());
     cvtColor(tmp, result,CV_BGR2RGB);
     return result;*/
}

QImage Mat2QImage(cv::Mat & inMat)
{
    switch ( inMat.type() )
    {
        case CV_8UC4:           // 8-bit, 4 channel
        {
            QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_RGB32 );
            return image;
        }
        case CV_8UC3:            // 8-bit, 3 channel
        {
            QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_RGB888 );
            return image.rgbSwapped();
        }
        case CV_8UC1:           // 8-bit, 1 channel
        {
            static QVector<QRgb>  sColorTable;
            // only create our color table once
            if ( sColorTable.isEmpty() )
            {
               for ( int i = 0; i < 256; ++i )
                  sColorTable.push_back( qRgb( i, i, i ) );
            }
            QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_Indexed8 );
            image.setColorTable( sColorTable );
            return image;
         }
         default:
            qWarning() << "ASM::cvMatToQImage() - cv::Mat image type not handled in switch:" << inMat.type();
            break;
    }
    return QImage();

    /*
     cv::Mat temp; // make the same cv::Mat
     cvtColor(src, temp,CV_BGR2RGB); // cvtColor Makes a copt, that what i need
     QImage dest((const uchar *) temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
     dest.bits(); // enforce deep copy, see documentation of QImage::QImage ( const uchar * data, int width, int height, Format format )
     return dest;
     */
}

#endif // TYPES_H
