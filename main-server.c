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
    free(wr);
}

static void on_close(uv_handle_t* handle) {
    struct client_ *c = to_container(struct client_, handle, client);
    list_remove(&c->list);
    free(c);
}

static void echo_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
    free_write_req(req);
}

static void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        struct client_ *c = to_container(struct client_, client, client);
        uv_stream_t *s = c->server;
        list_t head = uv_handle_get_data(s);
        list_t node = head->next;

        while (node != head)
        {
            c = list_entry(node, struct client_, list);
            write_req_t *req = (write_req_t*) malloc(sizeof(write_req_t));
            req->buf = uv_buf_init(buf->base, nread);
            uv_write((uv_write_t*) req, &c->client, &req->buf, 1, echo_write);
            node = node->next;
        }

        return;
    }
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) client, on_close);
    }

    free(buf->base);
}

void on_new_connection(uv_stream_t *server, int status) {
    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        // error!
        return;
    }

    struct client_ *c = (struct client_*) malloc(sizeof(struct client_));
    uv_tcp_init(loop, &c->client);
    list_init(&c->list);
    c->server = server;
    list_add_tail(uv_handle_get_data(server), &c->list);
    if (uv_accept(server, (uv_stream_t*) &c->client) == 0) {
        uv_read_start((uv_stream_t*) &c->client, alloc_buffer, echo_read);
    }
    else {
        uv_close((uv_handle_t*) &c->client, on_close);
    }
}

int main(int argc, char *argv[], char *env[])
{
    int r;
    struct list_tag clients;
    list_init(&clients);
    loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    term_init();

    uv_pipe_init(loop, &stdin_pipe, 0);
    uv_pipe_open(&stdin_pipe, 0);

    uv_signal_t sig;
    uv_signal_init(loop, &sig);
    uv_signal_start(&sig, signal_handler, SIGINT);

    uv_read_start((uv_stream_t*)&stdin_pipe, alloc_buffer, read_stdin);

    uv_tcp_t server;
    uv_tcp_init(loop, &server);
    uv_handle_set_data(&server, &clients);

    uv_ip4_addr("0.0.0.0", 7000, &addr);

    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    r = uv_listen((uv_stream_t*) &server, 2, on_new_connection);
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return 1;
    }

    //MAINLOOP
    r = uv_run(loop, UV_RUN_DEFAULT);

    uv__pipe_close(&stdin_pipe);
    uv_loop_close(loop);
    term_reset();
    free(loop);
    return r;
}
