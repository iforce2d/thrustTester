#include <memory.h>
#include "messages.h"

_message g_message;

void calcChecksum(unsigned char* CK, void* data, int size) {
    memset(CK, 0, 2);
    for (int i = 0; i < size; i++) {
        CK[0] += ((unsigned char*)data)[i];
        CK[1] += CK[0];
    }
}

bool compareMsgHeader(const unsigned char* msgHeader) {
    unsigned char* ptr = (unsigned char*)(&g_message);
    return ptr[0] == msgHeader[0] && ptr[1] == msgHeader[1];
}

int processSerialByte(unsigned char c) {
    static int fpos = 0;
    static unsigned char checksum[2];

    static _messageType currentMessageType = MT_NONE;
    static int payloadSize = sizeof(_message);

    if ( fpos < 2 ) {
        // For the first two bytes we are simply looking for a match with the messageSync bytes
        if ( c == messageSync[fpos] )
            fpos++;
        else
            fpos = 0; // Reset to beginning state.
    }
    else {
        // If we come here then fpos >= 2, which means we have found a match with the messageSync
        // and we are now reading in the bytes that make up the payload.

        // Place the incoming byte into the g_message struct. The position is fpos-2 because
        // the struct does not include the initial two-byte header (messageSync).
        if ( (fpos-2) < (int)sizeof(g_message) )
            ((unsigned char*)(&g_message))[fpos-2] = c;

        fpos++;

        if ( fpos == 3 ) {
            unsigned char* ptr = (unsigned char*)(&g_message);
            // We have just received the second byte of the message type header,
            // so now we can check to see what kind of message it is.
            if ( *ptr == MT_VERSION ) {
                currentMessageType = MT_VERSION;
                payloadSize = sizeof(msg_version) + 1;
            }
            else if ( *ptr == MT_SAMPLE ) {
                currentMessageType = MT_SAMPLE;
                payloadSize = sizeof(msg_sample) + 1;
            }
            else {
                // unexpected message type, bail
                fpos = 0;
                return MT_NONE;
            }
        }

        if ( fpos == (payloadSize+2) ) {
            // All payload bytes have now been received, so we can calculate the
            // expected checksum value to compare with the next two incoming bytes.
            calcChecksum(checksum, &g_message, payloadSize);
        }
        else if ( fpos == (payloadSize+3) ) {
            // First byte after the payload, ie. first byte of the checksum.
            // Does it match the first byte of the checksum we calculated?
            if ( c != checksum[0] ) {
                // Checksum doesn't match, reset to beginning state and try again.
                fpos = 0;
            }
        }
        else if ( fpos == (payloadSize+4) ) {
            // Second byte after the payload, ie. second byte of the checksum.
            // Does it match the second byte of the checksum we calculated?
            fpos = 0; // We will reset the state regardless of whether the checksum matches.
            if ( c == checksum[1] ) {
                // Checksum matches, we have a valid message.
                return currentMessageType;
            }
        }
        else if ( fpos > (payloadSize+4) ) {
            // We have now read more bytes than both the expected payload and checksum
            // together, so something went wrong. Reset to beginning state and try again.
            fpos = 0;
        }
    }

    return MT_NONE;
}


