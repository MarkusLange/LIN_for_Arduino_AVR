int ledState = LOW;             // ledState used to set the LED
long previousMillis = 0;        // will store last time LED was updated

long interval = 250;

//Lin Initailisation
#define linspeed                19200
unsigned long Tbit = 1000000/linspeed;

#define uartlenght                 10

//Tbits Header
#define breakfield                 13
#define breakdelimiter              1
#define breakfieldinterbytespace    2

int frameerrordelay = (breakfield + breakdelimiter) - uartlenght;
//int frameerrordelay = (breakfield + breakdelimiter + breakfieldinterbytespace) - uartlenght;

#define syncfield                  uartlenght
#define syncfieldPIDinterbytespace  0
#define PIDfield                   uartlenght

//Tbit Response
#define responsespace               8
#define interbytespace              0

void setup() {
  // initialize both serial port and LIN:
  Serial.begin(19200);
  Lininit();
  pinMode(13, OUTPUT);
}

void loop() {
//  Serial1.write(0x55);
//  Serial1.write(0xFF);
//  Serial1.write(0b01111110);
  LinBreak();
  blinking();
}

void Lininit() {
  Serial1.begin(linspeed);
  pinMode(0,INPUT_PULLUP);
}

int numbers = 4;
byte message[4];
byte linb,sync,PID,checksum;

void LinBreak() {
  if (1<<FE1) {
    delayMicroseconds(6*52);      //after Frame Error Tbit to Sync Field
    if (Serial1.available() > 0) {
      sync = Serial1.read();
      if (sync != 0x55) {
        sync = 0x00;
      }
      if (Serial1.available() > 0) {
        PID = Serial1.read();
      }
      delayMicroseconds(6*52);    //after PID Tbit space
      for (int i=0;i<numbers;i++) {
//        delayMicroseconds(0);
        message[i] = Serial1.read();
      }
//      delayMicroseconds(0);
      checksum = Serial1.read();
    }
  } else {
    Serial.println("NO Break");
  }
  if(sync != 0) {
    Serial.print("Break Field   Sync:  ");
    Serial.print(sync,HEX);
    Serial.print("   PID:  ");
    //PID without CRC
    Serial.print(PID & 0x3F,HEX);
    PIDCRC();
    Serial.print("  Message:  ");
    for (int i=0;i<numbers;i++) {
      Serial.print(message[i],HEX);
      Serial.print(";");
    }
    Serial.print("  Checksum:  ");
    Serial.print(checksum,HEX);
    MessageCRC();
    sync = 0;
    PID = 0;
    for (int i=0;i<numbers;i++) {
      message[i] = 0;
    }
    checksum = 0;
    Serial.println();
  }
}

void blinking() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;
    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;
    // set the LED with the ledState of the variable:
    digitalWrite(13, ledState);
  }
}

void PIDCRC() {
  int CRC = 0;
  int P0 = ((PID>>0) + (PID>>1) + (PID>>2) + (PID>>4)) & 1;
  int P1 = ~((PID>>1) + (PID>>3) + (PID>>4) + (PID>>5)) & 1;
  CRC = P0 | (P1<<1);
  if ((PID>>6) == CRC) {
    Serial.print("  CRC OK");
  } else {
    Serial.print("  CRC failed");
  }
}

void MessageCRC() {
  byte messageCRC = 0;
  for (int i=0;i<numbers;i++) {
    messageCRC += message[i];
  }
  messageCRC = ~messageCRC;
  if ((messageCRC -1) == checksum) {
    Serial.print("  CRC OK  ");
  } else {
    Serial.print("  CRC failed  ");
  }
}
