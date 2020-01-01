#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "libuv/include/uv.h"

static uv_pipe_t stdin_pipe;
static uv_loop_t *loop;
static struct termios oldT, newT;
static uv_timer_t timer;

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

static void term_init(void)
{
    (void)ioctl(0, TCGETS, &oldT);
    newT = oldT;
    newT.c_lflag &= ~ECHO;
    newT.c_lflag &= ~ICANON;
    newT.c_cc[VMIN] = 0;
    newT.c_cc[VTIME] = 0;

    (void)ioctl(0, TCSETS, &newT);
}

static void term_reset()
{
    (void)ioctl(0, TCSETS, &oldT);
}

static void usage(void)
{
    fprintf(stderr, "q(uit)\n");
    fprintf(stderr, "h(elp)\n");
    fprintf(stderr, "s(top) timer\n");
    fprintf(stderr, "r(estart) timer\n");
    fprintf(stderr, "4(exitcode 4)\n");
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
            case '4':
            uv_stop_exitcode(loop, 4);
            break;
            case 'h':
            usage();
            break;
            case '?':
            usage();
            break;
            case 's':
                uv_timer_stop(&timer);
                fprintf(stderr, "stop timer\n");
            break;
            case 'r':
                uv_timer_again(&timer);
                fprintf(stderr, "restart timer\n");
            break;

        }
    }

    // OK to free buffer as write_data copies it.
    if (buf->base)
        free(buf->base);
}

static void signal_handler(uv_signal_t *req, int signum)
{
    fprintf(stderr, "Caugth signal %d\n", signum);
    uv_stop(loop);
    uv_signal_stop(req);
}

static void timer_func(uv_timer_t* handle)
{
    fprintf(stderr, "timer called, missing cookie %p\n", uv_handle_get_data((uv_handle_t*)handle));
}

int main(int argc, char *argv[], char *env[])
{
    int r;

    loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    term_init();

    uv_pipe_init(loop, &stdin_pipe, 0);
    uv_pipe_open(&stdin_pipe, 0);

    uv_signal_t sig;
    uv_signal_init(loop, &sig);
    uv_signal_start(&sig, signal_handler, SIGINT);

    uv_read_start((uv_stream_t*)&stdin_pipe, alloc_buffer, read_stdin);

    uv_timer_init(loop, &timer);
    uv_timer_start(&timer, timer_func, 1, 2000);
    uv_handle_set_data((uv_handle_t*)&timer, (void*)4);

    //MAINLOOP
    r = uv_run(loop, UV_RUN_DEFAULT);

    uv_close((uv_handle_t*)&stdin_pipe, NULL);
    uv_loop_close(loop);
    uv_close((uv_handle_t*)&timer, NULL);
    term_reset();
    free(loop);
    return r;
}
