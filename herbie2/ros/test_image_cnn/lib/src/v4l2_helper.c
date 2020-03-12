/*
 * opencv_v4l2 - v4l2_helper.c file
 *
 * Copyright (c) 2017-2018, e-con Systems India Pvt. Ltd.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include "v4l2_helper.h"

#define NUM_BUFFS	4
#define CLEAR(x) memset(&(x), 0, sizeof(x))


struct buffer {
	void   *start;
	size_t  length;
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

static enum io_method   io = IO_METHOD_MMAP;
static int              fd = -1;
static struct buffer          *buffers;
static unsigned int     n_buffers;
static struct v4l2_buffer frame_buf;
static char is_initialised = 0, is_released = 1;

/**
 * Start of static (internal) helper functions
 */
static int xioctl(int fh, unsigned long request, void *arg)
{
	int r;

	do {
		r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

static int set_io_method(enum io_method io_meth)
{
	switch (io_meth)
	{
		case IO_METHOD_READ:
		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			io = io_meth;
			return 0;
		default:
			fprintf(stderr, "Invalid I/O method\n");
			return ERR;
	}
}

static int stop_capturing(void)
{
	enum v4l2_buf_type type;
	struct v4l2_requestbuffers req;

	switch (io) {
		case IO_METHOD_READ:
			/* Nothing to do. */
			break;

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
			{
				fprintf(stderr, "Error occurred when streaming off\n");
				return ERR;
			}
			break;
	}

	CLEAR(req);

	/*
	 * Request to clear the buffers. This helps with changing resolution.
	 */
	req.count = 0;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "The device does not support "
					"memory mapping\n");
		}
		return ERR;
	}

	return 0;
}

static int start_capturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

	switch (io) {
		case IO_METHOD_READ:
			/* Nothing to do. */
			break;

		case IO_METHOD_MMAP:
			for (i = 0; i < n_buffers; ++i) {
				struct v4l2_buffer buf;

				CLEAR(buf);
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_MMAP;
				buf.index = i;

				if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				{
					fprintf(stderr, "Error occurred when queueing buffer\n");
					return ERR;
				}
			}
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			{
				fprintf(stderr, "Error occurred when turning on stream\n");
				return ERR;
			}
			break;

		case IO_METHOD_USERPTR:
			for (i = 0; i < n_buffers; ++i) {
				struct v4l2_buffer buf;

				CLEAR(buf);
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_USERPTR;
				buf.index = i;
				buf.m.userptr = (unsigned long)buffers[i].start;
				buf.length = buffers[i].length;

				if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				{
					fprintf(stderr, "Error occurred when queueing buffer\n");
					return ERR;
				}
			}
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			{
				fprintf(stderr, "Error when turning on stream\n");
				return ERR;
			}
			break;
	}

	return 0;
}

static int uninit_device(void)
{
	unsigned int i;
	int ret = 0;

	switch (io) {
		case IO_METHOD_READ:
			free(buffers[0].start);
			break;

		case IO_METHOD_MMAP:
			for (i = 0; i < n_buffers; ++i)
				if (-1 == munmap(buffers[i].start, buffers[i].length))
					ret = ERR;
			break;

		case IO_METHOD_USERPTR:
			for (i = 0; i < n_buffers; ++i)
				free(buffers[i].start);
			break;
	}

	free(buffers);
	return ret;
}

static int init_read(unsigned int buffer_size)
{
	buffers = (struct buffer *) calloc(1, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		return ERR;
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);

	if (!buffers[0].start) {
		fprintf(stderr, "Out of memory\n");
		return ERR;
	}

	return 0;
}

static int init_mmap(void)
{
	struct v4l2_requestbuffers req;
	int ret = 0;

	CLEAR(req);

	req.count = NUM_BUFFS;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "The device does not support "
					"memory mapping\n");
		}
		return ERR;
	}

	if (req.count < 1) {
		fprintf(stderr, "Insufficient memory to allocate "
				"buffers");
		return ERR;
	}

	buffers = (struct buffer *) calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		return ERR;
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		int loop_err = 0;
		struct v4l2_buffer buf;

		CLEAR(buf);
		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
		{
			fprintf(stderr, "Error occurred when querying buffer\n");
			loop_err = 1;
			goto LOOP_FREE_EXIT;
		}

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
			mmap(NULL /* start anywhere */,
					buf.length,
					PROT_READ | PROT_WRITE /* required */,
					MAP_SHARED /* recommended */,
					fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start) {
			fprintf(stderr, "Error occurred when mapping memory\n");
			loop_err = 1;
			goto LOOP_FREE_EXIT;
		}

LOOP_FREE_EXIT:
		if (loop_err)
		{
			unsigned int curr_buf_to_free;
			for (curr_buf_to_free = 0;
				curr_buf_to_free < n_buffers;
				curr_buf_to_free++)
			{
				if (
					munmap(buffers[curr_buf_to_free].start,
					buffers[curr_buf_to_free].length) != 0
				)
				{
					/*
					 * Errors ignored as mapping itself
					 * failed for a buffer
					 */
				}
			}
			free(buffers);
			return ERR;
		}
	}

	return ret;
}

static int init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count  = NUM_BUFFS;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "The device does not "
					"support user pointer i/o\n");
		}
		return ERR;
	}

	buffers = (struct buffer *) calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		return ERR;
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		if(posix_memalign(&buffers[n_buffers].start,getpagesize(),buffer_size) != 0)
		{
			/*
			 * This happens only in case of ENOMEM
			 */
			unsigned int curr_buf_to_free;
			for (curr_buf_to_free = 0;
				curr_buf_to_free < n_buffers;
				curr_buf_to_free++
			)
			{
				free(buffers[curr_buf_to_free].start);
			}
			free(buffers);
			fprintf(stderr, "Error occurred when allocating memory for buffers\n");
			return ERR;
		}
	}

	return 0;
}

static int init_device(unsigned int width, unsigned int height, unsigned int format)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;

	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "Given device is no V4L2 device\n");
		}
		fprintf(stderr, "Error occurred when querying capabilities\n");
		return ERR;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "Given device is no video capture device\n");
		return ERR;
	}

	switch (io) {
		case IO_METHOD_READ:
			if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
				fprintf(stderr, "Given device does not "
						"support read i/o\n");
				return ERR;
			}
			break;

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
				fprintf(stderr, "Given device does not "
						"support streaming i/o\n");
				return ERR;
			}
			break;
	}


	/* Select video input, video standard and tune here. */

	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
				case EINVAL:
					/* Cropping not supported. */
					break;
				default:
					/* Errors ignored. */
					break;
			}
		}
	} else {
		/* Errors ignored. */
	}

	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = width;
	fmt.fmt.pix.height      = height;
	fmt.fmt.pix.pixelformat = format;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
	{
		fprintf(stderr, "Error occurred when trying to set format\n");
		return ERR;
	}

	/* Note VIDIOC_S_FMT may change width and height. */

	printf("pixfmt = %c %c %c %c \n", (fmt.fmt.pix.pixelformat & 0x000000ff) , (fmt.fmt.pix.pixelformat & 0x0000ff00) >>8 , (fmt.fmt.pix.pixelformat & 0x00ff0000) >>16, (fmt.fmt.pix.pixelformat & 0xff000000) >>24 );
	printf("width = %d height = %d\n",fmt.fmt.pix.width,fmt.fmt.pix.height);

	if (
		fmt.fmt.pix.width != width ||
		fmt.fmt.pix.height != height ||
		fmt.fmt.pix.pixelformat != format
	)
	{
		fprintf(stderr, "Warning: The current format does not match requested format/resolution!\n");
		return ERR;
	}

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	switch (io) {
		case IO_METHOD_READ:
			return init_read(fmt.fmt.pix.sizeimage);
			break;

		case IO_METHOD_MMAP:
			return init_mmap();
			break;

		case IO_METHOD_USERPTR:
			return init_userp(fmt.fmt.pix.sizeimage);
			break;
	}

	return 0;
}

static int close_device(void)
{
	if (-1 == close(fd))
	{
		fprintf(stderr, "Error occurred when closing device\n");
		return ERR;
	}

	fd = -1;

	return 0;
}

static int open_device(const char *dev_name)
{
	struct stat st;

	if (-1 == stat(dev_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n",
				dev_name, errno, strerror(errno));
		return ERR;
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", dev_name);
		return ERR;
	}

	fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (-1 == fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n",
				dev_name, errno, strerror(errno));
		return ERR;
	}

	return fd;
}
/**
 * End of static (internal) helper functions
 */


/**
 * Start of public helper functions
 */
int helper_init_cam(const char* devname, unsigned int width, unsigned int height, unsigned int format, enum io_method io_meth)
{
	if (is_initialised)
	{
		/*
		 * The library currently doesn't support
		 * accessing multiple devices simultaneously.
		 * So, return an error when such access is attempted.
		 */
		fprintf(stderr, "Cannot use the library to initialise multiple devices, simultaneously.\n");
		return ERR;
	}

	if(
		set_io_method(io_meth) < 0 ||
		open_device(devname) < 0 ||
		init_device(width,height,format) < 0 ||
		start_capturing() < 0
	)
	{
		fprintf(stderr, "Error occurred when initialising camera\n");
		return ERR;
	}

	is_initialised = 1;
	return 0;
}

int helper_deinit_cam()
{
	if (!is_initialised)
	{
		fprintf(stderr, "Error: trying to de-initialise without initialising camera\n");
		return ERR;
	}

	/*
	 * It's better to turn off is_initialised even if the
	 * de-initialisation fails as it shouldn't have affect
	 * re-initialisation a lot.
	 */
	is_initialised = 0;

	if(
		stop_capturing() < 0 ||
		uninit_device() < 0 ||
		close_device() < 0
	)
	{
		fprintf(stderr, "Error occurred when de-initialising camera\n");
		return ERR;
	}

	return 0;
}

int helper_get_cam_frame(unsigned char **pointer_to_cam_data, int *size)
{
	static unsigned char max_timeout_retries = 10;
	unsigned char timeout_retries = 0;

	if (!is_initialised)
	{
		fprintf (stderr, "Error: trying to get frame without successfully initialising camera\n");
		return ERR;
	}

	if (!is_released)
	{
		fprintf (stderr, "Error: trying to get another frame without releasing already obtained frame\n");
		return ERR;
	}

	for (;;) {
		fd_set fds;
		struct timeval tv;
		int r;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		/* Timeout. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		r = select(fd + 1, &fds, NULL, NULL, &tv);

		if (-1 == r) {
			if (EINTR == errno)
				continue;
		}

		if (0 == r) {
			fprintf(stderr, "select timeout\n");
			timeout_retries++;

			if (timeout_retries == max_timeout_retries)
			{
				fprintf(stderr, "Could not get frame after multiple retries\n");
				return ERR;
			}
		}

		CLEAR(frame_buf);
		frame_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(fd, VIDIOC_DQBUF, &frame_buf)) {
			switch (errno) {
				case EAGAIN:
					continue;

				case EIO:
					/* Could ignore EIO, see spec. */

					/* fall through */

				default:
					continue;
			}
		}
		*pointer_to_cam_data = (unsigned char*) buffers[frame_buf.index].start;
		*size = frame_buf.bytesused;
		break;
		/* EAGAIN - continue select loop. */
	}

	is_released = 0;
	return 0;
}

int helper_release_cam_frame()
{
	if (!is_initialised)
	{
		fprintf (stderr, "Error: trying to release frame without successfully initialising camera\n");
		return ERR;
	}

	if (is_released)
	{
		fprintf (stderr, "Error: trying to release already released frame\n");
		return ERR;
	}

	if (-1 == xioctl(fd, VIDIOC_QBUF, &frame_buf))
	{
		fprintf(stderr, "Error occurred when queueing frame for re-capture\n");
		return ERR;
	}

	/*
	 * We assume the frame hasn't been released if an error occurred as
	 * we couldn't queue the frame for streaming.
	 *
	 * Assuming it to be released in case an error occurs causes issues
	 * such as the loss of a buffer, etc.
	 */
	is_released = 1;
	return 0;
}

/**
 * End of public helper functions
 */

/**
 * Untested for misusages
 */


/*
int helper_change_cam_res(unsigned int width, unsigned int height, unsigned int format, enum io_method io_meth)
{
	if (!is_initialised)
	{
		fprintf(stderr, "Error: trying to de-initialise without initialising camera\n");
		return ERR;
	}

	if (
		stop_capturing() < 0 ||
		uninit_device() < 0
	)
	{
		fprintf(stderr, "Error occurred when ude-initializing device to change camera resolution\n");
		return ERR;
	}

	is_initialised = 0;

	if (
		set_io_method(io_meth) < 0 ||
		init_device(width,height,format) < 0 ||
		start_capturing() < 0
	)
	{
		fprintf(stderr, "Error occurred when changing camera resolution\n");
		return ERR;
	}

	is_initialised = 1;

	return 0;
}

int helper_queryctrl(unsigned int id,struct v4l2_queryctrl* qctrl)
{
	if (!is_initialised)
	{
		fprintf(stderr, "Error: trying to query control without initialising camera\n");
		return ERR;
	}

	qctrl->id = id;
	if (-1 == xioctl(fd, VIDIOC_QUERYCTRL, qctrl)) {
		fprintf(stderr, "Error QUERYCTRL\n");
		return -1;
	}

	return 0;
}

int helper_ctrl(unsigned int id, int flag,int* value)
{
	if (!is_initialised)
	{
		fprintf(stderr, "Error: trying to get/set control without initialising camera\n");
		return ERR;
	}

	unsigned int ioctl_num = 0;
	struct v4l2_control ctrl;
	ctrl.id = id;

	if (flag == GET) {
		ioctl_num = VIDIOC_G_CTRL;
	} else if (flag == SET) {
		ioctl_num = VIDIOC_S_CTRL;
		ctrl.value = *value;
	}
	if (-1 == xioctl(fd, ioctl_num, &ctrl)) {
		printf("Error GET/SET\n");
		return -1;
	}

	if (flag == GET) {
		*value = ctrl.value;
	}

	return 0;
}
*/
