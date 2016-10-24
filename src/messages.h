#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>

const unsigned char messageSync[] = { 255, 254 };
const unsigned char messageVersion = 1;

enum _messageType {

    MT_NONE,

    // these two must never change
    MT_REQUEST_VERSION  = 1,
    MT_VERSION          = 2,

    // outgoing
    MT_SET_THROTTLE,
    MT_START_BEEP,
    MT_STOP_BEEP,
    MT_START_SAMPLING,
    MT_STOP_SAMPLING,

    // incoming
    MT_SAMPLE
};

struct msg_version {
    uint8_t version;
} __attribute__((packed));

struct msg_setThrottle {
    uint16_t throttle;
} __attribute__((packed));

struct msg_sample {
    uint16_t throttle;
    int32_t thrust;
    int16_t voltage;
    int16_t current;
    uint16_t rpmcount;
} __attribute__((packed));

struct _message {
    unsigned char type;
    union {
        msg_version version;
        msg_setThrottle setThrottle;
        msg_sample sample;
    };
} __attribute__((packed));

void calcChecksum(unsigned char* CK, void* data, int size);
int processSerialByte(unsigned char c);

extern _message g_message;

#endif // MESSAGES_H
