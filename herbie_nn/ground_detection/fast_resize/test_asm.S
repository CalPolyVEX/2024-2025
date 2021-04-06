.section .text
.globl asm_foo
.type asm_foo, @function
asm_foo:
  //load the first 2 UYVY
  ld1 {v0.2s}, [x0] //load 8-bytes, post-increment x0 by 8
  ext v1.8b, v0.8b, v0.8b, #4 //swap 4 bytes

  //load the third UYVY
  add x3, x0, 8
  ld1 {v2.2s}, [x3] //load 8-bytes

  //skip down 2 rows (1920*2 * 2 bytes) = 7680
  add x4, x0, 3840
  add x4, x4, 3840
  add x5, x4, 8

  //load the first 2 UYVY
  ld1 {v3.2s}, [x4] //load 8-bytes, post-increment x0 by 8
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
  mov x3, x3, lsl 32 //shift left 24
  mov x4, 16 //16 to subtract from Y
  mov x4, x4, lsl 16 //shift left 16
  orr x3, x3, x4
  mov x4, 128 //128 to subtract from U
  orr x3, x3, x4

  ins v12.s[0], w3

  sub v10.8h,v10.8h,v12.8h
  sub v11.8h,v11.8h,v12.8h

  //do FP computation to get BGR
  eor v13.8b, v13.8b, v13.8b //zero out v13
  saddw v14.4s, v13.4s, v10.4h
  saddw v15.4s, v13.4s, v11.4h
  



  //get lane
  //mov x3, v0.d[0]
  mov w3, v14.s[2]
  //mov w3,w3,lsr 16
  
  //mov x3, -2 //move constant -2 into x3
  //ldr x3,[x0] //load into a register
  str x3,[x1]
  ret
