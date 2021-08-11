.text
.globl convert_nhwc_asm
.type convert_nhwc_asm, @function

convert_nhwc_asm:
  //must preserve v8-v15 registers
  sub sp, sp, #128
  st1 { v8.2d,  v9.2d, v10.2d, v11.2d}, [sp], #64
  st1 {v12.2d, v13.2d, v14.2d, v15.2d}, [sp], #64

  mov x10,x0 //keep a copy of the starting address of the source array
  
  //x0 is the first argument
  //x1 is the second argument

  //00111011100000001000000010000000 = the constant (1.0/255.0)
  movz w5, 0b0011101110000000, lsl 16
  movk w5, 0b1000000010000000 //1/255
  dup v20.4s, w5 //copy the constant 4 times in the vector register

  //the constant 0
  movz w5, 0b0000000000000000, lsl 16
  movk w5, 0b0000000000000000 //0
  dup v21.4s, w5 //copy the constant 4 times in the vector register

  ld3 { v8.16b, v9.16b, v10.16b }, [x0], #48 //interleaved load of BGRBGR... to B->V8, G->V9, R->V10

  //copy lower 4 blue bytes to 4 fp vector register
  /* uaddw v15.8h, v15.8h, v8.8b */
  //copy next 4 blue bytes to 4 fp vector register
  //copy next, next 4 blue bytes to 4 fp vector register
  //copy next, next, next 4 blue bytes to 4 fp vector register

  fmul v17.4s, v17.4s, v20.4s //multiply 4 floats with (1/255)

end:
  //restore registers
  sub sp, sp, #128
  ld1 { v8.2d,  v9.2d, v10.2d, v11.2d}, [sp], #64
  ld1 {v12.2d, v13.2d, v14.2d, v15.2d}, [sp], #64
  ret

///* void CameraReader::nhwc_to_nchw(unsigned char* src, float* dest, int nn_height, int nn_width) { */
///*    //convert NHWC to NCHW with 3 channels */
///*    int num_pixels = nn_height * nn_width; */
///*    unsigned char* temp_src; */
///*    float div_255 = 1.00000 / 255.00000; */
//   
///*    for (int i = 0; i < 3; i++) */
///*    { */
///*       temp_src = src + i; */
//
///*       for (int h = 0; h < num_pixels; h++) */
///*       { */
///*          *dest = ((float)*temp_src) * div_255; //B */
//
///*          dest++; //move over 1 pixel */
///*          temp_src += 3; */
///*       } */
///*    } */
//
///*    return; */
///* } */
