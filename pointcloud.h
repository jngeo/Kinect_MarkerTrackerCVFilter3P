#ifndef POINTCLOUD_H
#define POINTCLOUD_H

#include "types.h"
#include <fstream>
#include <iostream>

using namespace std;

double sumBlock(Mat & block)
{
  double sum = 0;
  for(int i = 0; i < block.cols; i++)
    {
      for(int j = 0; j < block.rows; j++)
        {
            sum = sum + block.at<uchar>(i,j);
        }
    }
  return sum;
}


MatrixXd cloudExtraction(cv::Mat & inputRGB, cv::Mat & inputDepth, double angle)
{
    Mat gray, gray3CH;
    MatrixXd inputCloud,outputCloud;

    int boxsize = 1;
    double f = 525.0;
    double cx = 319.5;
    double cy = 239.5;

    double radius = 20.0;   //int nPoints = 5

    //Defining the Images
    cvtColor(inputRGB, gray, CV_BGR2GRAY);
    cvtColor(inputRGB, gray3CH, CV_BGR2GRAY);
        gray.convertTo(gray, CV_8UC1);
        //gray3CH.convertTo(gray3CH, CV_8UC3);

    //Down sampling of Pixels
    Point3D p;
    double sum;
    int d,sx,sy,nPoints = 0;
    Mat depthblock,imageblock;

    for (int x = 0; x<IMAGE_WIDTH; x++)
        for (int y = 0; y<IMAGE_HEIGHT; y++)
        {
            //gray(cv::Rect(x,y,1,1)) = cv::Scalar(250,0,0);
            //qDebug() << "depth ushort: " << (ushort)inputDepth(Rect(x,y,1,1)).at<ushort>(0,0);

            depthblock = inputDepth(Rect(x,y,boxsize,boxsize));
            imageblock = gray(Rect(x,y,boxsize,boxsize));
            sum = sumBlock(imageblock);
            d = (ushort)depthblock.at<ushort>(boxsize/2,boxsize/2);

            if(sum > 0 && d > 0)
            {
                //gray3CH(cv::Rect(x,y,1,1)) = cv::Scalar(250,250,250);       //for checking turn blob into white
                nPoints++;
                sx = x + boxsize/2;
                sy = y + boxsize/2;
                inputCloud.conservativeResize(nPoints,3);
                p(0) = sx-cx;  p(1) = -(sy-cy);  p(2) = f;
                p(0) = p(0)/f;  p(1) = p(1)/f;  p(2) = p(2)/f;
                p(0) = p(0)*d;  p(1) = p(1)*d;  p(2) = p(2)*d;

                inputCloud.row(nPoints-1) << p;
            }
        }   

    //Denoising of the point cloud

    int np = 0;
    nPoints = 0;
    double dist = 0.0;
    double theta = (angle*PI)/180;

    for(int i = 0; i < inputCloud.rows(); i++)
    {
        np = 0;
        for(int j = 0;j < inputCloud.rows(); j++)
        {
            dist = (inputCloud.row(i) - inputCloud.row(j)).norm();
            if(dist <= radius)
                np++;
            if(np >= nPoints)
            {
              nPoints++;
              p(0) = inputCloud(i,0);
              p(1) = inputCloud(i,1);   //inputCloud(i,1)*cos(theta) - inputCloud(i,2)*sin(theta);
              p(2) = inputCloud(i,2);   //inputCloud(i,1)*sin(theta) + inputCloud(i,2)*cos(theta);
              outputCloud.conservativeResize(nPoints,3);
              outputCloud.row(nPoints-1) << p;
              break;
            }
        }
    }

    return outputCloud;
}


MatrixXd rgbPoint2Cloud(cv::Mat & inputRGB, vector<Point3D> &clickPoints, cv::Mat & inputDepth)
{
    MatrixXd inputCloud;
    int x, y;

    int boxsize = 1;
    double f = 525.0;
    double cx = 319.5;
    double cy = 239.5;

    //Down sampling of Pixels
    Point3D p;
    int d, sx,sy;
    Mat depthblock,imageblock;

    inputRGB.convertTo(inputRGB, CV_8UC1);

    for(int i = 0; i < clickPoints.size(); i++)
    {
        //s = inputRGB.at<cv::Vec3b>(clickPoints[i](1), clickPoints[i](0));
        x = clickPoints[i](0);
        y = clickPoints[i](1);

        depthblock = inputDepth(Rect(x,y,boxsize,boxsize));
        imageblock =   inputRGB(Rect(x,y,boxsize,boxsize));
        d = (ushort)depthblock.at<ushort>(boxsize/2,boxsize/2);
            qDebug() << "d: "  << d;

        sx = x + boxsize/2;
        sy = y + boxsize/2;

        inputCloud.conservativeResize(i+1,3);

        p(0) = sx-cx;  p(1) = -(sy-cy);  p(2) = f;
        p(0) = p(0)/f;  p(1) = p(1)/f;  p(2) = p(2)/f;
        p(0) = p(0)*d;  p(1) = p(1)*d;  p(2) = p(2)*d;

        inputCloud.row(i) << p;
        qDebug() << "inputCloud: " << inputCloud(i,0) << "  " << inputCloud(i,1) << "  " << inputCloud(i,2) << "  " ;

    }

    return inputCloud;
}


/*
MatrixXd kinectRotMat(MatrixXd & inputCloud)
{
    MatrixXd rotateCloud;
        rotateCloud(i,0) = inputCloud(i,0);
        p(1) = inputCloud(i,1)*cos(theta) - inputCloud(i,2)*sin(theta);
        p(2) = inputCloud(i,1)*sin(theta) + inputCloud(i,2)*cos(theta);

}
*/

cv::Mat colorExtraction(cv::Mat & imageBuff, Vector3D & colorBuff)
{
    int i,j;
    cv::Mat hsv, mask, gray;
    std::vector<cv::Mat> ch;
    cv::Mat outputPointImage;

    //Image definitions
    Vector3D lower,upper,val = Vector3D::Zero();
    lower << colorBuff[0]-10,colorBuff[1]-100,5;
    upper << colorBuff[0]+10,colorBuff[1]+100,255;

    //Element for Erosion and Dilation
    int opsize = 1;
    Mat element = getStructuringElement(MORPH_RECT, Size(2*opsize + 1, 2*opsize+1),Point(opsize,opsize));

    //Color_img is created according to input code
    cvtColor(imageBuff, hsv, CV_BGR2HSV);

    //Matrix for creating the lookup table
    Mat lut(256, 1, CV_8UC3);
    Mat hsvLut = Mat(hsv.rows, hsv.cols, CV_8UC3);

    //Creating a Look up table for setting image
    for (i=0; i<256; i++)
    {
        for (j=0; j<3; j++)
            if (lower(j) <= upper(j))
                if ((lower(j) <= i) && (i <= upper(j)))
                    val(j) = 255;
                else
                    val(j) = 0;
            else
                if ((i <= upper(j)) || (lower(j) <= i))
                    val(j) = 255;
                else
                    val(j) = 0;
                //Command to create the Look up table
                lut.row(i) = Scalar(val(0), val(1), val(2));
    }

    //Using Look-up-table(lut) to create color image
    LUT(hsv, lut, hsvLut);
        //qDebug() << "Lut: " << lut.rows << " x " << lut.cols;
        //qDebug() << "hsvLut: " << hsvLut.rows << " x " << hsvLut.cols;

    //Splitting the color image amoungst the 3 new images
    if(!hsvLut.data)
        qDebug() << "Error loading hsvLut";
    split(hsvLut, ch);

    //Mask Image formed by AND operation of 3 channels
    bitwise_and(ch[0], ch[1], mask);
    bitwise_and(mask, ch[2], mask);

    imageBuff.copyTo(outputPointImage,mask);
    erode(outputPointImage,outputPointImage,element);
    dilate(outputPointImage,outputPointImage,element);

    return outputPointImage;
}



#endif // POINTCLOUD_H
