#ifndef VIDEO_OUT_H

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

struct vo {
	pid_t ffmpeg_pid;
	int rawvideo_pipe;
	size_t frame_size;
	int frame;
};

static struct vo* vo_open(char* path, int width, int height, char* format)
{
	int rawvideo_pipe[2]; // [0]=read, [1]=write
	assert(pipe(rawvideo_pipe) == 0);

	pid_t p = fork();
	if (p == 0) {
		close(rawvideo_pipe[1]);
		dup2(rawvideo_pipe[0], 0);

		char video_size[1024];
		snprintf(video_size, sizeof video_size, "%dx%d", width, height);

		char framerate[1024];
		snprintf(framerate, sizeof framerate, "%d", 30); // XXX

		char* argv[] = {
			"ffmpeg",
			"-f", "rawvideo",
			"-vcodec", "rawvideo",
			"-pixel_format", format,
			"-video_size", video_size,
			"-framerate", framerate,
			"-i", "-", // stdin
			//"-c:v", "libvpx-vp9",
			//"-b:v", "2M",
			"-c:v", "libx264",
			"-preset", "slow",
			"-crf", "18",
			path,
			NULL
		};
		execvp(argv[0], argv);
		assert(!"unreachable");
		return NULL;
	}

	close(rawvideo_pipe[0]);

	struct vo* vo = (struct vo*)calloc(1, sizeof* vo);
	vo->ffmpeg_pid = p;
	vo->rawvideo_pipe = rawvideo_pipe[1];
	vo->frame_size = width*height*4; // XXX assumption
	return vo;
}

static void vo_frame(struct vo* vo, void* bitmap)
{
	printf("Frame #%d\n", ++vo->frame);
	uint8_t* p = (uint8_t*)bitmap;
	size_t remaining = vo->frame_size;
	while (remaining > 0) {
		int n = write(vo->rawvideo_pipe, p, remaining);
		assert(n > 0);
		p += n;
		remaining -= n;
	}
}


static int vo_close(struct vo* vo)
{
	close(vo->rawvideo_pipe);
	int ws;
	waitpid(vo->ffmpeg_pid, &ws, 0);
	return ws;
}

#define VIDEO_OUT_H
#endif
