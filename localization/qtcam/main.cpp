/*
 * main.cpp -- the main loop and the interface connection
 * Copyright © 2015  e-con Systems India Pvt. Limited
 *
 * This file is part of Qtcam.
 *
 * Qtcam is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Qtcam is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Qtcam. If not, see <http://www.gnu.org/licenses/>.
 */

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

int main(int argc, char *argv[])
{
    //Create a object for Camera property
    Cameraproperty camProperty;    

    /* JS */
    if (0 == 0) {
       QTextStream(stdout) << "test";
       uvccamera test_cam;
       See3CAM_CU20 s;

       camProperty.checkforDevice(); //check for all devices

       QString num = "2";
       QString cu20 = "See3CAM_CU20";
       camProperty.setCurrentDevice(num, cu20);

       camProperty.openHIDDevice(cu20);

       QTextStream(stdout) << "---set sensor mode: " << s.setSensorMode(See3CAM_CU20::SENSOR_HDR_DLO) << endl;
       //QTextStream(stdout) << s.setSensorMode(See3CAM_CU20::SENSOR_STANDARD) << endl;
       if (argc > 1)
          QTextStream(stdout) << s.setOrientation(false, true) << endl;
       else
          QTextStream(stdout) << s.setOrientation(false, false) << endl;

       QTextStream(stdout) << "---get sensor mode: " << s.getSensorMode() << endl;

       //uvccam.exitExtensionUnit();
       //test_cam.exitExtensionUnit();
       close(uvccamera::hid_fd);
       exit(0);
    }
    return 0;

    /* end JS */
}
