Description
- this program was built solely for the purpose of tracking a colored marker in a 2D plane (e.g. Table)
- reference coordinate and marker color has to be calibrated
- more instruction will be added...


Requirements
- QT 5.3 win32 -msvc2012
- OpenCV 2.3
- OpenNI
- Kinect version 1


Instruction:

- change the OpenCV and OpenNi library Path in the project file to the correct location in your computer
- Edit #define SAMPLE_XML_PATH "C:/Users/Jim/qtworkspace/Kinect_MarkerTrackerCVFilter3P/Data/SamplesConfig.xml" 
  in the kinect.h file
 
- In the QT Creator, go to Projects and build with the correct kit
  *this current version has been tested with the "Desktop QT 5.3 MSVC2012" kit
- Clean the project
- Build/Rebuild
- Run
