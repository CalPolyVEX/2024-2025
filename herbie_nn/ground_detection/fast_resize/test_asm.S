.section .text
.globl asm_foo
.type asm_foo, @function
asm_foo:
  //load the first 2 UYVY
  ld1 {v0.2s}, [x0] //load 8-bytes
  ext v1.8b, v0.8b, v0.8b, #4 //swap 4 bytes

  //load the third UYVY
  add x3, x0, 8
  ld1 {v2.2s}, [x3] //load 8-bytes

  //skip down 2 rows (1920*2 * 2 bytes) = 7680
  add x4, x0, 3840
  add x4, x4, 3840
  add x5, x4, 8

  //load the first 2 UYVY
  ld1 {v3.2s}, [x4] //load 8-bytes
  ext v4.8b, v3.8b, v3.8b, #4 //swap 4 bytes

  //load the third UYVY
  ld1 {v5.2s}, [x5] //load 8-bytes

  //add the words for the first pixel
  eor v10.16b, v10.16b, v10.16b //zero out v10
  mov v11.16b, v10.16b          //zero out v11
  uaddw v10.8h, v10.8h, v0.8b
  uaddw v10.8h, v10.8h, v1.8b //add the first and second words of current row

  uaddw v10.8h, v10.8h, v3.8b //add the first word of 2 rows below
  uaddw v10.8h, v10.8h, v4.8b //add the second word of 2 rows below

  //add the words for the second pixel
  uaddw v11.8h, v11.8h, v1.8b
  uaddw v11.8h, v11.8h, v2.8b //add the first and second words of current row

  uaddw v11.8h, v11.8h, v4.8b //add the first word of 2 rows below
  uaddw v11.8h, v11.8h, v5.8b //add the second word of 2 rows below

  //v10 and v11 have the YUV sum, divide by 4 to average
  ushr v10.8h, v10.8h, 2
  ushr v11.8h, v11.8h, 2

  //subtract 16, 128, and 128
  mov x3, 128 //128 to subtract from V
  mov x3, x3, lsl 32 //shift left 32
  mov x4, 16 //16 to subtract from Y
  mov x4, x4, lsl 16 //shift left 16
  orr x3, x3, x4
  mov x4, 128 //128 to subtract from U
  orr x3, x3, x4

  ins v12.d[0], x3

  ssubl v10.4s,v10.4h,v12.4h
  ssubl v11.4s,v11.4h,v12.4h

  //do FP computation to get BGR
  /* eor v13.8b, v13.8b, v13.8b //zero out v13 */
  /* saddw v14.4s, v13.4s, v10.4h //convert YUV values in 4x32 int */
  /* saddw v15.4s, v13.4s, v11.4h */
  
  scvtf v14.4s, v10.4s //convert to floats, registers in format Y1-V0-Y0-U0
  scvtf v15.4s, v11.4s

  //00111111100101001111110111110100 //1.164
  movz w5, 0b0011111110010100, lsl 16
  movk w5, 0b1111110111110100 //1.164
  dup v16.4s, w5 //copy to all elements
  //mov w4, v16.s[1] //debugging print 

  mov v17.s[1], v15.s[1]  //move the Y into place
  mov v17.s[0], v14.s[1] 
  //mov w4, v17.s[0] //debugging print 
  fmul v17.4s, v17.4s, v16.4s //v17 contains 1.164*Y
  mov v20.16b, v17.16b  //where to hold the R values
  mov v21.16b, v17.16b  //where to hold the G values
  mov v22.16b, v17.16b  //where to hold the B values

  //00111111110011000100100110111010 //1.596
  movz w5, 0b0011111111001100, lsl 16
  movk w5, 0b0100100110111010 //1.596
  dup v16.4s, w5 //copy to all elements
  //mov w4, v16.s[0] //debugging print 

  mov v18.s[1], v15.s[2]  //move the V into place
  mov v18.s[0], v14.s[2] 
  //mov w4, v18.s[0] //debugging print 
  fmla v20.4s, v18.4s, v16.4s //R values computed
  
  //00111111010100000010000011000101  //.813
  movz w5, 0b0011111101010000, lsl 16
  movk w5, 0b0010000011000101 //.813
  dup v16.4s, w5 //copy to all elements
  //mov w4, v16.s[0] //debugging print 
  fmls v21.4s, v18.4s, v16.4s

  //00111110110010001011010000111001 //.392
  movz w5, 0b0011111011001000, lsl 16
  movk w5, 0b1011010000111001 //.392
  dup v16.4s, w5 //copy to all elements
  //mov w4, v16.s[0] //debugging print 

  mov v23.s[1], v15.s[0]  //move the U into place
  mov v23.s[0], v14.s[0] 
  fmls v21.4s, v23.4s, v16.4s //G values computed

  //01000000000001101101100100010111 //2.017
  movz w5, 0b0100000000000110, lsl 16
  movk w5, 0b1101100100010111 //2.107
  dup v16.4s, w5 //copy to all elements
  //mov w4, v16.s[0] //debugging print 
  fmla v22.4s, v23.4s, v16.4s //B values computed

  //replace all values above 255
  movz w5, 0b0100001101111111, lsl 16
  movk w5, 0b0000000000000000 //255.0
  dup v16.4s, w5 //copy to all elements
  //mov w4, v16.s[0] //debugging print 
  fcmgt v17.4s, v22.4s, v16.4s //set max B values
  bit v22.16b, v16.16b, v17.16b

  fcmgt v17.4s, v21.4s, v16.4s //set max G values
  bit v21.16b, v16.16b, v17.16b

  fcmgt v17.4s, v20.4s, v16.4s //set max R values
  bit v20.16b, v16.16b, v17.16b

  //convert the fp to int (rounding toward zero)
  /* fcvtzu v22.4s, v22.4s //convert B to int */
  /* fcvtzu v21.4s, v21.4s //convert G to int */
  /* fcvtzu v20.4s, v20.4s //convert R to int */


  //get lane
  //mov x3, v0.d[0]
  mov w3, v21.s[0]
  //mov w3,w3,lsr 16
  
  //mov x3, -2 //move constant -2 into x3
  //ldr x3,[x0] //load into a register
  str w3,[x1]
  add x1, x1, 4
  str w4,[x1]
  ret
