.section .text
.globl asm_foo
.type asm_foo, @function
asm_foo:
  /* asm code with proper stack manipulation for C calling conventions */
  //add v31.2s,v31.2s,v31.2s

  //load the first 2 UYVY
  ld1 {v0.2s}, [x0] //load 8-bytes, post-increment x0 by 8
  ext v1.8b, v0.8b, v0.8b, #4 //swap 4 bytes

  //load the third UYVY
  add x3, x0, 4
  ld1 {v2.2s}, [x3] //load 8-bytes
  /* ext v3.8b, v2.8b, v2.8b, #4 //swap 4 bytes */

  //skip down 2 rows (1920*2 * 2 bytes) = 7680
  add x4, x0, 3840
  add x4, x4, 3840
  add x5, x4, 4

  //load the first 2 UYVY
  ld1 {v3.2s}, [x4] //load 8-bytes, post-increment x0 by 8
  ext v1.8b, v0.8b, v0.8b, #4 //swap 4 bytes

  //load the third UYVY
  ld1 {v4.2s}, [x5] //load 8-bytes





  //get lane
  //mov x3, v0.d[0]
  mov w3, v2.s[0]
  //mov w3,w3,lsr 8
  
  //mov x3, -2 //move constant -2 into x3
  //ldr x3,[x0] //load into a register
  str x3,[x1]
  ret
