.section .text
.globl asm_foo
.type asm_foo, @function

asm_foo:
  //must preserve v8-v15 registers
  sub sp, sp, #128
  st1 { v8.2d,  v9.2d, v10.2d, v11.2d}, [sp], #64
  st1 {v12.2d, v13.2d, v14.2d, v15.2d}, [sp], #64

  mov w9, 0 //w9 = counter
  movz w12, 360 //360 columns
  mov w13, 0 //w13 = counter
  movz w14, 160 //640/4 per row

  mov x10,x1 //keep a copy of the starting address of the incoming array

  //compute the constant (16,128,16,128) and put into v12
  //subtract 16, 128, and 128
  mov x3, 16 //16 to subtract from Y
  mov x3, x3, lsl 48 //shift left 16
  mov x4, 128 //128 to subtract from V
  mov x4, x4, lsl 32 //shift left 32
  orr x3, x3, x4
  mov x4, 16 //16 to subtract from Y
  mov x4, x4, lsl 16 //shift left 16
  orr x3, x3, x4
  mov x4, 128 //128 to subtract from U
  orr x3, x3, x4

  ins v12.d[0], x3 //put the constant (16,128,16,128) into a register

  //00111111100101001111110111110100 //1.164
  movz w5, 0b0011111110010100, lsl 16
  movk w5, 0b1111110111110100 //1.164
  dup v20.4s, w5 //copy to all elements

  //00111111110011000100100110111010 //1.596
  movz w5, 0b0011111111001100, lsl 16
  movk w5, 0b0100100110111010 //1.596
  dup v21.4s, w5 //copy to all elements

  //00111111010100000010000011000101  //.813
  movz w5, 0b0011111101010000, lsl 16
  movk w5, 0b0010000011000101 //.813
  dup v22.4s, w5 //copy to all elements

  //00111110110010001011010000111001 //.392
  movz w5, 0b0011111011001000, lsl 16
  movk w5, 0b1011010000111001 //.392
  dup v24.4s, w5 //copy to all elements

  //01000000000001101101100100010111 //2.017
  movz w5, 0b0100000000000110, lsl 16
  movk w5, 0b1101100100010111 //2.107
  dup v25.4s, w5 //copy to all elements

  //255.0
  movz w5, 0b0100001101111111, lsl 16
  movk w5, 0b0000000000000000 //255.0
  dup v26.4s, w5 //copy to all elements

  //register usage
  //v0 - first 2 UYVY
  //v1 - v0 swapped
  //v2 - third UYVY
  //v3 - 2 rows down - first 2 UYVY
  //v4 - v3 swapped
  //v5 - 2 rows down - third UYVY
  //v6-v7 - used 
  //v12 - constant (16,128,16,128)
  //v17-v19 - used
  //v20-v22 - constants
  //v24-v26 - constants
  //v29 - R
  //v30 - G
  //v31 - B
  //available = v8,v9,v10,v11,v13,v14,v15,v16,v27,v28

  //testing
  //v8,v9 - third UYVY swapped
  //10,11,13,14 - following 2 bytes, swapped, and next row
  //15,16 - sum of pixel values and average

outerloop:
  cmp w9, w12
  beq end

  mov w13,0

innerloop:
  //load the first 2 UYVY
  ld1 {v0.2s}, [x0] //load 8-bytes

  //skip down 2 rows (1920*2 * 2 bytes) = 7680
  add x4, x0, 3840
  add x4, x4, 3840
  add x5, x4, 8

  ext v1.8b, v0.8b, v0.8b, #4 //swap 4 bytes

  //load the third UYVY
  add x3, x0, 8
  ld1 {v2.2s}, [x3] //load 8-bytes
  ext v8.8b, v2.8b, v2.8b, #4 //swap 4 bytes

  //load the first 2 UYVY - 2 rows down
  ld1 {v3.2s}, [x4] //load 8-bytes
  ext v4.8b, v3.8b, v3.8b, #4 //swap 4 bytes

  //load the third UYVY - 2 rows down
  ld1 {v5.2s}, [x5] //load 8-bytes
  ext v9.8b, v5.8b, v5.8b, #4 //swap 4 bytes

  //add the words for the first pixel
  eor v6.16b, v6.16b, v6.16b //zero out v6
  mov v7.16b, v6.16b         //zero out v7
  uaddw v6.8h, v6.8h, v0.8b
  uaddw v6.8h, v6.8h, v1.8b //add the first and second words of current row

  uaddw v6.8h, v6.8h, v3.8b //add the first word of 2 rows below
  uaddw v6.8h, v6.8h, v4.8b //add the second word of 2 rows below

  //add the words for the second pixel
  uaddw v7.8h, v7.8h, v1.8b
  uaddw v7.8h, v7.8h, v2.8b //add the first and second words of current row

  uaddw v7.8h, v7.8h, v4.8b //add the first word of 2 rows below
  uaddw v7.8h, v7.8h, v5.8b //add the second word of 2 rows below

  //v6 and v7 have the YUV sum, divide by 4 to average
  ushr v6.8h, v6.8h, 2 //shift right to divide by 4
  ushr v7.8h, v7.8h, 2

  //subtract the constant after averaging
  ssubl v6.4s,v6.4h,v12.4h //subtract the constant (16,128,16,128)
  ssubl v7.4s,v7.4h,v12.4h

  //do FP computation to get BGR
  scvtf v6.4s, v6.4s //convert to floats, registers in format Y1-V0-Y0-U0
  scvtf v7.4s, v7.4s

  //////////////// load the third UYVY set of bytes (packet 4,5,6)
//  add x3, x3, 8
  add x5, x5, 8
  //load the packet 5,6 bytes (8 bytes)
 // ld1 {v10.2s}, [x3] //load 8-bytes
  ext v11.8b, v10.8b, v10.8b, #4 //swap 4 bytes

  //load the next rows packet 5,6 bytes (8 bytes)
  ld1 {v13.2s}, [x5] //load 8-bytes
  ext v14.8b, v13.8b, v13.8b, #4 //swap 4 bytes

  //add the words for the first pixel
  eor v15.16b, v15.16b, v15.16b //zero out v15
  mov v16.16b, v15.16b         //zero out v16

  uaddw v15.8h, v15.8h, v8.8b
  uaddw v15.8h, v15.8h, v10.8b //add the first and second words of current row

  uaddw v15.8h, v15.8h, v9.8b //add the first word of 2 rows below
  uaddw v15.8h, v15.8h, v13.8b //add the second word of 2 rows below

  //add the words for the second pixel
  uaddw v16.8h, v16.8h, v10.8b
  uaddw v16.8h, v16.8h, v11.8b //add the first and second words of current row

  uaddw v16.8h, v16.8h, v13.8b //add the first word of 2 rows below
  uaddw v16.8h, v16.8h, v14.8b //add the second word of 2 rows below

  //v15 and v16 have the YUV sum, divide by 4 to average
  ushr v15.8h, v15.8h, 2 //shift right to divide by 4
  ushr v16.8h, v16.8h, 2

  //subtract the constant after averaging
  ssubl v15.4s,v15.4h,v12.4h //subtract the constant (16,128,16,128)
  ssubl v16.4s,v16.4h,v12.4h

  //do FP computation to get BGR
  scvtf v15.4s, v15.4s //convert to floats, registers in format Y1-V0-Y0-U0
  scvtf v16.4s, v16.4s

  ///////////////////////////////////////////////////////////////

  mov v17.s[3], v16.s[3]  //move the Y into place (packet 4,5,6)
  mov v17.s[2], v15.s[1] 
  mov v17.s[1], v7.s[3]  //move the Y into place
  mov v17.s[0], v6.s[1] 

  fmul v17.4s, v17.4s, v20.4s //v17 contains 1.164*Y

  mov v18.s[3], v16.s[2]  //move the V into place (packet 4,5,6)
  mov v18.s[2], v15.s[2] 
  mov v18.s[1], v7.s[2]  //move the V into place
  mov v18.s[0], v6.s[2] 

  mov v29.16b, v17.16b  //where to hold the R values
  mov v30.16b, v17.16b  //where to hold the G values
  mov v31.16b, v17.16b  //where to hold the B values

  fmla v29.4s, v18.4s, v21.4s //R values computed
  
  fmls v30.4s, v18.4s, v22.4s //G compute -.813*V

  mov v23.s[3], v16.s[0]  //move the U into place (packet 4,5,6)
  mov v23.s[2], v15.s[0] 
  mov v23.s[1], v7.s[0]  //move the U into place
  mov v23.s[0], v6.s[0] 

  fmls v30.4s, v23.4s, v24.4s //G values compute: -.392*U

  fmla v31.4s, v23.4s, v25.4s //B values compute: 2.017*U

  ////////////////////////////////////////////
  //replace all values above 255
  fcmgt v17.4s, v31.4s, v26.4s //set max B values
  bit v31.16b, v26.16b, v17.16b

  fcmgt v19.4s, v30.4s, v26.4s //set max G values
  bit v30.16b, v26.16b, v19.16b

  fcmgt v17.4s, v29.4s, v26.4s //set max R values
  bit v29.16b, v26.16b, v17.16b

  //convert the fp to int (rounding toward zero)
  fcvtzu v31.4s, v31.4s //convert B to int
  fcvtzu v30.4s, v30.4s //convert G to int
  fcvtzu v29.4s, v29.4s //convert R to int

  ///////store 4 bytes BGRB
  mov w10, v31.s[1]
  lsl w10, w10, 8

  mov w11, v29.s[0] 
  orr w10, w10, w11
  lsl w10, w10, 8
  
  mov w11, v30.s[0] 
  orr w10, w10, w11
  lsl w10, w10, 8

  mov w11, v31.s[0]
  orr w10, w10, w11

  str w10,[x1] //store BGRB
  /* add x1, x1, 4 */

  ///////store 4 bytes GRBG
  mov w10, v30.s[2]
  lsl w10, w10, 8

  mov w11, v31.s[2] 
  orr w10, w10, w11
  lsl w10, w10, 8
  
  mov w11, v29.s[1] 
  orr w10, w10, w11
  lsl w10, w10, 8

  mov w11, v30.s[1]
  orr w10, w10, w11

  str w10,[x1,#4] //store GRBG
  /* add x1, x1, 4 */

  ///////store 4 bytes RBGR
  mov w10, v29.s[3]
  lsl w10, w10, 8

  mov w11, v30.s[3] 
  orr w10, w10, w11
  lsl w10, w10, 8
  
  mov w11, v31.s[3] 
  orr w10, w10, w11
  lsl w10, w10, 8

  mov w11, v29.s[2]
  orr w10, w10, w11

  str w10,[x1,#8] //store RBGR
  /* add x1, x1, 4 */

//////

  add x0, x0, 24 //move over 24 bytes to point to next set of 3 UYVY
  add x1, x1, 12

  add w13, w13, 1

  add x7, x0, 256 //prefetch
  prfm pldl1strm, [x7]

  cmp w13, w14
  bne innerloop

  add x0, x0, 3840  //move down 2 rows
  add x0, x0, 3840

  add w9, w9, 1 //increment the outer loop
  b outerloop
end:
  //str x15,[x10]

  //restore registers
  sub sp, sp, #128
  ld1 { v8.2d,  v9.2d, v10.2d, v11.2d}, [sp], #64
  ld1 {v12.2d, v13.2d, v14.2d, v15.2d}, [sp], #64
  ret
