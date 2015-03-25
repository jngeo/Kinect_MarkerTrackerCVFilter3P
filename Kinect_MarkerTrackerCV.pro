#-------------------------------------------------
#
# Project created by QtCreator 2014-11-04T14:25:14
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Kinect_MarkerTrackerCV
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    imagemouseclass.cpp

HEADERS  += mainwindow.h \
    imagemouseclass.h \
    types.h \
    kinect.h \
    calibration.h \
    pointcloud.h

FORMS    += mainwindow.ui


win32 {
message("Using win32 configuration")

INCLUDEPATH += Eigen

OPENCV_PATH = "C:/OpenCV2.3"
OPENNI_PATH = "C:/Program Files (x86)/OpenNI"

INCLUDEPATH += "$$OPENCV_PATH/build/include"
LIBS_PATH += "$$OPENCV_PATH/build/x86/vc10/lib" #project compiled using Visual C++ 2010 32bit compiler
    CONFIG(debug, debug|release) {
    LIBS     += -L$$LIBS_PATH \
                -lopencv_core230d \
                -lopencv_highgui230d \
                -lopencv_imgproc230d \
                -lopencv_objdetect230d \
                -lopencv_legacy230d
    }

    CONFIG(release, debug|release) {
    LIBS     += -L$$LIBS_PATH \
                -lopencv_core230 \
                -lopencv_highgui230 \
                -lopencv_imgproc230 \
                -lopencv_objdetect230 \
                -lopencv_legacy230
    }


INCLUDEPATH += "$$OPENNI_PATH/Include"
NILIBS_PATH += "$$OPENNI_PATH/Lib" #project compiled using Visual C++ 2010 32bit compiler
    CONFIG(debug, debug|release) {
    LIBS     += -L$$NILIBS_PATH \
                -lopenNI
    }
    CONFIG(release, debug|release) {
    LIBS     += -L$$NILIBS_PATH \
                -lopenNI
    }
}




