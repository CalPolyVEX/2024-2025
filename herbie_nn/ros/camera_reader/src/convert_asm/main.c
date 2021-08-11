#include <stdio.h>
 
void convert_nhwc_asm(unsigned char* in, float* out);

int main() {
   unsigned char a[100];
   float b;
   
   for (int i=0; i<100; i++) {
      a[i] = i;
   }

   convert_nhwc_asm(a, &b);
   
   return 0;
}
