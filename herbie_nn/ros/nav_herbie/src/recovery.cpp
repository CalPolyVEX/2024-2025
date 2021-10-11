#include "navigation.h"

#include <dynamic_reconfigure/DoubleParameter.h>
#include <dynamic_reconfigure/Reconfigure.h>
#include <dynamic_reconfigure/Config.h>

#include <pcl_conversions/pcl_conversions.h>
#include <sensor_msgs/PointCloud2.h>

extern int sim_mode;
extern ros::Timer goal_timer;

using namespace std;

void Navigation::get_major_heading() {
   float distance = 0;
   int temp_localization_index;
   int counter = 0;
   int found1 = 0;

   //get the current odom to base_link transform
   geometry_msgs::TransformStamped transformStamped;
   transformStamped = tfBuffer.lookupTransform("odom", "base_link", ros::Time(0), ros::Duration(0.5));

   //find the first localization pose that is over 1 meter away previously
   temp_localization_index = localization_index;
   while(counter < (LOCALIZATION_ARRAY_SIZE-1)) {
      float temp_x1 = localization_pose[temp_localization_index].position.x;
      float temp_y1 = localization_pose[temp_localization_index].position.y;

      float temp_x2 = transformStamped.transform.translation.x;
      float temp_y2 = transformStamped.transform.translation.y;

      float dist = pow((pow(temp_x1 - temp_x2, 2) + pow(temp_y1 - temp_y2, 2)), .5);

      if (dist < 1.0) {
         temp_localization_index -= 20;
         counter+=20;

         if (temp_localization_index < 0)
            temp_localization_index += LOCALIZATION_ARRAY_SIZE;

      } else {
         found1 = 1;
         break;
      }
   }

   if (found1) {
      //there was a localization pose 1 meter ago
   } else {
      return;
   }
}


