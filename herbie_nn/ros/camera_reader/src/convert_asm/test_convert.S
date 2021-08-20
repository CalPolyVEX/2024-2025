.text
.globl convert_nhwc_asm
.type convert_nhwc_asm, @function

convert_nhwc_asm:
  //must preserve v8-v15 registers, if they are modified

  mov x10,x0 //keep a copy of the starting address of the source array
  
  //x0 is the first argument
  //x1 is the second argument
  mov x2, x1  //point to the dest Blue
  mov x3, x1  //point to the dest Green
  mov x4, x1  //point to the dest Red

  movz x5, 640
  movz x6, 360
  mul x5, x5, x6  //x5 is 640*360

  movz x6, 4
  mul x5, x5, x6  //each float is 4 bytes

  add x3, x3, x5  //point to the Green destination
  add x4, x4, x5
  add x4, x4, x5  //point to the Red destination

  //00111011100000001000000010000000 = the constant (1.0/255.0)
  movz w5, 0b0011101110000000, lsl 16
  movk w5, 0b1000000010000000 //1/255
  dup v20.4s, w5 //copy the constant 4 times in the vector register

  //the constant 0
  movz w5, 0b0000000000000000, lsl 16
  movk w5, 0b0000000000000000 //0
  dup v21.4s, w5 //copy the constant 4 times in the vector register

  movz x6, 14400
  movz x7, 0

loop:
  ld3 { v8.16b, v9.16b, v10.16b }, [x0], #48 //interleaved load of BGRBGR... to B->V8, G->V9, R->V10

  //Convert Blue
  ////////////////////////////////////////////////////////////////
  //convert the lower 8 bytes (out of the 16) of Blue to 16-bit shorts
  uaddw v22.8h, v21.8h, v8.8b   //adding 0 to each number is the same as a move

  uaddw v24.4s, v21.4s, v22.4h  //widen the lower 4 shorts to 32-bits each
  ucvtf v24.4s, v24.4s          //convert to floats
  fmul v24.4s, v20.4s, v24.4s   //divide each value by 255 (same as multiply by 1/255)
  st1 { v24.4s }, [x2], #16

  ////////////////////////////////////////////////////////////////
  //copy next 4 blue bytes to shorts
  uaddw2 v23.4s, v21.4s, v22.8h //adding 0 to each number is the same as a move
  ucvtf v23.4s, v23.4s          //convert to floats
  fmul v23.4s, v20.4s, v23.4s   //divide each value by 255 (same as multiply by 1/255)
  st1 { v23.4s }, [x2], #16

  ////////////////////////////////////////////////////////////////
  //convert the upper 8 bytes (out of the 16) of Blue to 16-bit shorts
  uaddw2 v25.8h, v21.8h, v8.16b //adding 0 to each number is the same as a move

  uaddw v24.4s, v21.4s, v25.4h  //widen the lower 4 shorts to 32-bits each
  ucvtf v24.4s, v24.4s          //convert to floats
  fmul v24.4s, v20.4s, v24.4s   //divide each value by 255 (same as multiply by 1/255)
  st1 { v24.4s }, [x2], #16

  ////////////////////////////////////////////////////////////////
  //copy next 4 blue bytes to shorts
  uaddw2 v23.4s, v21.4s, v25.8h //adding 0 to each number is the same as a move
  ucvtf v23.4s, v23.4s          //convert to floats
  fmul v23.4s, v20.4s, v23.4s   //divide each value by 255 (same as multiply by 1/255)
  st1 { v23.4s }, [x2], #16

  //Convert Green
  ////////////////////////////////////////////////////////////////
  //convert the lower 8 bytes (out of the 16) of Blue to 16-bit shorts
  uaddw v22.8h, v21.8h, v9.8b   //adding 0 to each number is the same as a move

  uaddw v24.4s, v21.4s, v22.4h  //widen the lower 4 shorts to 32-bits each
  ucvtf v24.4s, v24.4s          //convert to floats
  fmul v24.4s, v20.4s, v24.4s   //divide each value by 255 (same as multiply by 1/255)
  st1 { v24.4s }, [x3], #16

  ////////////////////////////////////////////////////////////////
  //copy next 4 blue bytes to shorts
  uaddw2 v23.4s, v21.4s, v22.8h //adding 0 to each number is the same as a move
  ucvtf v23.4s, v23.4s          //convert to floats
  fmul v23.4s, v20.4s, v23.4s   //divide each value by 255 (same as multiply by 1/255)
  st1 { v23.4s }, [x3], #16

  ////////////////////////////////////////////////////////////////
  //convert the upper 8 bytes (out of the 16) of Blue to 16-bit shorts
  uaddw2 v25.8h, v21.8h, v9.16b //adding 0 to each number is the same as a move

  uaddw v24.4s, v21.4s, v25.4h  //widen the lower 4 shorts to 32-bits each
  ucvtf v24.4s, v24.4s          //convert to floats
  fmul v24.4s, v20.4s, v24.4s   //divide each value by 255 (same as multiply by 1/255)
  st1 { v24.4s }, [x3], #16

  ////////////////////////////////////////////////////////////////
  //copy next 4 blue bytes to shorts
  uaddw2 v23.4s, v21.4s, v25.8h //adding 0 to each number is the same as a move
  ucvtf v23.4s, v23.4s          //convert to floats
  fmul v23.4s, v20.4s, v23.4s   //divide each value by 255 (same as multiply by 1/255)
  st1 { v23.4s }, [x3], #16

  //Convert Red
  ////////////////////////////////////////////////////////////////
  //convert the lower 8 bytes (out of the 16) of Red to 16-bit shorts
  uaddw v22.8h, v21.8h, v10.8b   //adding 0 to each number is the same as a move

  uaddw v24.4s, v21.4s, v22.4h  //widen the lower 4 shorts to 32-bits each
  ucvtf v24.4s, v24.4s          //convert to floats
  fmul v24.4s, v20.4s, v24.4s   //divide each value by 255 (same as multiply by 1/255)
  st1 { v24.4s }, [x4], #16

  ////////////////////////////////////////////////////////////////
  //copy next 4 red bytes to shorts
  uaddw2 v23.4s, v21.4s, v22.8h //adding 0 to each number is the same as a move
  ucvtf v23.4s, v23.4s          //convert to floats
  fmul v23.4s, v20.4s, v23.4s   //divide each value by 255 (same as multiply by 1/255)
  st1 { v23.4s }, [x4], #16

  ////////////////////////////////////////////////////////////////
  //convert the upper 8 bytes (out of the 16) of Red to 16-bit shorts
  uaddw2 v25.8h, v21.8h, v10.16b //adding 0 to each number is the same as a move

  uaddw v24.4s, v21.4s, v25.4h  //widen the lower 4 shorts to 32-bits each
  ucvtf v24.4s, v24.4s          //convert to floats
  fmul v24.4s, v20.4s, v24.4s   //divide each value by 255 (same as multiply by 1/255)
  st1 { v24.4s }, [x4], #16

  ////////////////////////////////////////////////////////////////
  //copy next 4 red bytes to shorts
  uaddw2 v23.4s, v21.4s, v25.8h //adding 0 to each number is the same as a move
  ucvtf v23.4s, v23.4s          //convert to floats
  fmul v23.4s, v20.4s, v23.4s   //divide each value by 255 (same as multiply by 1/255)
  st1 { v23.4s }, [x4], #16

  add x7, x7, 1
  cmp x6, x7
  bne loop      //go back to loop if not complete

end:
  //restore registers, if v8-v15 are modified
  ret
