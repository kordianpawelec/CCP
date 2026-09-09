#ifndef CCI_PROTOCOL_H
#define CCI_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#define CCI_MAGIC        0xCC
#define CCI_VERSION      1
#define CCI_HEADER_SIZE  8
#define CCI_MAX_PAYLOAD  2048

typedef enum {
    CCI_REGISTER = 1,
    CCI_SEND     = 2,
    CCI_FETCH    = 3,
    CCI_ACK      = 4,
    CCI_OK       = 100,
    CCI_DELIVERY = 101,
    CCI_ERROR    = 102
} CCIMessageType;

typedef struct {
    uint8_t magic;
    uint8_t version;
    uint16_t type;
    uint32_t payload_length;
} CCIHeader;

typedef struct {
    CCIHeader header;
    uint8_t payload[CCI_MAX_PAYLOAD];
} CCIFrame;


typedef enum {
    CCI_ERROR_INVALID_FRAME = 1,
    CCI_ERROR_INVALID_PAYLOAD,
    CCI_ERROR_UNSUPPORTED_VERSION,

    CCI_ERROR_CLIENT_EXISTS,
    CCI_ERROR_CLIENT_NOT_FOUND,
    CCI_ERROR_CLIENT_LIMIT_REACHED,

    CCI_ERROR_MAILBOX_FULL,
    CCI_ERROR_MAILBOX_EMPTY,

    CCI_ERROR_INVALID_ACK,

    CCI_ERROR_INTERNAL,

    CCI_ERROR_MESSAGE_TOO_LONG,
    CCI_ERROR_MESSAGE_AWAITING_ACK,
} CCIErrorCode;


int cci_build_frame(CCIFrame *frame, CCIMessageType type, const uint8_t *payload, uint32_t payload_length);

int cci_parse_register(const CCIFrame *frame, char *username, size_t username_capacity);

int cci_parse_send(const CCIFrame *frame, char *destination, size_t destination_capacity, uint8_t *body, size_t body_capacity, size_t *out_body_length);

int cci_parse_ack(const CCIFrame *frame, uint64_t *out_message_id);

int cci_build_ok(CCIFrame *frame, CCIMessageType operation, uint64_t value);

int cci_build_error(CCIFrame *frame, CCIErrorCode error);

int cci_build_delivery(CCIFrame *frame, uint64_t message_id, const char *sender, const uint8_t *body, size_t body_length);



int cci_read_frame(int socket_fd, CCIFrame *out_frame);

int cci_write_frame(int socket_fd, const CCIFrame *frame);

#endif
