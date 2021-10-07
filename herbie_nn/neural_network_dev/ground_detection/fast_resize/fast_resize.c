#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void process_uyvy(unsigned char* buffer) {
    //convert 1920*1080 UYVY to 360*640*3 BGR

    int counter = 0;
    int next_counter = 0;
    unsigned char ba_new[360*640*3];
    int row_size = 3840; //number of bytes per row
    int index=0;
    int u0,v0,y0,y2;

    double total_time = 0;
    double v_table[256], v_table2[256];

    for(int z=0;z<255;z++) {
        v_table[z] = 1.596 * (z-128);
    }

    for(int z=0;z<255;z++) {
        v_table2[z] = 0.813 * (z-128);
    }

    for (int t = 0; t < 1000; t++)
    {
        counter = 0;
        next_counter = 0;
        index = 0;

        clock_t begin = clock();

        for (int i = 0; i < 360; i++)
        {
            for (int j = 0; j < 320; j++) //640/2 = process pixels in sets of 6
            {                 
                for (int k = 0; k < 2; k++) //alternating pixels within macropixel sets of 3
                {
                    u0 = buffer[counter] + buffer[counter + 4];     //add the U values (u0 and u2)
                    v0 = buffer[counter + 2] + buffer[counter + 6]; //add the V values (v0 and v2)

                    //2 rows down
                    next_counter = counter + row_size * 2;
                    u0 += buffer[next_counter] + buffer[next_counter + 4];     //add the U values
                    v0 += buffer[next_counter + 2] + buffer[next_counter + 6]; //add the V values

                    if (k == 0)
                    {
                        y0 = buffer[counter + 1];
                        y2 = buffer[counter + 5];

                        //2 rows down
                        y0 += buffer[next_counter + 1];
                        y2 += buffer[next_counter + 5];

                        //goto the next macropixel
                        counter += 4;
                    }
                    else
                    {
                        y0 = buffer[counter + 3];
                        y2 = buffer[counter + 7];

                        //2 rows down
                        y0 += buffer[next_counter + 3];
                        y2 += buffer[next_counter + 7];

                        //skip 8 bytes to get to the next set of 3 macropixels
                        counter += 8;
                    }

                    //compute totals and average
                    u0 = u0 >> 2; //divide by 4 to get average
                    v0 = v0 >> 2; //divide by 4 to get average
                    y0 = (y0 + y2) >> 2; //divide by 4 to get average

                    // int avg_u = (u0);
                    // int avg_v = (v0);
                    // int avg_y = (y0);

                    // avg_y -= 16;
                    // avg_u -= 128;
                    // avg_v -= 128;
                    // printf ("v: %d\n", v0);
                    y0 -= 16;
                    u0 -= 128;
                    // v0 -= 128;

                    // float r = 1.164 * avg_y + 1.596 * avg_v;
                    // float g = 1.164 * avg_y - 0.392 * avg_u - 0.813 * avg_v;
                    // float b = 1.164 * avg_y + 2.017 * avg_u;


                    //double r = 1.164 * y0              + 1.596 * v0;
                    double r = 1.164 * y0              + v_table[v0];
                    // double g = 1.164 * y0 - 0.392 * u0 - 0.813 * v0;
                    double g = 1.164 * y0 - 0.392 * u0 + v_table2[v0];
                    double b = 1.164 * y0 + 2.017 * u0;

                    if (b > 255)
                        b = 255;
                    else if (b < 0)
                        b = 0;

                    if (g > 255)
                        g = 255;
                    else if (g < 0)
                        g = 0;

                    if (r > 255)
                        r = 255;
                    else if (r < 0)
                        r = 0;

                    ba_new[index] = (unsigned char)b;
                    index++;
                    ba_new[index] = (unsigned char)g;
                    index++;
                    ba_new[index] = (unsigned char)r;
                    index++;
                }

                __builtin_prefetch (&(buffer[counter + 256]));
                __builtin_prefetch (&(buffer[counter + 256 + row_size*2]));
            }
            counter += row_size * 2;
        }

        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        total_time += time_spent;
    }
    printf ("time in ms:  %f\n", total_time);

    FILE *p = fopen("beach360.uyvy","wb");
    fwrite(ba_new,1,sizeof(ba_new),p);
    fclose(p);
}
/*
    counter = 0
    next_counter = 0
    ba_new=bytearray()

    row_size = 3840 #number of bytes per row
    start_ms = time.time()*1000.0

    for i in range(360):
        for j in range(320): #640/2 = process pixels in sets of 6
            for k in range(2): #alternating pixels within macropixel sets of 3
                u0 = ba[counter] + ba[counter+4] #add the U values (u0 and u2)
                v0 = ba[counter+2] + ba[counter+6] #add the V values (v0 and v2)

                #2 rows down
                next_counter = counter + row_size*2
                u0 += ba[next_counter] + ba[next_counter+4] #add the U values
                v0 += ba[next_counter+2] + ba[next_counter+6] #add the V values

                if k == 0:
                    y0 = ba[counter+1]
                    y2 = ba[counter+5]

                    #2 rows down
                    y0 += ba[next_counter+1]
                    y2 += ba[next_counter+5]

                    #goto the next macropixel
                    counter += 4
                else:
                    y0 = ba[counter+3]
                    y2 = ba[counter+7]

                    #2 rows down
                    y0 += ba[next_counter+3]
                    y2 += ba[next_counter+7]

                    #skip 8 bytes to get to the next set of 3 macropixels
                    counter += 8

                #compute totals and average
                avg_u = (u0) / 4.0
                avg_v = (v0) / 4.0
                avg_y = (y0 + y2) / 4.0

                avg_y -= 16;
                avg_u -= 128;
                avg_v -= 128;

                r = 1.164 * avg_y                 + 1.596 * avg_v;
                g = 1.164 * avg_y - 0.392 * avg_u - 0.813 * avg_v;
                b = 1.164 * avg_y + 2.017 * avg_u;

                if b > 255:
                    b = 255
                if g > 255:
                    g = 255
                if r > 255:
                    r = 255

                if b < 0:
                    b = 0
                if g < 0:
                    g = 0
                if r < 0:
                    r = 0

                # print (r1,g1,b1)

                ba_new.append(int(b))
                ba_new.append(int(g))
                ba_new.append(int(r))

        counter += row_size*2
        */

int main(){
    FILE* ptr;
    unsigned char buffer[1920*1080*2];

    ptr = fopen("../beach.uyvy","rb");  // r for read, b for binary

    if (!ptr) {
        printf ("Unable to open file.\n");
    }

    // read bytes to the buffer 

    int x = fread(buffer,1,sizeof(buffer),ptr);

    if (x != sizeof(buffer)) {
        printf ("read error %d\n", x);
    }

    fclose(ptr);

    process_uyvy(buffer);

    return 0;
}