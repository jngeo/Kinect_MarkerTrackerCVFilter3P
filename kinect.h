#ifndef KINECT_H
#define KINECT_H

#include <QDebug>
#include <XnCppWrapper.h>
#include "types.h"

using namespace xn;

#define SAMPLE_XML_PATH "C:/Users/Jim/qtworkspace/Kinect_MarkerTrackerCVFilter3P/Data/SamplesConfig.xml"		//eddit jim

//OpenNI Initialization
    XnStatus nRetVal = XN_STATUS_OK;
    Context context;
    ScriptNode scriptNode;
    EnumerationErrors errors;
    DepthGenerator depth;
    ImageGenerator image;
    DepthMetaData depthMD;
    ImageMetaData imageMD;

//Function to initialize Kinect
void kinectInit()
{
    nRetVal = context.InitFromXmlFile(SAMPLE_XML_PATH, &errors);
    if (nRetVal == XN_STATUS_NO_NODE_PRESENT)
    {
      XnChar strError[1024];
      errors.ToString(strError, 1024);
      qDebug() << strError << endl;
    }
    else if (nRetVal != XN_STATUS_OK)
    {
      qDebug() << "Open failed: " << xnGetStatusString(nRetVal) << endl;
    }

    nRetVal = context.FindExistingNode(XN_NODE_TYPE_DEPTH, depth);
    if(nRetVal != XN_STATUS_OK)
    {
      qDebug() << "Find depth generator failed: " << xnGetStatusString(nRetVal) << endl;
    }

    nRetVal = context.FindExistingNode(XN_NODE_TYPE_IMAGE, image);
    if(nRetVal != XN_STATUS_OK)
    {
      qDebug() << "Find image generator failed: " << xnGetStatusString(nRetVal) << endl;
    }
}

void kinectUpdate(cv::Mat & m_image, cv::Mat & m_depth)
{
    //wait and error processing
    nRetVal = context.WaitAnyUpdateAll();
    if (nRetVal != XN_STATUS_OK)
    {
      qDebug() << "UpdateData failed: " << xnGetStatusString(nRetVal) << endl;
      return;
    }

    image.GetMetaData(imageMD);
    depth.GetMetaData(depthMD);

    //Creating Mat
    cv::Mat dept(480,640,CV_16UC1,(unsigned short*)depthMD.WritableData()); //dept(480,640,CV_16UC1,(unsigned short*)depthMD.WritableData());
    //cv::normalize(dept, dept, 0, 255, CV_MINMAX, CV_8UC1);

    cv::Mat img(480,640,CV_8UC3, (uchar*)imageMD.WritableData());

    m_image = img;
    m_depth = dept;

    //convert color space RGB2BGR
    cv::cvtColor(m_image,m_image,CV_RGB2BGR);
    //cv::cvtColor(m_depth,m_depth,CV_RGB2BGR);
}

#endif // KINECT_H
