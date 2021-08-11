#include <stdio.h>
 
void convert_nhwc_asm(unsigned char* in, float* out);
void convert_nhwc_asm_standard(unsigned char* in, float* out);

int main() {
   unsigned char a[100];
   float b[100];
   
   for (int i=0; i<100; i++) {
      a[i] = i;
   }

   convert_nhwc_asm_standard(a, b);
   for (int i=0; i<10; i++) {
      printf("%f ", b[i]);
   }
   printf("\n");

   convert_nhwc_asm(a, b);
   for (int i=0; i<10; i++) {
      printf("%f ", b[i]);
   }
   printf("\n");
   
   return 0;
}


void convert_nhwc_asm_standard(unsigned char* src, float* dest) {
   //convert NHWC to NCHW with 3 channels
   int num_pixels = 360 * 640;
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
