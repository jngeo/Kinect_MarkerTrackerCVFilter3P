#ifndef CALIBRATION_H
#define CALIBRATION_H

//#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include "types.h"
#include <QImage>
#include <QDebug>

#include <fstream>
#include <iostream>
#include <stdio.h>

using std::vector;
using namespace cv;
using namespace std;

Vector3D colorCalib(Mat &image, vector<Point3D> &clickPoints)
{
  //Initialization
  cv::Mat hsv;
  cv::Vec3b s;
  Vector3D color = Vector3D::Zero();

  //Convert image to HSV
  cv::cvtColor(image,hsv,CV_BGR2HSV);
      //qDebug() << "rgbImage: " << image.rows << " x " << image.cols;
      //qDebug() << "hsvImage: " << hsv.rows << " x " << hsv.cols;

  //Get average HSV values over clicked points
  for(int i = 0; i < clickPoints.size(); i++)
    {
      s = hsv.at<cv::Vec3b>(clickPoints[i](1), clickPoints[i](0));	//hsv
      //s = image.at<cv::Vec3b>(clickPoints[i](1), clickPoints[i](0));	//rgb
      for(int j = 0; j < 3; j++)
        color(j) += s[j];
    }

  for(int i = 0; i < 3; i++)
    color(i) = color(i)/clickPoints.size();

  //Return HSV values
  return color;
}


Eigen::MatrixXd cloudSort(Eigen::MatrixXd & pointCloud)
{
  Point3D p;
  Eigen::MatrixXd pointCloudOut;

  int size = pointCloud.rows();
  for(int i = 1; i < size; i++)
    for(int j = 0; j < size-i; j++)
      {
    p = pointCloud.row(j+1) - pointCloud.row(j);
    if(p(0) < 0)
      pointCloud.row(j+1).swap(pointCloud.row(j));
    else if(p(0) == 0)
      if(p(1) < 0)
        pointCloud.row(j+1).swap(pointCloud.row(j));
      else if(p(1) == 0)
        if(p(2) < 0)
          pointCloud.row(j+1).swap(pointCloud.row(j));
      }

  pointCloudOut = pointCloud;
  return pointCloudOut;
}


Eigen::MatrixXd computeKMeans(const Eigen::MatrixXd & pointCloud, int clusterNum)
{
    Eigen::MatrixXd pointCloudSort;

    //Converting to cvMat format
    int size = pointCloud.rows();
        //qDebug() << "pointCloud Size: " << size ;
    cv::Mat cloudData(size,3,CV_32F);

    for(int i = 0; i < size; i++)
    {
      //qDebug() << "i: " << i;
      //qDebug() << "pointCloud: " << pointCloud(i,0) << "  " << pointCloud(i,1) << "  " << pointCloud(i,2) << "  " ;
      cloudData.at<float>(i,0) = (float) pointCloud(i,0);
      cloudData.at<float>(i,1) = (float) pointCloud(i,1);
      cloudData.at<float>(i,2) = (float) pointCloud(i,2);
    }

  //Inilializing variables for K-Means Clustering
  int attempts = 5;
  Mat labels;
  Mat centers;

  //Performing K-Means Clustering
  cv::kmeans(cloudData, clusterNum, labels, TermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 10000, 0.001), attempts, KMEANS_PP_CENTERS, centers);

  //Converting Mat to matrix format for clusters
   Eigen::MatrixXd clusters(clusterNum,3);
   for(int i = 0; i < clusterNum; i++)
   {
      clusters(i,0) = centers.at<float>(i,0);
      clusters(i,1) = centers.at<float>(i,1);
      clusters(i,2) = centers.at<float>(i,2);
    }

  //Sort cluster centers lexicographically
   cloudSort(clusters);
   pointCloudSort = cloudSort(clusters);

  //Return matrix of cluster centers
   return pointCloudSort;
}


//Function to calculate the angle made by Kinect with horizontal surface
double calibrate(MatrixXd & pointCloud)
{
  //Initialization
  Point3D v;
  Eigen::MatrixXd clusters;
  double theta = 0;

  //Getting the clusters
  clusters = computeKMeans(pointCloud, 2);
    qDebug() << "cluster center : " << clusters(0,0) << "  " << clusters(0,1) << "  " << clusters(0,2);
    qDebug() << "cluster center : " << clusters(1,0) << "  " << clusters(1,1) << "  " << clusters(1,2);

  //Getting the unit vector
  v = clusters.row(0) - clusters.row(1);
  v.normalize();
  if(v(2) < 0)
    v(2) = -v(2);

  //Computing the angle in degree
  theta = (acos(v(2))*180)/PI;

  //Return angle value
  return theta;
}


//Function to
Eigen::MatrixXd colorCenterTrack(MatrixXd & pointCloud)
{
  //Initialization
  Point3D v;
  Eigen::MatrixXd clusters;

  //Getting the clusters
  clusters = computeKMeans(pointCloud, 1);
    //qDebug() << "cluster center : " << clusters(0,0) << "  " << clusters(0,1) << "  " << clusters(0,2);

  return clusters;
}


//Filter Butterworth LP
//
Eigen::MatrixXd pointLowPassFilt(MatrixXd & buffPos, MatrixXd & buffPostFilt)
{
    int ButterOrder = 2;
    double A01 = 1, A02 = -0.6202, A03 = 0.2404;
    double B01 = 0.1551, B02 = 0.3101, B03 = 0.1551;

    MatrixXd Xdata(3,3), Ydata(2,3), lpFiltData(1,3);

    Xdata = buffPos;
    Ydata = buffPostFilt;

    for (int i = 0; i<3; i++ )
        lpFiltData(0,i) = ( B01*Xdata(2,i) + B02*Xdata(1,i) + B03*Xdata(0,i)
                                           - A02*Ydata(1,i) - A03*Ydata(0,i) ) / A01;

    return lpFiltData;
}

//get tranformation matrix
cv::Mat getTransMatrix(Mat &calibMat)
{
    ofstream fid;
    Mat centKinectMatTrans, centTableMatTrans;
    Mat kinCentroidTrans, tabCentroidTrans;
    Mat H, w, u, vt, v, ut, rotmat, transvect;
    Mat calibMatTrans, checkPlane, transvectMat;

    float dummy_data1[9] = { 0, 0, 0, 0, 400, 0, 600, 0, 0};
        Mat coordTable = Mat(3,3, CV_32F, dummy_data1);

    Mat centKinectMat(3,3, CV_32F);
    Mat centTableMat(3,3, CV_32F);

    Mat kinCentroid = Mat(1,3, CV_32F);
    Mat tabCentroid = Mat(1,3, CV_32F);
    for(int i = 0; i <3; i++)
    {
        kinCentroid(cv::Rect(i,0,1,1)) = ((float) calibMat.at<float>(0,i) + (float) calibMat.at<float>(1,i) + (float) calibMat.at<float>(2,i))/3;
        tabCentroid(cv::Rect(i,0,1,1)) = ((float) coordTable.at<float>(0,i) + (float) coordTable.at<float>(1,i) + (float) coordTable.at<float>(2,i))/3;
    }

    calibMat.copyTo(centKinectMat);
    coordTable.copyTo(centTableMat);

    for (int i = 0; i < 3 ; i++)
        for (int j = 0; j < 3 ; j++)
        {
            centKinectMat(cv::Rect(j,i,1,1)) = centKinectMat(cv::Rect(j,i,1,1)) - kinCentroid.at<float>(0,j);
            centTableMat(cv::Rect(j,i,1,1)) = centTableMat(cv::Rect(j,i,1,1)) - tabCentroid.at<float>(0,j);
        }

    cv::transpose(centKinectMat, centKinectMatTrans);
    cv::transpose(centTableMat, centTableMatTrans);

    H = centKinectMatTrans * centTableMat;
    cv::SVD::compute(H, w, u, vt);
    cv::transpose(vt, v);
    cv::transpose(u, ut);

    rotmat = v*ut;

    if(cv::determinant(rotmat) < 0)
    {
        for (int i = 1; i< 3; i++)
            v(cv::Rect(2,i,1,1)) = -1*v(cv::Rect(2,i,1,1));
        rotmat = v*ut;
    }


    cv::transpose(kinCentroid, kinCentroidTrans);
    cv::transpose(tabCentroid, tabCentroidTrans);

    transvect = ((-1)*rotmat)*kinCentroidTrans + tabCentroidTrans;

    cv::transpose(calibMat, calibMatTrans);
    cv::repeat(transvect, 1, 3, transvectMat);

    checkPlane = (rotmat*calibMatTrans) + transvectMat;
    cv::transpose(checkPlane, checkPlane);

    fid.open("RotMat");
    for (int i = 0; i < 3 ; i++)
    {
        fid << (float) rotmat.at<float>(i,0) << "   " << (float) rotmat.at<float>(i,1) << "   " << (float) rotmat.at<float>(i,2) << "   " << endl;
    }
    fid.close();

    fid.open("transvect");
    fid << (float) transvect.at<float>(0,0) << "   " << (float) transvect.at<float>(1,0) << "   " << (float)  transvect.at<float>(2,0) << "   " << endl;
    fid.close();

    fid.open("checkPlane");
        fid << (float) checkPlane.at<float>(0,0) << "   " << (float) checkPlane.at<float>(0,1) << "   " << (float) checkPlane.at<float>(0,2) << "   " << endl;
        fid << (float) checkPlane.at<float>(1,0) << "   " << (float) checkPlane.at<float>(1,1) << "   " << (float) checkPlane.at<float>(1,2) << "   " << endl;
        fid << (float) checkPlane.at<float>(2,0) << "   " << (float) checkPlane.at<float>(2,1) << "   " << (float) checkPlane.at<float>(2,2) << "   " << endl;
    fid.close();

    return rotmat;
    //return centTableMat;
}

//Function to
double absoluteVelocity(MatrixXd & matrixVelocity)
{
  //Initialization
  double absv;

  absv = sqrt( pow(matrixVelocity(0,0),2) + pow(matrixVelocity(0,1),2) + pow(matrixVelocity(0,2),2)   );

  return absv;
}

#endif // CALIBRATION_H
