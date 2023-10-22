#include <sys/ioctl.h>
#include <fcntl.h>

#include "usb_test.h"
#include "uvcvideo.h"

extern int msleep(unsigned int msecs);

static void *uvc_thread = NULL;
static void *file_thread = NULL;
static hal_mailbox_t uvc_mailbox = NULL;

static int save_frame_to_file(void *str, void *start, int length)
{
	FILE *fp = NULL;

	fp = fopen(str, "wb+"); //save more frames
	if (!fp) {
		printf(" Open %s error\n", (char *)str);

		return -1;
	}

	if (fwrite(start, length, 1, fp)) {
		fclose(fp);

		return 0;
	} else {
		printf(" Write file fail (%s)\n", strerror(errno));
		fclose(fp);

		return -1;
	}

	return 0;
}

void usb_uvc_file_thread(void *para)
{
	char source_data_path[64];
	unsigned int value = 0;
	struct v4l2_buffer *mailbuf;
	int np = 0;
	while(1) {
		hal_mailbox_recv(uvc_mailbox, &value, -1);
		if (value != 0) {
			mailbuf = (struct v4l2_buffer *)(uintptr_t)value;
			printf("np = %d\n", np);
			sprintf(source_data_path, "/data/source_frame_%d.jpg", np++);
			save_frame_to_file(source_data_path, (uint32_t *)((int64_t)mailbuf->mem_buf), mailbuf->length);
			free((void *)((int64_t)mailbuf->mem_buf));
			free(mailbuf);
			mailbuf = NULL;
			value = 0;
		}
	}
}

void usb_uvc_test_thread(void *para)
{
	int fd;
	struct v4l2_capability cap;     /* Query device capabilities */
	struct v4l2_streamparm parms;   /* set streaming parameters */
	struct v4l2_format fmt;         /* try a format */
	struct v4l2_requestbuffers req; /* Initiate Memory Mapping or User Pointer I/O */
	struct v4l2_buffer buf;         /* Query the status of a buffer */
	struct v4l2_buffer *mailbuf = NULL;
	enum v4l2_buf_type type;
	int n_buffers;
	int np;

	/* 1.open /dev/videoX node */
	fd = open("/dev/video", O_RDWR);

	/* 2.Query device capabilities */
	memset(&cap, 0, sizeof(cap));
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
		printf(" Query device capabilities fail!!!\n");
	} else {
		printf(" Querey device capabilities succeed\n");
		printf(" cap.driver=%s\n", cap.driver);
		printf(" cap.card=%s\n", cap.card);
		printf(" cap.bus_info=%s\n", cap.bus_info);
		printf(" cap.version=0x%08x\n", cap.version);
		printf(" cap.capabilities=0x%08x\n", cap.capabilities);
	}

	/* 7.set streaming parameters */
	memset(&parms, 0, sizeof(struct v4l2_streamparm));
	parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parms.parm.capture.timeperframe.numerator = 1;
	parms.parm.capture.timeperframe.denominator = 30;
	if (ioctl(fd, VIDIOC_S_PARM, &parms) < 0) {
		printf(" Setting streaming parameters failed, numerator:%d denominator:%d\n",
			   parms.parm.capture.timeperframe.numerator,
			   parms.parm.capture.timeperframe.denominator);
		close(fd);
		return;
	}

	/* 9.set the data format */
	memset(&fmt, 0, sizeof(struct v4l2_format));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	fmt.fmt.pix.width = 320;
	fmt.fmt.pix.height = 240;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;

	if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
		printf(" setting the data format failed!\n");
		close(fd);
		return;
	}

	/* 10.Initiate Memory Mapping or User Pointer I/O */
	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	req.count = 5;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
		printf(" VIDIOC_REQBUFS failed\n");
		close(fd);
		return;
	}

	/* 11.Exchange a buffer with the driver */
	for (n_buffers = 0; n_buffers < req.count; n_buffers++) {
		memset(&buf, 0, sizeof(struct v4l2_buffer));

		buf.index = n_buffers;
		if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
			printf(" VIDIOC_QBUF error\n");

			close(fd);
			return;
		}
	}

	/* streamon */
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
		printf(" VIDIOC_STREAMON error! %s\n", strerror(errno));
	} else
		printf(" stream on succeed\n");

	np = 0;
	while (1) {
		printf(" camera capture num is [%d]\n", np);

		/* wait uvc frame */
		memset(&buf, 0, sizeof(struct v4l2_buffer));

		if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
			printf(" VIDIOC_DQBUF error\n");

			goto EXIT;
		} else {
			// printf("*****DQBUF[%d] FINISH*****\n", buf.index);
		}
		mailbuf = malloc(sizeof(struct v4l2_buffer));
		mailbuf->mem_buf = (uint32_t)(uintptr_t)malloc(buf.length);
		if (mailbuf->mem_buf != 0) {
		    memcpy((uint32_t *)((uint64_t)mailbuf->mem_buf), (uint32_t *)((uint64_t)buf.mem_buf), buf.length);
		    mailbuf->length = buf.length;
		    if (hal_mailbox_send_wait(uvc_mailbox, (uint32_t)(uintptr_t)mailbuf, 100) < 0) {
		        printf("uvc data send failed, data lost\n");
		    }
		}
		if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
			printf(" VIDIOC_QBUF error\n");

			goto EXIT;
		} else {
			// printf("************QBUF[%d] FINISH**************\n\n", buf.index);
		}

		np++;
	}

	printf("\n\n Capture thread finish\n");

EXIT:
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(fd, VIDIOC_STREAMOFF, &type);

	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	req.count = 0;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	ioctl(fd, VIDIOC_REQBUFS, &req);

	close(fd);
	while (hal_is_queue_empty(uvc_mailbox) != 1) {
		msleep(100);
	}
	printf("close.......\n");
	hal_mailbox_delete(uvc_mailbox);
	hal_thread_stop(file_thread);
	hal_thread_stop(uvc_thread);
}

int usb_test_cmd_uvc(int argc, const char **argv)
{
	uvc_mailbox = hal_mailbox_create("ucv_queue", 320);
	if (uvc_mailbox == NULL) {
		printf("mailbox create failed\n");
		goto fail_exit1;
	}
	printf("uvc_mailbox create sucess!\n");
	// usb thread is HAL_THREAD_PRIORITY_SYS,must be lower than HAL_THREAD_PRIORITY_SYS
	uvc_thread = hal_thread_create(usb_uvc_test_thread, NULL, "uvc_thread", 4 * 1024,
								   (HAL_THREAD_PRIORITY_APP + 1));
	if (uvc_thread == NULL) {
		printf("usb_uvc_test_thread create failed\n");
		goto fail_exit2;
	}
	hal_thread_start(uvc_thread);

	file_thread = hal_thread_create(usb_uvc_file_thread, NULL, "uvc_file_thread", 2 * 1024,
								   (HAL_THREAD_PRIORITY_APP));
	if (file_thread == NULL) {
		printf("uvc file thread create failed\n");
		goto fail_exit3;
	}
	hal_thread_start(file_thread);

	return 0;
fail_exit3:
	hal_thread_stop(uvc_thread);
fail_exit2:
	hal_mailbox_delete(uvc_mailbox);
fail_exit1:
	return -1;
}