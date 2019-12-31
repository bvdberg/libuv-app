#include <stdio.h>

#include "libuv/include/uv.h"

static uv_handle_t stdin_pipe;
static uv_loop_t *loop;

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>

struct termios oldT, newT;
static void term_init(void)
{
    ioctl(0, TCGETS, &oldT);
    newT = oldT;
    newT.c_lflag &= ~ECHO;
    newT.c_lflag &= ~ICANON;
    newT.c_cc[VMIN] = 0;
    newT.c_cc[VTIME] = 0;

    ioctl(0, TCSETS, &newT);
}

static void term_reset()
{
    ioctl(0, TCSETS, &oldT);
}

void read_stdin(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0){
        if (nread == UV_EOF){
            // end of file
            uv_close((uv_handle_t *)&stdin_pipe, NULL);
        }
    } else if (nread > 0) {
        switch (buf->base[0])
        {
            case 'q':
            uv_stop(loop);
            break;
            case 'h':
            fprintf(stderr, "usage\n");
            break;
        }
    }

    // OK to free buffer as write_data copies it.
    if (buf->base)
        free(buf->base);
}

int main(int argc, char *argv[], char *env[])
{
    loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    term_init();

    uv_pipe_init(loop, &stdin_pipe, 0);
    uv_pipe_open(&stdin_pipe, 0);

    uv_read_start((uv_stream_t*)&stdin_pipe, alloc_buffer, read_stdin);

    //MAINLOOP
    uv_run(loop, UV_RUN_DEFAULT);

    uv_loop_close(loop);
    term_reset();
    free(loop);
    return 0;
}
