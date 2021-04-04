#include<stdio.h>
#include<arm_neon.h>
#include<stdlib.h>
#include <time.h>

int main() {
   FILE* ptr;
   unsigned char arr[1920 * 1080 * 2];
   unsigned char img1[100];
   unsigned char img2[100];
   int16x8_t u0, u2;
   int16x8_t u0_temp, u2_next;
   int16x8_t v0, v2;
   int16x8_t v0_temp, v2_next;
   int16x8_t y0_a, y0_b;
   int16x8_t y0_a_temp, y0_b_temp;
   float32x4_t y_const;
   
   ptr = fopen("./beach.uyvy","rb");  // r for read, b for binary

   if (!ptr) {
      printf ("Unable to open file.\n");
   }

   // read bytes to the buffer 
   int x = fread(arr,1,sizeof(arr),ptr);

   if (x != sizeof(arr)) {
      printf ("read error %d\n", x);
   }

   fclose(ptr);

   int row_size = 3840; 
   int temp_index = 0;

   clock_t begin = clock();

   for (int j=0;j<1000;j++) {
      int counter = 0;

      while(counter < (96*40*360))
      {
         //u0 = buffer[counter] + buffer[counter + 4];     //add the U values (u0 and u2)
         u0 = vsetq_lane_s16(arr[counter   ], u0, 0);
         u0 = vsetq_lane_s16(arr[counter+12], u0, 1);
         u0 = vsetq_lane_s16(arr[counter+24], u0, 2);
         u0 = vsetq_lane_s16(arr[counter+36], u0, 3);
         u0 = vsetq_lane_s16(arr[counter+48], u0, 4);
         u0 = vsetq_lane_s16(arr[counter+60], u0, 5);
         u0 = vsetq_lane_s16(arr[counter+72], u0, 6);
         u0 = vsetq_lane_s16(arr[counter+84], u0, 7);

         u2 = vsetq_lane_s16(arr[counter    + 4], u2, 0);
         u2 = vsetq_lane_s16(arr[counter+12 + 4], u2, 1);
         u2 = vsetq_lane_s16(arr[counter+24 + 4], u2, 2);
         u2 = vsetq_lane_s16(arr[counter+36 + 4], u2, 3);
         u2 = vsetq_lane_s16(arr[counter+48 + 4], u2, 4);
         u2 = vsetq_lane_s16(arr[counter+60 + 4], u2, 5);
         u2 = vsetq_lane_s16(arr[counter+72 + 4], u2, 6);
         u2 = vsetq_lane_s16(arr[counter+84 + 4], u2, 7);

         u0 += u2;

         //v0 = buffer[counter + 2] + buffer[counter + 6]; //add the V values (v0 and v2)
         v0 = vsetq_lane_s16(arr[counter    + 2], v0, 0);
         v0 = vsetq_lane_s16(arr[counter+12 + 2], v0, 1);
         v0 = vsetq_lane_s16(arr[counter+24 + 2], v0, 2);
         v0 = vsetq_lane_s16(arr[counter+36 + 2], v0, 3);
         v0 = vsetq_lane_s16(arr[counter+48 + 2], v0, 4);
         v0 = vsetq_lane_s16(arr[counter+60 + 2], v0, 5);
         v0 = vsetq_lane_s16(arr[counter+72 + 2], v0, 6);
         v0 = vsetq_lane_s16(arr[counter+84 + 2], v0, 7);

         v2 = vsetq_lane_s16(arr[counter    + 6], v2, 0);
         v2 = vsetq_lane_s16(arr[counter+12 + 6], v2, 1);
         v2 = vsetq_lane_s16(arr[counter+24 + 6], v2, 2);
         v2 = vsetq_lane_s16(arr[counter+36 + 6], v2, 3);
         v2 = vsetq_lane_s16(arr[counter+48 + 6], v2, 4);
         v2 = vsetq_lane_s16(arr[counter+60 + 6], v2, 5);
         v2 = vsetq_lane_s16(arr[counter+72 + 6], v2, 6);
         v2 = vsetq_lane_s16(arr[counter+84 + 6], v2, 7);

         v0 += v2;

         /* next_counter = counter + row_size * 2; */
         int next_counter = counter + row_size * 2; 

         /* u0 += buffer[next_counter] + buffer[next_counter + 4];     //add the U values */
         u0_temp = vsetq_lane_s16(arr[next_counter   ], u0_temp, 0);
         u0_temp = vsetq_lane_s16(arr[next_counter+12], u0_temp, 1);
         u0_temp = vsetq_lane_s16(arr[next_counter+24], u0_temp, 2);
         u0_temp = vsetq_lane_s16(arr[next_counter+36], u0_temp, 3);
         u0_temp = vsetq_lane_s16(arr[next_counter+48], u0_temp, 4);
         u0_temp = vsetq_lane_s16(arr[next_counter+60], u0_temp, 5);
         u0_temp = vsetq_lane_s16(arr[next_counter+72], u0_temp, 6);
         u0_temp = vsetq_lane_s16(arr[next_counter+84], u0_temp, 7);

         u0 = u0 + u0_temp;

         u2_next = vsetq_lane_s16(arr[next_counter    + 4], u2_next, 0);
         u2_next = vsetq_lane_s16(arr[next_counter+12 + 4], u2_next, 1);
         u2_next = vsetq_lane_s16(arr[next_counter+24 + 4], u2_next, 2);
         u2_next = vsetq_lane_s16(arr[next_counter+36 + 4], u2_next, 3);
         u2_next = vsetq_lane_s16(arr[next_counter+48 + 4], u2_next, 4);
         u2_next = vsetq_lane_s16(arr[next_counter+60 + 4], u2_next, 5);
         u2_next = vsetq_lane_s16(arr[next_counter+72 + 4], u2_next, 6);
         u2_next = vsetq_lane_s16(arr[next_counter+84 + 4], u2_next, 7);

         u0 = u0 + u2_next;

         /* v0 += buffer[next_counter + 2] + buffer[next_counter + 6]; //add the V values */
         v0_temp = vsetq_lane_s16(arr[next_counter    + 2], v0_temp, 0);
         v0_temp = vsetq_lane_s16(arr[next_counter+12 + 2], v0_temp, 1);
         v0_temp = vsetq_lane_s16(arr[next_counter+24 + 2], v0_temp, 2);
         v0_temp = vsetq_lane_s16(arr[next_counter+36 + 2], v0_temp, 3);
         v0_temp = vsetq_lane_s16(arr[next_counter+48 + 2], v0_temp, 4);
         v0_temp = vsetq_lane_s16(arr[next_counter+60 + 2], v0_temp, 5);
         v0_temp = vsetq_lane_s16(arr[next_counter+72 + 2], v0_temp, 6);
         v0_temp = vsetq_lane_s16(arr[next_counter+84 + 2], v0_temp, 7);

         v0 = v0 + v0_temp;

         v2_next = vsetq_lane_s16(arr[next_counter    + 6], v2_next, 0);
         v2_next = vsetq_lane_s16(arr[next_counter+12 + 6], v2_next, 1);
         v2_next = vsetq_lane_s16(arr[next_counter+24 + 6], v2_next, 2);
         v2_next = vsetq_lane_s16(arr[next_counter+36 + 6], v2_next, 3);
         v2_next = vsetq_lane_s16(arr[next_counter+48 + 6], v2_next, 4);
         v2_next = vsetq_lane_s16(arr[next_counter+60 + 6], v2_next, 5);
         v2_next = vsetq_lane_s16(arr[next_counter+72 + 6], v2_next, 6);
         v2_next = vsetq_lane_s16(arr[next_counter+84 + 6], v2_next, 7);

         v0 = v0 + v2_next;

         //y0 = buffer[counter + 1];
         y0_a = vsetq_lane_s16(arr[counter    + 1], y0_a, 0);
         y0_a = vsetq_lane_s16(arr[counter+12 + 1], y0_a, 1);
         y0_a = vsetq_lane_s16(arr[counter+24 + 1], y0_a, 2);
         y0_a = vsetq_lane_s16(arr[counter+36 + 1], y0_a, 3);
         y0_a = vsetq_lane_s16(arr[counter+48 + 1], y0_a, 4);
         y0_a = vsetq_lane_s16(arr[counter+60 + 1], y0_a, 5);
         y0_a = vsetq_lane_s16(arr[counter+72 + 1], y0_a, 6);
         y0_a = vsetq_lane_s16(arr[counter+84 + 1], y0_a, 7);

         //y2 = buffer[counter + 5];
         y0_a_temp = vsetq_lane_s16(arr[counter    + 5], y0_a_temp, 0);
         y0_a_temp = vsetq_lane_s16(arr[counter+12 + 5], y0_a_temp, 1);
         y0_a_temp = vsetq_lane_s16(arr[counter+24 + 5], y0_a_temp, 2);
         y0_a_temp = vsetq_lane_s16(arr[counter+36 + 5], y0_a_temp, 3);
         y0_a_temp = vsetq_lane_s16(arr[counter+48 + 5], y0_a_temp, 4);
         y0_a_temp = vsetq_lane_s16(arr[counter+60 + 5], y0_a_temp, 5);
         y0_a_temp = vsetq_lane_s16(arr[counter+72 + 5], y0_a_temp, 6);
         y0_a_temp = vsetq_lane_s16(arr[counter+84 + 5], y0_a_temp, 7);

         y0_a += y0_a_temp;

         //2 rows down
         //y0 += buffer[next_counter + 1];
         //y2 += buffer[next_counter + 5];
         y0_a_temp = vsetq_lane_s16(arr[next_counter    + 1], y0_a_temp, 0);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+12 + 1], y0_a_temp, 1);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+24 + 1], y0_a_temp, 2);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+36 + 1], y0_a_temp, 3);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+48 + 1], y0_a_temp, 4);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+60 + 1], y0_a_temp, 5);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+72 + 1], y0_a_temp, 6);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+84 + 1], y0_a_temp, 7);

         y0_a += y0_a_temp;

         y0_a_temp = vsetq_lane_s16(arr[next_counter    + 5], y0_a_temp, 0);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+12 + 5], y0_a_temp, 1);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+24 + 5], y0_a_temp, 2);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+36 + 5], y0_a_temp, 3);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+48 + 5], y0_a_temp, 4);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+60 + 5], y0_a_temp, 5);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+72 + 5], y0_a_temp, 6);
         y0_a_temp = vsetq_lane_s16(arr[next_counter+84 + 5], y0_a_temp, 7);

         y0_a += y0_a_temp;

         //divide each sum by 4
         u0 = vshrq_n_s16(u0,2);
         v0 = vshrq_n_s16(v0,2);
         y0_a = vshrq_n_s16(y0_a,2);

         //subtract scalars
         y0_a -= vdupq_n_s16(-16);
         u0 -= vdupq_n_s16(-128);
         v0 -= vdupq_n_s16(-128);

         ///////////////////////////////////////////////////////////////
         //Convert to BGR values
         //convert 4 of the BGR set at a time starting with the lower set of 4 
         //convert 4 low (4 low out of 8) Red values
         float32x4_t y0_temp_low = vcvtq_f32_s32(vmovl_s16(vget_low_s16(y0_a))); //convert lower 4 to 4 floats
         y0_temp_low *= vdupq_n_f32(1.164); //this variable will get reused in the computation

         float32x4_t r_low = y0_temp_low;
         r_low = vmlaq_f32(r_low, vcvtq_f32_s32(vmovl_s16(vget_low_s16(v0))), vdupq_n_f32(1.596));

         //convert 4 low (4 low out of 8) Green values
         float32x4_t g_low = y0_temp_low;
         g_low = vmlaq_f32(g_low, vcvtq_f32_s32(vmovl_s16(vget_low_s16(u0))), vdupq_n_f32(-0.392));
         g_low = vmlaq_f32(g_low, vcvtq_f32_s32(vmovl_s16(vget_low_s16(v0))), vdupq_n_f32(-0.813));
         y_const = g_low;

         //convert 4 low (4 low out of 8) Blue values
         float32x4_t b_low = y0_temp_low;
         b_low = vmlaq_f32(b_low, vcvtq_f32_s32(vmovl_s16(vget_low_s16(u0))), vdupq_n_f32(2.017));

         uint32x4_t b_low_int = vcvtq_u32_f32(b_low); //convert from floats to ints, rounding toward zero
         uint32x4_t g_low_int = vcvtq_u32_f32(g_low);
         uint32x4_t r_low_int = vcvtq_u32_f32(r_low);

         for (int i=0; i<4; i++) {
            //store blue values
            float val = vgetq_lane_u32(b_low_int, i);

            if (val > 255)
               val = 255;
            else if (val < 0)
               val = 0;

            img1[temp_index] = (char) val;
            temp_index++;

            //store green values
            val = vgetq_lane_u32(g_low_int, i);

            if (val > 255)
               val = 255;
            else if (val < 0)
               val = 0;

            img1[temp_index] = (char) val;
            temp_index++;

            //store red values
            val = vgetq_lane_u32(r_low_int, i);

            if (val > 255)
               val = 255;
            else if (val < 0)
               val = 0;

            img1[temp_index] = (char) val;
            temp_index++;
         }

         //////////////convert high set of 4/////////////////////////////////////////
         //convert 4 high (4 high out of 8) Red values
         float32x4_t y0_temp_high = vcvtq_f32_s32(vmovl_s16(vget_high_s16(y0_a))); //convert to 4 floats
         y0_temp_high *= vdupq_n_f32(1.164); 

         float32x4_t r_high = y0_temp_high;
         r_high = vmlaq_f32(r_high, vcvtq_f32_s32(vmovl_s16(vget_high_s16(v0))), vdupq_n_f32(1.596));

         //convert 4 high (4 high out of 8) Green values
         float32x4_t g_high = y0_temp_high;
         g_high = vmlaq_f32(g_high, vcvtq_f32_s32(vmovl_s16(vget_high_s16(u0))), vdupq_n_f32(-0.392));
         g_high = vmlaq_f32(g_high, vcvtq_f32_s32(vmovl_s16(vget_high_s16(v0))), vdupq_n_f32(-0.813));
         y_const = g_high;

         //convert 4 high (4 high out of 8) Blue values
         float32x4_t b_high = y0_temp_high;
         b_high = vmlaq_f32(b_high, vcvtq_f32_s32(vmovl_s16(vget_high_s16(u0))), vdupq_n_f32(2.017));

         uint32x4_t b_high_int = vcvtq_u32_f32(b_high); //convert from floats to ints, rounding toward zero
         uint32x4_t g_high_int = vcvtq_u32_f32(g_high);
         uint32x4_t r_high_int = vcvtq_u32_f32(r_high);

         for (int i=0; i<4; i++) {
            //store blue values
            float val = vgetq_lane_u32(b_high_int, i);

            if (val > 255)
               val = 255;
            else if (val < 0)
               val = 0;

            img1[temp_index] = (char) val;
            temp_index++;

            //store green values
            val = vgetq_lane_u32(g_high_int, i);

            if (val > 255)
               val = 255;
            else if (val < 0)
               val = 0;

            img1[temp_index] = (char) val;
            temp_index++;

            //store red values
            val = vgetq_lane_u32(r_high_int, i);

            if (val > 255)
               val = 255;
            else if (val < 0)
               val = 0;

            img1[temp_index] = (char) val;
            temp_index++;
         }
         temp_index = 0;
         // float r = 1.164 * avg_y                 + 1.596 * avg_v;
         // float g = 1.164 * avg_y - 0.392 * avg_u - 0.813 * avg_v;
         // float b = 1.164 * avg_y + 2.017 * avg_u;



         //////////////////////////////////////////////////////////////////
         //Compute the BGR values here before continuing
         //////////////////////////////////////////////////////////////////




         //second set of U in 'counter' row
         u0_temp = vsetq_lane_s16(arr[next_counter    + 8], u0_temp, 0);
         u0_temp = vsetq_lane_s16(arr[next_counter+12 + 8], u0_temp, 1);
         u0_temp = vsetq_lane_s16(arr[next_counter+24 + 8], u0_temp, 2);
         u0_temp = vsetq_lane_s16(arr[next_counter+36 + 8], u0_temp, 3);
         u0_temp = vsetq_lane_s16(arr[next_counter+48 + 8], u0_temp, 4);
         u0_temp = vsetq_lane_s16(arr[next_counter+60 + 8], u0_temp, 5);
         u0_temp = vsetq_lane_s16(arr[next_counter+72 + 8], u0_temp, 6);
         u0_temp = vsetq_lane_s16(arr[next_counter+84 + 8], u0_temp, 7);

         u2 += u0_temp;

         //second set of V in 'counter' row
         v0_temp = vsetq_lane_s16(arr[next_counter    + 10], v0_temp, 0);
         v0_temp = vsetq_lane_s16(arr[next_counter+12 + 10], v0_temp, 1);
         v0_temp = vsetq_lane_s16(arr[next_counter+24 + 10], v0_temp, 2);
         v0_temp = vsetq_lane_s16(arr[next_counter+36 + 10], v0_temp, 3);
         v0_temp = vsetq_lane_s16(arr[next_counter+48 + 10], v0_temp, 4);
         v0_temp = vsetq_lane_s16(arr[next_counter+60 + 10], v0_temp, 5);
         v0_temp = vsetq_lane_s16(arr[next_counter+72 + 10], v0_temp, 6);
         v0_temp = vsetq_lane_s16(arr[next_counter+84 + 10], v0_temp, 7);

         v2 += v0_temp;

         //second set of U in 'nextcounter' row
         u0_temp = vsetq_lane_s16(arr[next_counter    + 8], u0_temp, 0);
         u0_temp = vsetq_lane_s16(arr[next_counter+12 + 8], u0_temp, 1);
         u0_temp = vsetq_lane_s16(arr[next_counter+24 + 8], u0_temp, 2);
         u0_temp = vsetq_lane_s16(arr[next_counter+36 + 8], u0_temp, 3);
         u0_temp = vsetq_lane_s16(arr[next_counter+48 + 8], u0_temp, 4);
         u0_temp = vsetq_lane_s16(arr[next_counter+60 + 8], u0_temp, 5);
         u0_temp = vsetq_lane_s16(arr[next_counter+72 + 8], u0_temp, 6);
         u0_temp = vsetq_lane_s16(arr[next_counter+84 + 8], u0_temp, 7);

         u2 += u2_next + u0_temp;

         //second set of V in 'nextcounter' row
         v0_temp = vsetq_lane_s16(arr[next_counter    + 10], v0_temp, 0);
         v0_temp = vsetq_lane_s16(arr[next_counter+12 + 10], v0_temp, 1);
         v0_temp = vsetq_lane_s16(arr[next_counter+24 + 10], v0_temp, 2);
         v0_temp = vsetq_lane_s16(arr[next_counter+36 + 10], v0_temp, 3);
         v0_temp = vsetq_lane_s16(arr[next_counter+48 + 10], v0_temp, 4);
         v0_temp = vsetq_lane_s16(arr[next_counter+60 + 10], v0_temp, 5);
         v0_temp = vsetq_lane_s16(arr[next_counter+72 + 10], v0_temp, 6);
         v0_temp = vsetq_lane_s16(arr[next_counter+84 + 10], v0_temp, 7);

         v2 += v2_next + v0_temp;

         /////////////////////////////////////////////////////////
         //Second set of y values
         //y0_b = buffer[counter + 7];
         y0_b = vsetq_lane_s16(arr[counter    + 7], y0_b, 0);
         y0_b = vsetq_lane_s16(arr[counter+12 + 7], y0_b, 1);
         y0_b = vsetq_lane_s16(arr[counter+24 + 7], y0_b, 2);
         y0_b = vsetq_lane_s16(arr[counter+36 + 7], y0_b, 3);
         y0_b = vsetq_lane_s16(arr[counter+48 + 7], y0_b, 4);
         y0_b = vsetq_lane_s16(arr[counter+60 + 7], y0_b, 5);
         y0_b = vsetq_lane_s16(arr[counter+72 + 7], y0_b, 6);
         y0_b = vsetq_lane_s16(arr[counter+84 + 7], y0_b, 7);

         //y2_b = buffer[counter + 11];
         y0_b_temp = vsetq_lane_s16(arr[counter    + 11], y0_b_temp, 0);
         y0_b_temp = vsetq_lane_s16(arr[counter+12 + 11], y0_b_temp, 1);
         y0_b_temp = vsetq_lane_s16(arr[counter+24 + 11], y0_b_temp, 2);
         y0_b_temp = vsetq_lane_s16(arr[counter+36 + 11], y0_b_temp, 3);
         y0_b_temp = vsetq_lane_s16(arr[counter+48 + 11], y0_b_temp, 4);
         y0_b_temp = vsetq_lane_s16(arr[counter+60 + 11], y0_b_temp, 5);
         y0_b_temp = vsetq_lane_s16(arr[counter+72 + 11], y0_b_temp, 6);
         y0_b_temp = vsetq_lane_s16(arr[counter+84 + 11], y0_b_temp, 7);

         y0_b += y0_b_temp;

         //2 rows down
         //y0_b += buffer[next_counter + 7];
         //y2_b += buffer[next_counter + 11];
         y0_b_temp = vsetq_lane_s16(arr[next_counter    + 7], y0_b_temp, 0);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+12 + 7], y0_b_temp, 1);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+24 + 7], y0_b_temp, 2);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+36 + 7], y0_b_temp, 3);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+48 + 7], y0_b_temp, 4);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+60 + 7], y0_b_temp, 5);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+72 + 7], y0_b_temp, 6);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+84 + 7], y0_b_temp, 7);

         y0_b += y0_b_temp;

         y0_b_temp = vsetq_lane_s16(arr[next_counter    + 11], y0_b_temp, 0);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+12 + 11], y0_b_temp, 1);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+24 + 11], y0_b_temp, 2);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+36 + 11], y0_b_temp, 3);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+48 + 11], y0_b_temp, 4);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+60 + 11], y0_b_temp, 5);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+72 + 11], y0_b_temp, 6);
         y0_b_temp = vsetq_lane_s16(arr[next_counter+84 + 11], y0_b_temp, 7);

         y0_b += y0_b_temp;

         //divide each sum by 4
         u2 = vshrq_n_s16(u2,2);
         v2 = vshrq_n_s16(v2,2);
         y0_b = vshrq_n_s16(y0_b,2);

         //subtract scalars
         y0_b -= vdupq_n_s16(-16);
         u2 -= vdupq_n_s16(-128);
         v2 -= vdupq_n_s16(-128);

         ///////////////////////////////////////////////////////////////
         //Convert to BGR values
         //convert 4 of the BGR set at a time starting with the lower set of 4 
         //convert 4 low (4 low out of 8) Red values
         y0_temp_low = vcvtq_f32_s32(vmovl_s16(vget_low_s16(y0_b))); //convert lower 4 to 4 floats
         y0_temp_low *= vdupq_n_f32(1.164); //this variable will get reused in the computation

         r_low = y0_temp_low;
         r_low = vmlaq_f32(r_low, vcvtq_f32_s32(vmovl_s16(vget_low_s16(v2))), vdupq_n_f32(1.596));

         //convert 4 low (4 low out of 8) Green values
         g_low = y0_temp_low;
         g_low = vmlaq_f32(g_low, vcvtq_f32_s32(vmovl_s16(vget_low_s16(u2))), vdupq_n_f32(-0.392));
         g_low = vmlaq_f32(g_low, vcvtq_f32_s32(vmovl_s16(vget_low_s16(v2))), vdupq_n_f32(-0.813));
         y_const = g_low;

         //convert 4 low (4 low out of 8) Blue values
         b_low = y0_temp_low;
         b_low = vmlaq_f32(b_low, vcvtq_f32_s32(vmovl_s16(vget_low_s16(u2))), vdupq_n_f32(2.017));

         b_low_int = vcvtq_u32_f32(b_low); //convert from floats to ints, rounding toward zero
         g_low_int = vcvtq_u32_f32(g_low);
         r_low_int = vcvtq_u32_f32(r_low);

         for (int i=0; i<4; i++) {
            //store blue values
            float val = vgetq_lane_u32(b_low_int, i);

            if (val > 255)
               val = 255;
            else if (val < 0)
               val = 0;

            img2[temp_index] = (char) val;
            temp_index++;

            //store green values
            val = vgetq_lane_u32(g_low_int, i);

            if (val > 255)
               val = 255;
            else if (val < 0)
               val = 0;

            img2[temp_index] = (char) val;
            temp_index++;

            //store red values
            val = vgetq_lane_u32(r_low_int, i);

            if (val > 255)
               val = 255;
            else if (val < 0)
               val = 0;

            img2[temp_index] = (char) val;
            temp_index++;
         }

         //////////////convert high set of 4/////////////////////////////////////////
         //convert 4 high (4 high out of 8) Red values
         y0_temp_high = vcvtq_f32_s32(vmovl_s16(vget_high_s16(y0_b))); //convert to 4 floats
         y0_temp_high *= vdupq_n_f32(1.164); 

         r_high = y0_temp_high;
         r_high = vmlaq_f32(r_high, vcvtq_f32_s32(vmovl_s16(vget_high_s16(v2))), vdupq_n_f32(1.596));

         //convert 4 high (4 high out of 8) Green values
         g_high = y0_temp_high;
         g_high = vmlaq_f32(g_high, vcvtq_f32_s32(vmovl_s16(vget_high_s16(u2))), vdupq_n_f32(-0.392));
         g_high = vmlaq_f32(g_high, vcvtq_f32_s32(vmovl_s16(vget_high_s16(v2))), vdupq_n_f32(-0.813));
         y_const = g_high;

         //convert 4 high (4 high out of 8) Blue values
         b_high = y0_temp_high;
         b_high = vmlaq_f32(b_high, vcvtq_f32_s32(vmovl_s16(vget_high_s16(u2))), vdupq_n_f32(2.017));

         b_high_int = vcvtq_u32_f32(b_high); //convert from floats to ints, rounding toward zero
         g_high_int = vcvtq_u32_f32(g_high);
         r_high_int = vcvtq_u32_f32(r_high);

         for (int i=0; i<4; i++) {
            //store blue values
            float val = vgetq_lane_u32(b_high_int, i);

            if (val > 255)
               val = 255;
            else if (val < 0)
               val = 0;

            img2[temp_index] = (char) val;
            temp_index++;

            //store green values
            val = vgetq_lane_u32(g_high_int, i);

            if (val > 255)
               val = 255;
            else if (val < 0)
               val = 0;

            img2[temp_index] = (char) val;
            temp_index++;

            //store red values
            val = vgetq_lane_u32(r_high_int, i);

            if (val > 255)
               val = 255;
            else if (val < 0)
               val = 0;

            img2[temp_index] = (char) val;
            temp_index++;
         }
         temp_index = 0;
         // float r = 1.164 * avg_y                 + 1.596 * avg_v;
         // float g = 1.164 * avg_y - 0.392 * avg_u - 0.813 * avg_v;
         // float b = 1.164 * avg_y + 2.017 * avg_u;
         //


         // TODO: reorder the BGR values
         counter += 96;
      }
   }


   clock_t end = clock();
   double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
   printf ("time in ms:  %f\n", time_spent/1000.0);

   printf ("%d\n", vgetq_lane_s16 (u0, 0));
   printf ("%d\n", vgetq_lane_s16 (v0, 0));
   printf ("%d\n", vgetq_lane_s16 (y0_b, 0));
   printf ("%d\n", vgetq_lane_s16 (u2, 0));
   printf ("%f\n", vgetq_lane_f32 (y_const, 0));
   printf ("temp index: %d\n", temp_index);
   printf ("%c\n", img2[1]);

   return 0;
}
