default:
	gcc -o resize -O3 -g fast_resize.c
arm:
	gcc -o resize -O3 -g test_neon.c

opencv:
	gcc -o resize -O3 -g -lopencv -I/usr/include/opencv4 fast_resize.c
