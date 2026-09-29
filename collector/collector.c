
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uv_loop_t *loop;
static const char *SOCKET_PATH = "/tmp/my_socket";
static const char *GO_SOCKET_PATH = "/tmp/go_agent.sock";

static int go_socket_fd = -1;

int ConnectToGo();
int SendToGo(const char *message, size_t length);

typedef struct {
    size_t message_length;
    size_t received;
    char *message;
} MessageReader;

void ParsePayload(const char *message, size_t length){
    printf("Collector: Complete message: %.*s\n", (int)length, message);
}

void ReadMessage(MessageReader *reader, const char *buf, size_t nread)
{
    size_t offset = 0;

    if (reader->message_length == 0) {
        if (nread < 8) {
            return;
        }

        memcpy(&reader->message_length, buf, 8);

        reader->message = malloc(reader->message_length);

        offset = 8;
    }

    size_t remaining = reader->message_length - reader->received;
    size_t available = nread - offset;
    size_t copy = available < remaining ? available : remaining;

    memcpy(reader->message + reader->received, buf + offset, copy);

    reader->received += copy;

    if (reader->received == reader->message_length) {
        //used for printing do for now
        // ParsePayload(reader->message, reader->message_length);

        SendToGo(reader->message, reader->message_length);

        free(reader->message);

        reader->message = NULL;
        reader->message_length = 0;
        reader->received = 0;
    }
}

void OnAllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

void OnRead(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{
    if (nread > 0) {
        MessageReader *reader = client->data;

        ReadMessage(reader, buf->base, nread);
    }

    if (nread < 0) {
        uv_close((uv_handle_t *)client, NULL);
    }

    free(buf->base);
}

void OnNewConnection(uv_stream_t *server, int status)
{
    if (status < 0) {
        fprintf(stderr, "Collector: ERROR: connection failed: %s\n", uv_strerror(status));
        return;
    }

    uv_pipe_t *client = malloc(sizeof(uv_pipe_t));

    uv_pipe_init(loop, client, 0);

    MessageReader *reader = calloc(1, sizeof(MessageReader));

    client->data = reader;

    if (uv_accept(server, (uv_stream_t *)client) == 0) {
        uv_read_start((uv_stream_t *)client, OnAllocBuffer, OnRead);
    } else {
        free(reader);
        uv_close((uv_handle_t *)client, NULL);
    }
}





#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
int ConnectToGo(){
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        perror("Collector: ERROR: socket");
        return -1;
    }

    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, GO_SOCKET_PATH);

    if (connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Collector: ERROR: connect to Go");
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}




int SendToGo(const char *message, size_t length)
{
    if (go_socket_fd == -1)
    {
        printf("Collector: Go socket not connected, attempting connection...\n");
        go_socket_fd = ConnectToGo();
        if (go_socket_fd == -1)
        {
            fprintf(stderr, "Collector: ERROR: failed to connect to Go socket\n");
            return -1;
        }
        printf("Collector: Connected to Go socket successfully\n");
    }

    if (write(go_socket_fd, &length, sizeof(length)) == -1)
    {
        perror("Collector: ERROR: failed to write message length");
        close(go_socket_fd);
        go_socket_fd = -1;
        return -1;
    }

    if (write(go_socket_fd, message, length) == -1)
    {
        perror("Collector: ERROR: failed to write message");
        close(go_socket_fd);
        go_socket_fd = -1;
        return -1;
    }

    printf("Collector: Message sent to Go successfully (%zu bytes)\n", length);
    return 0;
}



int main()
{
    loop = uv_default_loop();

    uv_pipe_t server;

    uv_pipe_init(loop, &server, 0);

    unlink(SOCKET_PATH);

    if (uv_pipe_bind(&server, SOCKET_PATH) < 0) {
        fprintf(stderr, "Collector: ERROR: failed to bind\n");
        return 1;
    }

    if (uv_listen((uv_stream_t *)&server, 1, OnNewConnection) < 0) {
        fprintf(stderr, "Collector: ERROR: failed to listen\n");
        return 1;
    }

    printf("Collector listening on %s\n", SOCKET_PATH);

    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}