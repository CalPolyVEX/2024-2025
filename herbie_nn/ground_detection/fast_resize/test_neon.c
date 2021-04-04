#include<stdio.h>
#include<arm_neon.h>
#include<stdlib.h>

int main() {
   unsigned char arr[1024];
   uint16x8_t u0;
   uint16x8_t u0_temp;
   uint16x8_t v0;
   uint16x8_t v0_temp;

   for(int i=0;i<1024;i++)
   {
      arr[i] = rand() & 0xFF;
   }

   for(int i=0;i<8;i++)
   {
      //u0 = buffer[counter] + buffer[counter + 4];     //add the U values (u0 and u2)
      u0 = vsetq_lane_u16(arr[i   ], u0, 0);
      u0 = vsetq_lane_u16(arr[i+12], u0, 1);
      u0 = vsetq_lane_u16(arr[i+24], u0, 2);
      u0 = vsetq_lane_u16(arr[i+36], u0, 3);
      u0 = vsetq_lane_u16(arr[i+48], u0, 4);
      u0 = vsetq_lane_u16(arr[i+60], u0, 5);
      u0 = vsetq_lane_u16(arr[i+72], u0, 6);
      u0 = vsetq_lane_u16(arr[i+84], u0, 7);

      u0_temp = vsetq_lane_u16(arr[i    + 4], u0_temp, 0);
      u0_temp = vsetq_lane_u16(arr[i+12 + 4], u0_temp, 1);
      u0_temp = vsetq_lane_u16(arr[i+24 + 4], u0_temp, 2);
      u0_temp = vsetq_lane_u16(arr[i+36 + 4], u0_temp, 3);
      u0_temp = vsetq_lane_u16(arr[i+48 + 4], u0_temp, 4);
      u0_temp = vsetq_lane_u16(arr[i+60 + 4], u0_temp, 5);
      u0_temp = vsetq_lane_u16(arr[i+72 + 4], u0_temp, 6);
      u0_temp = vsetq_lane_u16(arr[i+84 + 4], u0_temp, 7);

      //v0 = buffer[counter + 2] + buffer[counter + 6]; //add the V values (v0 and v2)
      v0 = vsetq_lane_u16(arr[i    + 2], v0, 0);
      v0 = vsetq_lane_u16(arr[i+12 + 2], v0, 1);
      v0 = vsetq_lane_u16(arr[i+24 + 2], v0, 2);
      v0 = vsetq_lane_u16(arr[i+36 + 2], v0, 3);
      v0 = vsetq_lane_u16(arr[i+48 + 2], v0, 4);
      v0 = vsetq_lane_u16(arr[i+60 + 2], v0, 5);
      v0 = vsetq_lane_u16(arr[i+72 + 2], v0, 6);
      v0 = vsetq_lane_u16(arr[i+84 + 2], v0, 7);

      v0_temp = vsetq_lane_u16(arr[i    + 6], v0_temp, 0);
      v0_temp = vsetq_lane_u16(arr[i+12 + 6], v0_temp, 1);
      v0_temp = vsetq_lane_u16(arr[i+24 + 6], v0_temp, 2);
      v0_temp = vsetq_lane_u16(arr[i+36 + 6], v0_temp, 3);
      v0_temp = vsetq_lane_u16(arr[i+48 + 6], v0_temp, 4);
      v0_temp = vsetq_lane_u16(arr[i+60 + 6], v0_temp, 5);
      v0_temp = vsetq_lane_u16(arr[i+72 + 6], v0_temp, 6);
      v0_temp = vsetq_lane_u16(arr[i+84 + 6], v0_temp, 7);
   }


   u0 = u0 + u0_temp;
   v0 = v0 + v0_temp;

   /* float a[4] = {1,2,3,4}; */
   /* float b[4] = {5,6,7,8}; */
   /* float32x4_t in1,in2,out1; */
   /* in1 = vld1q_f32(a); */
   /* in2 = vld1q_f32(b); */
   /* out1 = in1 + in2; */

   printf ("%d\n", vget_lane_u16 (vget_low_u16(u0), 0));
   printf ("%d\n", vget_lane_u16 (vget_low_u16(v0), 0));

   return 0;
}
