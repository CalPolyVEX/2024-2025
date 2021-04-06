#include<stdio.h>
#include<arm_neon.h>
#include<stdlib.h>
#include<time.h>

int main() {
   FILE* ptr;
   unsigned char arr[1920 * 1080 * 2];
   unsigned char new_img[640*360*3];
   int16x8_t u0, u2;
   int16x8_t u0_temp, u2_next;
   int16x8_t v0, v2;
   int16x8_t v0_temp, v2_next;
   int16x8_t y0_a, y0_b;
   int16x8_t y0_a_temp; 
   unsigned int *test_ptr = (unsigned int*) arr;
   unsigned char val;
   
   ptr = fopen("./beach.uyvy","rb");  // r for read, b for binary

   if (!ptr) {
      printf ("Unable to open file.\n");
   }

   // read bytes to the buffer 
   int fp = fread(arr,1,sizeof(arr),ptr);

   if (fp != sizeof(arr)) {
      printf ("read error %d\n", fp);
   }

   fclose(ptr);

   int row_size = 3840; 
   int pixel_index = 0;
   int counter = 0;
   /* unsigned int x; */

   clock_t begin = clock();

   for (int j=0;j<1000;j++) {
      /* usleep(50000); */

      counter = 0;
      pixel_index = 0;

      for(int row=0; row<360; row++) 
      {
         //printf ("starting pixel index row: %d\n", pixel_index);
         for(int column=0; column<80; column++) //40 iterations per row (1920*2 / 96)
         {
            //u0 = buffer[counter] + buffer[counter + 4];     //add the U values (u0 and u2)
            u0 = vsetq_lane_s16(arr[counter   ], u0, 0);
            u0 = vsetq_lane_s16(arr[counter+12], u0, 1);
            u0 = vsetq_lane_s16(arr[counter+24], u0, 2);
            u0 = vsetq_lane_s16(arr[counter+36], u0, 3);

            u2 = vsetq_lane_s16(arr[counter    + 4], u2, 0);
            u2 = vsetq_lane_s16(arr[counter+12 + 4], u2, 1);
            u2 = vsetq_lane_s16(arr[counter+24 + 4], u2, 2);
            u2 = vsetq_lane_s16(arr[counter+36 + 4], u2, 3);

            u0 += u2;

            //v0 = buffer[counter + 2] + buffer[counter + 6]; //add the V values (v0 and v2)
            v0 = vsetq_lane_s16(arr[counter    + 2], v0, 0);
            v0 = vsetq_lane_s16(arr[counter+12 + 2], v0, 1);
            v0 = vsetq_lane_s16(arr[counter+24 + 2], v0, 2);
            v0 = vsetq_lane_s16(arr[counter+36 + 2], v0, 3);

            v2 = vsetq_lane_s16(arr[counter    + 6], v2, 0);
            v2 = vsetq_lane_s16(arr[counter+12 + 6], v2, 1);
            v2 = vsetq_lane_s16(arr[counter+24 + 6], v2, 2);
            v2 = vsetq_lane_s16(arr[counter+36 + 6], v2, 3);

            v0 += v2;

            /* next_counter = counter + row_size * 2; */
            int next_counter = counter + row_size * 2; 

            /* u0 += buffer[next_counter] + buffer[next_counter + 4];     //add the U values */
            u0_temp = vsetq_lane_s16(arr[next_counter   ], u0_temp, 0);
            u0_temp = vsetq_lane_s16(arr[next_counter+12], u0_temp, 1);
            u0_temp = vsetq_lane_s16(arr[next_counter+24], u0_temp, 2);
            u0_temp = vsetq_lane_s16(arr[next_counter+36], u0_temp, 3);

            u0 = u0 + u0_temp;

            u2_next = vsetq_lane_s16(arr[next_counter    + 4], u2_next, 0);
            u2_next = vsetq_lane_s16(arr[next_counter+12 + 4], u2_next, 1);
            u2_next = vsetq_lane_s16(arr[next_counter+24 + 4], u2_next, 2);
            u2_next = vsetq_lane_s16(arr[next_counter+36 + 4], u2_next, 3);

            u0 = u0 + u2_next;

            /* v0 += buffer[next_counter + 2] + buffer[next_counter + 6]; //add the V values */
            v0_temp = vsetq_lane_s16(arr[next_counter    + 2], v0_temp, 0);
            v0_temp = vsetq_lane_s16(arr[next_counter+12 + 2], v0_temp, 1);
            v0_temp = vsetq_lane_s16(arr[next_counter+24 + 2], v0_temp, 2);
            v0_temp = vsetq_lane_s16(arr[next_counter+36 + 2], v0_temp, 3);

            v0 = v0 + v0_temp;

            v2_next = vsetq_lane_s16(arr[next_counter    + 6], v2_next, 0);
            v2_next = vsetq_lane_s16(arr[next_counter+12 + 6], v2_next, 1);
            v2_next = vsetq_lane_s16(arr[next_counter+24 + 6], v2_next, 2);
            v2_next = vsetq_lane_s16(arr[next_counter+36 + 6], v2_next, 3);

            v0 = v0 + v2_next;

            //y0 = buffer[counter + 1];
            y0_a = vsetq_lane_s16(arr[counter    + 1], y0_a, 0);
            y0_a = vsetq_lane_s16(arr[counter+12 + 1], y0_a, 1);
            y0_a = vsetq_lane_s16(arr[counter+24 + 1], y0_a, 2);
            y0_a = vsetq_lane_s16(arr[counter+36 + 1], y0_a, 3);

            //y2 = buffer[counter + 5];
            y0_a_temp = vsetq_lane_s16(arr[counter    + 5], y0_a_temp, 0);
            y0_a_temp = vsetq_lane_s16(arr[counter+12 + 5], y0_a_temp, 1);
            y0_a_temp = vsetq_lane_s16(arr[counter+24 + 5], y0_a_temp, 2);
            y0_a_temp = vsetq_lane_s16(arr[counter+36 + 5], y0_a_temp, 3);

            y0_a += y0_a_temp;

            //2 rows down
            //y0 += buffer[next_counter + 1];
            //y2 += buffer[next_counter + 5];
            y0_a_temp = vsetq_lane_s16(arr[next_counter    + 1], y0_a_temp, 0);
            y0_a_temp = vsetq_lane_s16(arr[next_counter+12 + 1], y0_a_temp, 1);
            y0_a_temp = vsetq_lane_s16(arr[next_counter+24 + 1], y0_a_temp, 2);
            y0_a_temp = vsetq_lane_s16(arr[next_counter+36 + 1], y0_a_temp, 3);

            y0_a += y0_a_temp;

            y0_a_temp = vsetq_lane_s16(arr[next_counter    + 5], y0_a_temp, 0);
            y0_a_temp = vsetq_lane_s16(arr[next_counter+12 + 5], y0_a_temp, 1);
            y0_a_temp = vsetq_lane_s16(arr[next_counter+24 + 5], y0_a_temp, 2);
            y0_a_temp = vsetq_lane_s16(arr[next_counter+36 + 5], y0_a_temp, 3);

            y0_a += y0_a_temp;

            //divide each sum by 4
            u0 = vshrq_n_s16(u0,2);
            v0 = vshrq_n_s16(v0,2);
            y0_a = vshrq_n_s16(y0_a,2);

            //subtract scalars
            y0_a -= vdupq_n_s16(16);
            u0 -= vdupq_n_s16(128);
            v0 -= vdupq_n_s16(128);

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

            //convert 4 low (4 low out of 8) Blue values
            float32x4_t b_low = y0_temp_low;
            b_low = vmlaq_f32(b_low, vcvtq_f32_s32(vmovl_s16(vget_low_s16(u0))), vdupq_n_f32(2.017));

            uint32x4_t b_low_int = vcvtq_u32_f32(b_low); //convert from floats to ints, rounding toward zero
            uint32x4_t g_low_int = vcvtq_u32_f32(g_low);
            uint32x4_t r_low_int = vcvtq_u32_f32(r_low);

            /* printf ("starting: %d\n", pixel_index); */

            // float r = 1.164 * avg_y                 + 1.596 * avg_v;
            // float g = 1.164 * avg_y - 0.392 * avg_u - 0.813 * avg_v;
            // float b = 1.164 * avg_y + 2.017 * avg_u;

            //////////////////////////////////////////////////////////////////
            //Computing the second set of UYVY conversions
            //////////////////////////////////////////////////////////////////

            //second set of U in 'counter' row
            u0_temp = vsetq_lane_s16(arr[counter    + 8], u0_temp, 0);
            u0_temp = vsetq_lane_s16(arr[counter+12 + 8], u0_temp, 1);
            u0_temp = vsetq_lane_s16(arr[counter+24 + 8], u0_temp, 2);
            u0_temp = vsetq_lane_s16(arr[counter+36 + 8], u0_temp, 3);

            u2 += u0_temp;

            //second set of V in 'counter' row
            v0_temp = vsetq_lane_s16(arr[counter    + 10], v0_temp, 0);
            v0_temp = vsetq_lane_s16(arr[counter+12 + 10], v0_temp, 1);
            v0_temp = vsetq_lane_s16(arr[counter+24 + 10], v0_temp, 2);
            v0_temp = vsetq_lane_s16(arr[counter+36 + 10], v0_temp, 3);

            v2 += v0_temp;

            //second set of U in 'nextcounter' row
            u0_temp = vsetq_lane_s16(arr[next_counter    + 8], u0_temp, 0);
            u0_temp = vsetq_lane_s16(arr[next_counter+12 + 8], u0_temp, 1);
            u0_temp = vsetq_lane_s16(arr[next_counter+24 + 8], u0_temp, 2);
            u0_temp = vsetq_lane_s16(arr[next_counter+36 + 8], u0_temp, 3);

            u2 += u2_next + u0_temp;

            //second set of V in 'nextcounter' row
            v0_temp = vsetq_lane_s16(arr[next_counter    + 10], v0_temp, 0);
            v0_temp = vsetq_lane_s16(arr[next_counter+12 + 10], v0_temp, 1);
            v0_temp = vsetq_lane_s16(arr[next_counter+24 + 10], v0_temp, 2);
            v0_temp = vsetq_lane_s16(arr[next_counter+36 + 10], v0_temp, 3);

            v2 += v2_next + v0_temp;

            /////////////////////////////////////////////////////////
            //Second set of y values
            //y0_b = buffer[counter + 7];
            y0_b = vsetq_lane_s16(arr[counter    + 7], y0_b, 0);
            y0_b = vsetq_lane_s16(arr[counter+12 + 7], y0_b, 1);
            y0_b = vsetq_lane_s16(arr[counter+24 + 7], y0_b, 2);
            y0_b = vsetq_lane_s16(arr[counter+36 + 7], y0_b, 3);

            //y2_b = buffer[counter + 11];
            y0_a_temp = vsetq_lane_s16(arr[counter    + 11], y0_a_temp, 0);
            y0_a_temp = vsetq_lane_s16(arr[counter+12 + 11], y0_a_temp, 1);
            y0_a_temp = vsetq_lane_s16(arr[counter+24 + 11], y0_a_temp, 2);
            y0_a_temp = vsetq_lane_s16(arr[counter+36 + 11], y0_a_temp, 3);

            y0_b += y0_a_temp;

            //2 rows down
            //y0_b += buffer[next_counter + 7];
            //y2_b += buffer[next_counter + 11];
            y0_a_temp = vsetq_lane_s16(arr[next_counter    + 7], y0_a_temp, 0);
            y0_a_temp = vsetq_lane_s16(arr[next_counter+12 + 7], y0_a_temp, 1);
            y0_a_temp = vsetq_lane_s16(arr[next_counter+24 + 7], y0_a_temp, 2);
            y0_a_temp = vsetq_lane_s16(arr[next_counter+36 + 7], y0_a_temp, 3);

            y0_b += y0_a_temp;

            y0_a_temp = vsetq_lane_s16(arr[next_counter    + 11], y0_a_temp, 0);
            y0_a_temp = vsetq_lane_s16(arr[next_counter+12 + 11], y0_a_temp, 1);
            y0_a_temp = vsetq_lane_s16(arr[next_counter+24 + 11], y0_a_temp, 2);
            y0_a_temp = vsetq_lane_s16(arr[next_counter+36 + 11], y0_a_temp, 3);

            y0_b += y0_a_temp;

            //divide each sum by 4
            u2 = vshrq_n_s16(u2,2);
            v2 = vshrq_n_s16(v2,2);
            y0_b = vshrq_n_s16(y0_b,2);

            //subtract scalars
            y0_b -= vdupq_n_s16(16);
            u2 -= vdupq_n_s16(128);
            v2 -= vdupq_n_s16(128);

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

            //convert 4 low (4 low out of 8) Blue values
            b_low = y0_temp_low;
            b_low = vmlaq_f32(b_low, vcvtq_f32_s32(vmovl_s16(vget_low_s16(u2))), vdupq_n_f32(2.017));

            uint32x4_t b_second_int = vcvtq_u32_f32(b_low); //convert from floats to ints, rounding toward zero
            uint32x4_t g_second_int = vcvtq_u32_f32(g_low);
            uint32x4_t r_second_int = vcvtq_u32_f32(r_low);

            // float r = 1.164 * avg_y                 + 1.596 * avg_v;
            // float g = 1.164 * avg_y - 0.392 * avg_u - 0.813 * avg_v;
            // float b = 1.164 * avg_y + 2.017 * avg_u;
            //

            //copy the BGR values into the final image
            for (int i=0; i<4; i++) {
               val = (unsigned char) vgetq_lane_u32(b_low_int, i);
               if (val > 255)
                  val = 255;
               new_img[pixel_index] = val;

               val = (unsigned char) vgetq_lane_u32(g_low_int, i);
               if (val > 255)
                  val = 255;
               new_img[pixel_index+1] = val;

               val = (unsigned char) vgetq_lane_u32(r_low_int, i);
               if (val > 255)
                  val = 255;
               new_img[pixel_index+2] = val;

               val = (unsigned char) vgetq_lane_u32(r_second_int, i);
               if (val > 255)
                  val = 255;
               new_img[pixel_index+3] = val;

               val = (unsigned char) vgetq_lane_u32(g_second_int, i);
               if (val > 255)
                  val = 255;
               new_img[pixel_index+4] = val;

               val = (unsigned char) vgetq_lane_u32(r_second_int, i);
               if (val > 255)
                  val = 255;
               new_img[pixel_index+5] = val;
               pixel_index+=6;
            }

            counter += 48;
         }

         //at the end of each row, skip forward 2 rows
         counter += 1920*2*2;

         /* __builtin_prefetch (&(arr[counter + 1920*2*2*2])); */ 
      }
   }


   clock_t end = clock();
   double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
   printf ("time in ms:  %f\n", time_spent/1000.0);

   printf ("%d\n", vgetq_lane_s16 (u0, 0));
   printf ("%d\n", vgetq_lane_s16 (v0, 0));
   printf ("%d\n", vgetq_lane_s16 (y0_b, 0));
   printf ("%d\n", vgetq_lane_s16 (u2, 0));
   printf ("counter: %d\n", counter);
   printf ("pixel index: %d\n", pixel_index);
   printf ("%d\n", new_img[100]);

   return 0;
}
