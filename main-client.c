#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "list.h"
#include "utils.h"
#include "libuv/include/uv.h"

static uv_handle_t stdin_pipe;
static uv_loop_t *loop;
static struct termios oldT, newT;
struct sockaddr_in addr;

static void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

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

static void usage(void)
{
    fprintf(stderr, "q(uit)\n");
    fprintf(stderr, "h(elp)\n");
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
            usage();
            case '?':
            usage();
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

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;

struct client_
{
    struct list_tag list;
    uv_tcp_t client;
    uv_stream_t *server;
};

static void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*) req;
    fprintf(stderr, "free write req\n");
    free(wr);
}

static void on_close(uv_handle_t* handle) {
    fprintf(stderr, "on close\n");
}

static void echo_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
    free_write_req(req);
}

static void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        fprintf(stderr, "[%s]\n", buf->base);
        return;
    }
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) client, on_close);
    }

    free(buf->base);
}

static void on_connect(uv_connect_t *req, int status)
{
  uv_read_start((uv_stream_t*)req->handle, alloc_buffer, echo_read);
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

    uv_tcp_t client;
    uv_tcp_init(loop, &client);

    uv_ip4_addr("127.0.0.1", 7000, &addr);

    uv_connect_t connect_req;
    client.data = &connect_req;
    r = uv_tcp_connect(&connect_req,
                     &client,
                     (const struct sockaddr*) &addr,
                     on_connect);

    if (r) {
        fprintf(stderr, "Cannot connect %s\n", uv_strerror(r));
        return 1;
    }

    //MAINLOOP
    r = uv_run(loop, UV_RUN_DEFAULT);

    uv_close(&client, on_close);
    uv__pipe_close(&stdin_pipe);
    uv_loop_close(loop);
    term_reset();
    free(loop);
    return r;
}
