#include <stdio.h>
#include <stdlib.h>

int main(){
    unsigned char buffer[1920*1080*2];
    FILE* ptr;

    ptr = fopen("../beach.uyvy","rb");  // r for read, b for binary

    if (!ptr) {
        printf ("Unable to open file.\n");
    }

    // read 10 bytes to our buffer 
    int r = fread(buffer,100,1,ptr);
    if (r != 100) {
        printf ("read error %d\n", r);
    }

    fclose(ptr);

    return 0;
}