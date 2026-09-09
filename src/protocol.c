#include "cci_protocol.h"

#include <sys/socket.h>
#include <stddef.h>
#include <stdint.h>

static int recv_exact(int socket_fd, void *buffer, size_t length)
{
    size_t received_bytes = 0;
    uint8_t *bytes = buffer;

    while (received_bytes < length) {
        ssize_t count = recv(socket_fd, bytes + received_bytes, length - received_bytes, 0);

        if (count == 0) {
            return -2;
        }
        if (count < 0) {
            return -1;
        }

        received_bytes += (size_t)count;
    }

    return 0;
}

static int send_exact(int socket_fd, const void *buffer, size_t length)
{
    size_t sent = 0;
    const uint8_t *bytes = buffer;

    while (sent < length)
    {
        ssize_t count = send(socket_fd, bytes + sent, length - sent, 0);


        if (count <= 0) {
            return -1;
        }

        sent += (size_t)count;
    }
    return 0;
}

int cci_read_frame(int socket_fd, CCIFrame *out_frame)
{
    if (out_frame == NULL) {
        return -1;
    }

    uint8_t header[CCI_HEADER_SIZE];

    int rc = recv_exact(socket_fd, header, CCI_HEADER_SIZE);

    if (rc < 0) {
        return rc;
    }

    CCIHeader header_frame;

    header_frame.magic = header[0] ;
    header_frame.version = header[1];

    header_frame.type = ((uint16_t)header[2] << 8) | (uint16_t)header[3];
    header_frame.payload_length = ((uint32_t)header[4] << 24) | ((uint32_t)header[5] << 16) | ((uint32_t)header[6] << 8) | ((uint32_t)header[7]);


    if (header_frame.magic != CCI_MAGIC || header_frame.version != CCI_VERSION || header_frame.payload_length > CCI_MAX_PAYLOAD) {
        return -1;
    }

    out_frame->header = header_frame;

    if (out_frame->header.payload_length > 0) {
        rc = recv_exact(socket_fd, out_frame->payload, out_frame->header.payload_length);
        if (rc < 0) {
            return rc;
        }
    };

    return 0;
}


int cci_write_frame(int socket_fd, const CCIFrame *frame)
{
    if (frame == NULL) {
        return -1;
    }

    if (frame->header.payload_length > CCI_MAX_PAYLOAD) {
        return -1;
    }

    int rc = -1;
    uint8_t buffer[CCI_HEADER_SIZE];


    buffer[0] = frame->header.magic;
    buffer[1] = frame->header.version;

    buffer[2] = (uint8_t)(frame->header.type >> 8);
    buffer[3] = (uint8_t)(frame->header.type & 0xFF);

    buffer[4] = (uint8_t)(frame->header.payload_length >> 24);
    buffer[5] = (uint8_t)((frame->header.payload_length >> 16) & 0xFF);
    buffer[6] = (uint8_t)((frame->header.payload_length >> 8) & 0xFF);
    buffer[7] = (uint8_t)(frame->header.payload_length & 0xFF);


    if (frame->header.payload_length > 0) {
       rc = send_exact(socket_fd, buffer, CCI_HEADER_SIZE);
    }
    
    if (rc < 0) {
       return rc;
    }

    return 0;
}


int cci_build_frame(CCIFrame *frame, CCIMessageType type, const uint8_t *payload, uint32_t payload_length)
{
    if (frame == NULL) {
        return -1;
    }

    if (frame->payload > CCI_MAX_PAYLOAD) {
        return -1;
    }

    frame->header.magic = CCI_MAGIC;
    frame->header.version = CCI_VERSION;
    frame->header.type = type;
    frame->header.payload_length = payload_length;
    
    for (size_t b = 0; b < payload_length; b++) {
        frame->payload[b] = payload[b];
    }

    return 0;
}


int cci_parse_register(const CCIFrame *frame, char *username, size_t username_capacity)
{
    if (frame == NULL || username == NULL || frame->header.type != CCI_REGISTER) {
        return -1;
    }

    if (frame->header.payload_length == 0) {
        return -1;
    }

    if (frame->header.payload_length >= username_capacity) {
        return -1;
    }

    for (size_t b = 0; b < frame->header.payload_length; b++) {
        username[b] = (char)frame->payload[b];
    }

    username[frame->header.payload_length] = '\0';

    return 0;
}

int cci_parse_send(const CCIFrame *frame, char *destination, size_t destination_capacity, uint8_t *body, size_t body_capacity, size_t *out_body_length)
{
    if (frame == NULL || destination == NULL || body == NULL || out_body_length == NULL || frame->header.type != CCI_SEND) {
        return -1;
    }

    if (frame->header.payload_length < 2) {
        return -1;
    }

    uint8_t dest_length = frame->payload[0];

    if (dest_length == 0) {
        return -1;
    }

    if ((size_t)dest_length + 1 > frame->header.payload_length) {
        return -1;
    }

    if ((size_t)dest_length >= destination_capacity) {
        return -1;
    }

    size_t body_length = frame->header.payload_length - dest_length - 1;

    if (body_length == 0 || body_length > body_capacity) {
        return -1;
    }

    for (size_t b = 0; b < dest_length; b++) {
        destination[b] = (char)frame->payload[b + 1];
    }

    destination[dest_length] = '\0';


    for (size_t b = 0; b < body_length; b++) {
        body[b] = frame->payload[dest_length + b + 1];
    }

    *out_body_length = body_length;

    return 0;
}

int cii_parse_ack(const CCIFrame *frame, uint64_t *out_message_id)
{
    if (frame == NULL || out_message_id == NULL || frame->header.type != CCI_ACK) {
        return -1;
    }

    if (frame->header.payload_length != CCI_HEADER_SIZE) {
        return -1;
    }

    uint64_t id = 0;

    for (size_t b = 0; b < CCI_HEADER_SIZE; b++) {
        id = (id << 8) | frame->payload[b];
    }

    *out_message_id = id;

    return 0;
}

int cci_build_ok(CCIFrame *frame, CCIMessageType operation, uint64_t value)
{
    if (frame == NULL) {
        return -1;
    }

    uint8_t payload[10];

    payload[0] = (uint8_t)(operation >> 8);
    payload[1] = (uint8_t)(operation & 0xFF);

    for (int b = 0; b < CCI_HEADER_SIZE; i++) {
        payload[2 + b] = (uint8_t)(value >> (56 - (b * 8)));
    }

    return cci_build_frame(frame, CCI_OK, payload, sizeof(payload));
}


int cci_build_error(CCIFrame *frame, CCIErrorCode error)
{
    if (frame == NULL) {
        return -1;
    }

    uint8_t payload[2];

    payload[0] = (uint8_t)(error >> 8);
    payload[1] = (uint8_t)(error & 0xFF);

    return cci_build_frame(frame, CCI_ERROR, payload, sizeof(payload));
}

int cci_build_delivery(CCIFrame *frame, uint64_t message_id, const char *sender, const uint8_t *body, size_t body_length)
{
    if (frame == NULL || sender == NULL || body == NULL || body_length == 0) {
        return -1;
    }

    size_t sender_length = strlen(sender);

    if (sender_length == 0 || sender_length > 255) {
        return -1;
    }

    size_t payload_length = CCI_HEADER_SIZE + sender_length + body_length + 1;

    if (payload_length > CCI_MAX_PAYLOAD) {
        return -1;
    }

    uint8_t payload[CCI_MAX_PAYLOAD];

    for (int b = 0; b < 8; b++) {
        payload[b] = (uint8_t)(message_id >> (56 - (b * 8)));
    }

    payload[8] = (uint8_t)sender_length;

    for (size_t b = 0; b < sender_length; b++) {
        payload[9 + b] = (uint8_t)sender[b];
    }

    for (size_t b = 0; b < body_length; b++) {
        payload[9 + sender_length + b] = body[b];
    }

    return cci_build_frame(frame, CCI_DELIVERY, payload, (uint32_t)payload_length);
}