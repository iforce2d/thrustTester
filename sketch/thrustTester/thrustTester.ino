/*
Motor speed controller
ESC <----> Arduino
Signal     Pin 5

Load cell amplifier (https://learn.sparkfun.com/tutorials/load-cell-amplifier-hx711-breakout-hookup-guide)
HX711 <----> Arduino
GND          GND
VCC          3.3v
DT           Pin 3
SCK          Pin 4

ADC (http://henrysbench.capnfatz.com/henrys-bench/arduino-voltage-measurements/arduino-ads1115-module-getting-started-tutorial/)
ADS1115 <----> Arduino
GND            GND
VDD            5v
SDA            Pin A4
SCL            Pin A5
A0             GND
A1             Middle of 10:1 voltage divider (I used 10k and 1k, smaller resistor goes on ground side)
A2             GND
A3             OUT pin of ACS712

Hall sensor (http://embedded-lab.com/blog/a-brief-overview-of-allegro-acs712-current-sensor-part-1/)
ACS712 <----> Arduino
GND           GND
VDD           5v
OUT           A3 of ADS1115
(sense terminals should bridge a break in the main power line)

Laser TX
GND          GND
VCC (S)      5v

Laser RX
GND          GND
VCC          5v
OUT          Pin 2

Buzzer (continuous 5v buzzer)
GND          GND
VCC          Pin 7


*/

#include <Wire.h>
#include <Servo.h>
#include "HX711.h"
#include <Adafruit_ADS1015.h>

Servo servo;
HX711 cell(3, 4);
Adafruit_ADS1115 ads;

const unsigned char messageSync[] = { 255, 254 }; // used to mark the start of a message
const unsigned char messageVersion = 1;

void calcChecksum(unsigned char* CK, void* data, int size) {
  // Same as UBX protocol
  memset(CK, 0, 2);
  for (int i = 0; i < size; i++) {
    CK[0] += ((unsigned char*)data)[i];
    CK[1] += CK[0];
  }
}

enum _messageType {

  MT_NONE,

  // these two must never change
  MT_REQUEST_VERSION  = 1,
  MT_VERSION          = 2,

  // incoming
  MT_SET_THROTTLE,
  MT_START_BEEP,
  MT_STOP_BEEP,
  MT_START_SAMPLING,
  MT_STOP_SAMPLING,

  // outgoing
  MT_SAMPLE
};


// Messages that the arduino can send to the computer

struct msg_version {
  uint8_t version;
};

struct msg_setThrottle {
  uint16_t throttle;
};

struct msg_sample {
  uint16_t throttle;
  int32_t thrust;
  int16_t voltage;
  int16_t current;
  uint16_t rpmcount;
};

struct _message {
  unsigned char type;
  union {
    msg_version version;
    msg_setThrottle setThrottle;
    msg_sample sample;
  };
};

_message outgoingMessage;
int outgoingPayloadSize = 0;

_message incomingMessage; // should really use a separate type for this, but so far all incoming messages will be smaller than _message so it's safe
int incomingPayloadSize = 0;

boolean sampling = false;
unsigned short throttle = 0;
int sampleInterval = 0; // ms

byte rpmcount = 0;

void beepTimes(int howManyTimes) {
  // note that sending samples to the computer will stop while this is going on
  for (int i = 0; i < howManyTimes; i++) {
    digitalWrite(7, HIGH);
    delay(10);
    digitalWrite(7, LOW);
    delay(40);
  }
}


void setup() {
  Serial.begin(9600);
  pinMode(7, OUTPUT); // buzzer pin
  
  servo.attach(5); // servo signal pin
  servo.writeMicroseconds(1000);
  
  ads.setGain(GAIN_ONE);
  ads.begin();
  
  attachInterrupt(0, rpmCounterInterrupt, FALLING);
  
  beepTimes(2);
}


void rpmCounterInterrupt()
{
  rpmcount++;
}


void loop() {
  static unsigned char checksum[2];
  static unsigned long lastSampleTime = 0;
  unsigned long now;

  boolean sendVersion = false;
  while ( Serial.available() ) {
    int msgType = processSerialByte(Serial.read());
    if ( msgType == MT_REQUEST_VERSION ) {
      sendVersion = true;
    }
    else if ( msgType == MT_SET_THROTTLE ) {
      throttle = incomingMessage.setThrottle.throttle;
      servo.writeMicroseconds(throttle);
    }
    else if ( msgType == MT_START_BEEP ) {
      digitalWrite(7, HIGH);
    }
    else if ( msgType == MT_STOP_BEEP ) {
      digitalWrite(7, LOW);
    }
    else if ( msgType == MT_START_SAMPLING ) {
      rpmcount = 0;
      sampling = true;
    }
    else if ( msgType == MT_STOP_SAMPLING ) {
      sampling = false;
    }
  }

  if ( sendVersion )
  {
    outgoingMessage.type = MT_VERSION;
    outgoingMessage.version.version = messageVersion;
    outgoingPayloadSize = sizeof(msg_version) + 1;
  }
  else if ( sampling && (now = millis()) - lastSampleTime > sampleInterval ) {
    outgoingMessage.type = MT_SAMPLE;
    outgoingMessage.sample.throttle = throttle;
    outgoingMessage.sample.thrust = cell.read();
    outgoingMessage.sample.voltage = ads.readADC_Differential_0_1();
    outgoingMessage.sample.current = ads.readADC_Differential_2_3();
    outgoingMessage.sample.rpmcount = rpmcount;
    rpmcount = 0;
    outgoingPayloadSize = sizeof(msg_sample) + 1;
    lastSampleTime = now;    
  }
  else {
    return;
  }

  calcChecksum(checksum, &outgoingMessage, outgoingPayloadSize);

  Serial.write( messageSync[0] );
  Serial.write( messageSync[1] );

  for (int i = 0; i < outgoingPayloadSize; i++)
    Serial.write(((unsigned char*)&outgoingMessage)[i]);

  Serial.write(checksum[0]);
  Serial.write(checksum[1]);

}

int processSerialByte(unsigned char c) {
  static int fpos = 0;
  static unsigned char checksum[2];

  static _messageType currentMessageType = MT_NONE;

  //Serial.print(fpos);
  //Serial.print(" ");
  //Serial.println(c);
  //Serial.write(fpos);
  //Serial.write(c);

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

    // Place the incoming byte into the incomingMessage struct. The position is fpos-2 because
    // the struct does not include the initial two-byte header (messageSync).
    if ( (fpos-2) < sizeof(_message) )
      ((unsigned char*)(&incomingMessage))[fpos-2] = c;

    fpos++;

    if ( fpos == 3 ) {
      unsigned char* ptr = (unsigned char*)(&incomingMessage);
      // We have just received the second byte of the message type header,
      // so now we can check to see what kind of message it is.
      if ( *ptr == MT_REQUEST_VERSION ) {
        currentMessageType = MT_REQUEST_VERSION;
        incomingPayloadSize = 1;
      }
      else if ( *ptr == MT_SET_THROTTLE ) {
        currentMessageType = MT_SET_THROTTLE;
        incomingPayloadSize = sizeof(msg_setThrottle) + 1;
      }
      else if ( *ptr == MT_START_BEEP ) {
        currentMessageType = MT_START_BEEP;
        incomingPayloadSize = 1;
      }
      else if ( *ptr == MT_STOP_BEEP ) {
        currentMessageType = MT_STOP_BEEP;
        incomingPayloadSize = 1;
      }
      else if ( *ptr == MT_START_SAMPLING ) {
        currentMessageType = MT_START_SAMPLING;
        incomingPayloadSize = 1;
      }
      else if ( *ptr == MT_STOP_SAMPLING ) {
        currentMessageType = MT_STOP_SAMPLING;
        incomingPayloadSize = 1;
      }
      else {
        // unexpected message type, bail
        fpos = 0;
        return MT_NONE;
      }
    }

    if ( fpos == (incomingPayloadSize+2) ) {
      // All payload bytes have now been received, so we can calculate the
      // expected checksum value to compare with the next two incoming bytes.
      calcChecksum(checksum, &incomingMessage, incomingPayloadSize);
    }
    else if ( fpos == (incomingPayloadSize+3) ) {
      // First byte after the payload, ie. first byte of the checksum.
      // Does it match the first byte of the checksum we calculated?
      if ( c != checksum[0] ) {
        // Checksum doesn't match, reset to beginning state and try again.
        fpos = 0;
      }
    }
    else if ( fpos == (incomingPayloadSize+4) ) {
      // Second byte after the payload, ie. second byte of the checksum.
      // Does it match the second byte of the checksum we calculated?
      fpos = 0; // We will reset the state regardless of whether the checksum matches.
      if ( c == checksum[1] ) {
        // Checksum matches, we have a valid message.
        return currentMessageType;
      }
    }
    else if ( fpos > (incomingPayloadSize+4) ) {
      // We have now read more bytes than both the expected payload and checksum
      // together, so something went wrong. Reset to beginning state and try again.
      fpos = 0;
    }
  }

  return MT_NONE;
}



