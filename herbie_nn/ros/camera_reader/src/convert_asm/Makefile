default:  main.c test_convert.S
	~/arm-gcc/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc -march=armv8-a+simd main.c test_convert.S
	#arm-none-eabi-gcc -march=armv8-a+simd -mfpu=neon --specs=nosys.specs test.c

arm:  main.c test_convert.S
	#gcc -march=armv8-a+simd main.c test_convert.S
	gcc main.c test_convert.S -O2
	
