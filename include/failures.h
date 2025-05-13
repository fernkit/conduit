typedef enum {
    ERR_HOSTNAME_RESOLUTION,
    ERR_SOCKET_CREATION,
    ERR_SEVER_CONNECTION,
    ERR_SEND_HTTP_REQ,
    ERR_BUFF_OVERFLOW,
    ERR_RECEIVING_DATA
} ErrorCode;

const char* get_error_message(ErrorCode code);