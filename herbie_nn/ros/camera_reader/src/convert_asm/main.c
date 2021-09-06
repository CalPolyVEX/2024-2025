#include <stdio.h>
 
void convert_nhwc_asm(unsigned char* in, float* out);
void convert_nhwc_asm_standard(unsigned char* in, float* out);

int main() {
   unsigned char a[640*360*3];
   float b[640*360*3];
   float c[640*360*3];
   
   for (int i=0; i<640*360*3; i++) {
      a[i] = i+1;
      b[i] = 5;
      c[i] = 10;
   }

   convert_nhwc_asm_standard(a, b);
   for (int i=0; i<20; i++) {
      printf("%f ", b[i]);
   }
   printf("\n");

   convert_nhwc_asm(a, c);
   for (int i=0; i<20; i++) {
      printf("%f ", c[i]);
   }
   printf("\n");

   for (int i=0; i<(640*360*3); i++) {
      if ((c[i]-b[i]) > .000001) {
         printf("error: %d\n", i);
         break;
      }
   }
   
   return 0;
}

void convert_nhwc_asm_standard(unsigned char* src, float* dest) {
   //convert NHWC to NCHW with 3 channels
   int num_pixels = 640 * 360;
   unsigned char* temp_src;
   float div_255 = 1.00000 / 255.00000;
   
   for (int i = 0; i < 3; i++)
   {
      temp_src = src + i;

      for (int h = 0; h < num_pixels; h++)
      {
         *dest = ((float)*temp_src) * div_255; //B

         dest++; //move over 1 pixel
         temp_src += 3;
      }
   }

   return;
}
