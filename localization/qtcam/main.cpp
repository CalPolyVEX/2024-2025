#include <iostream>
#include <QtWidgets/QApplication>
#include <QDateTime>
#include <QtWidgets/QWidget>
#include <QIcon>
#include <QStandardPaths>
#include "qtquick2applicationviewer.h"
#include "cameraproperty.h"
#include "videostreaming.h"
#include "uvccamera.h"
#include "seecam_10cug_bayer.h"
#include "seecam_10cug_m.h"
#include "seecam_11cug.h"
#include "seecam_cu50.h"
#include "seecam_cu51.h"
#include "seecam_cu80.h"
#include "seecam_81.h"
#include "see3cam_cu40.h"
#include "see3cam_cu130.h"
#include "see3cam_cu135.h"
#include "see3cam_130.h"
#include "see3cam_cu20.h"
#include "seecam_ar0130.h"
#include "seecam_cu30.h"
// Added by Sankari : 07 Feb 2017
#include "see3cam_30.h"
#include "uvcExtCx3sni.h"
#include "ascella.h"
#include "about.h"
#include "common.h"
#include "common_enums.h"

//*! \mainpage Qtcam - A econ's camera product
// *
// * \section See3CAM_10CUG
// * Mono Camera
// * \ref See3CAM_10CUG
// * \section See3CAM_10CUG
// * Bayer Camera
extern int buf_val;

int main(int argc, char *argv[])
{
   //Create a object for Camera property
   Cameraproperty camProperty;    
   
   /* JS */
   uvccamera test_cam;
   See3CAM_CU20 s;
   QString cu20 = "See3CAM_CU20";

   camProperty.checkforDevice(); //check for all devices

   //search for the string See3CAM_CU20
   if (camProperty.availableCam.contains(cu20) == true) {
      int i = camProperty.availableCam.indexOf(cu20);
      QString num = QString::number(i);
      QTextStream(stdout) << "---camera index:" << num << endl; 

      camProperty.setCurrentDevice(num, cu20);
   } else {
      QTextStream(stdout) << "See3Cam_CU20 not found." << endl; 
      exit(0);
   }

   camProperty.openHIDDevice(cu20);
   if (buf_val == 0) {
      QTextStream(stdout) << "---initialization failed---" << endl;
      exit(0);
   }

   QTextStream(stdout) << "---set HDR sensor mode...";
   if (s.setSensorMode(See3CAM_CU20::SENSOR_HDR_DLO) == true) {
      QTextStream(stdout) << "successful." << endl << endl;
   } else {
      QTextStream(stdout) << "failed." << endl;
   }

   QTextStream(stdout) << "---get HDR sensor mode...";
   if (s.getSensorMode() == true) {
      QTextStream(stdout) << buf_val << endl;
   }

   QTextStream(stdout) << "---set light sensor mode...";
   if (s.setLSCMode(See3CAM_CU20::LSC_AUTO) == true) {
      QTextStream(stdout) << "successful." << endl << endl;
   } else {
      QTextStream(stdout) << "failed." << endl;
   }

   QTextStream(stdout) << "---get light sensor mode...";
   if (s.getLSCMode() == true) {
      QTextStream(stdout) << buf_val << endl;
   }

   close(uvccamera::hid_fd);
   exit(0);
   return 0;
}
