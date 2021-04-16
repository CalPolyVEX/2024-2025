/*
 * opencv_v4l2 - v4l2_helper.h file
 *
 * Copyright (c) 2017-2018, e-con Systems India Pvt. Ltd.  All rights reserved.
 *
 */
// Header file for v4l2_helper functions.

#ifndef V4L2_HELPER_H
#define V4L2_HELPER_H

#define GET 1
#define SET 2
#include <linux/videodev2.h>

#define ERR -128

#ifdef __cplusplus
extern "C" {
#endif

enum io_method {
	IO_METHOD_READ = 1,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR
};

/**
 * FIXME: All the state for the library is maintained via global variables.
 * So, it's not possible to use this library to access multiple devices
 * simultaneously. This is done to simplify the process of assessment.
 *
 * Hint: To access multiple devices at the same time using this application,
 * the public helper functions can be made to accept a new structure, that holds
 * the global state, as parameter.
 */

/*
 * All functions return 0 on success and ERR ( a negative value) in case of failure.
 */

int helper_init_cam(const char* devname, unsigned int width, unsigned int height, unsigned int format, enum io_method io_meth);

int helper_get_cam_frame(unsigned char** pointer_to_cam_data, int *size);

int helper_release_cam_frame();

int helper_deinit_cam();

//int helper_change_cam_res(unsigned int width, unsigned int height, unsigned int format, enum io_method io_meth);

//int helper_ctrl(unsigned int, int,int*);

//int helper_queryctrl(unsigned int,struct v4l2_queryctrl* );

#ifdef __cplusplus
}
#endif

#endif
