#include "../include/failures.h"

const char* get_error_message(ErrorCode code){
    switch(code) {
        case ERR_HOSTNAME_RESOLUTION:
            return "Error: Could not resolve hostname";
        case ERR_SOCKET_CREATION:
            return "Error creating socket";
        case ERR_SEVER_CONNECTION:
            return "Error connecting to server";
        case ERR_SEND_HTTP_REQ:
            return "Error sending http request";
        case ERR_BUFF_OVERFLOW:
            return "Buffer Overflowed";
        case ERR_RECEIVING_DATA:
            return "Error Revieving data";
        defualt: 
            return "Unknown Error";
    }
}