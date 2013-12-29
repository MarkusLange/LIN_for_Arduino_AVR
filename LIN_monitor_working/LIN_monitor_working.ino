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

int frameerrordelay = ((breakfield + breakdelimiter) - uartlenght)*Tbit;
//int frameerrordelay = (breakfield + breakdelimiter + breakfieldinterbytespace) - uartlenght;

#define syncfield                  uartlenght
#define PIDfield                   uartlenght
#define syncfieldPIDinterbytedelay  0
int syncfieldPIDinterbytespace = syncfieldPIDinterbytedelay*Tbit;

//Tbit Response
#define responsedelay               8
int responsespace = responsedelay*Tbit;
#define interbytedelay              0
int interbytespace = interbytedelay*Tbit;

#define numbers  4
byte message[numbers],sending[numbers];
byte linb,sync,PID,checksum;
int n = 0;

void setup() {
  // initialize serial port and LIN:
  Serial.begin(19200);
  Lininit();
  pinMode(13, OUTPUT);
}

void loop() {
  blinking();
  LinReading();
//  LinWriting();
}

void LinReading() {
  LinRead();
  PrintlnLin();
  ClearFrame();
}

void LinWriting() {
  LinWrite(sending);
  Answer();
  delay(1000);
}

void Lininit() {
  Serial1.begin(linspeed);
  pinMode(0,INPUT_PULLUP);
}

void Answer() {
  for (int i=0;i<numbers;i++) {
     sending[i] = n;
  }
  n++;
  n = n % 256;
  Serial.println(n,HEX);
}

void LinRead() {
  if (1<<FE1) {
    delayMicroseconds(frameerrordelay);               //after Frame Error Tbit to Sync Field
    if (Serial1.available() > 0) {
      sync = Serial1.read();
      if (sync != 0x55) {
        sync = 0x00;
      }
      delayMicroseconds(syncfieldPIDinterbytespace);  //Interbyte Space
      if (Serial1.available() > 0) {
        PID = Serial1.read();
      }
      delayMicroseconds(responsespace);               //after PID Tbit space
      for (int i=0;i<numbers;i++) {
        message[i] = Serial1.read();
        if (interbytespace == 0) {                    //Interbyte Space
          delayMicroseconds(1);
        } else {
          delayMicroseconds(interbytespace);
        }
      }
      checksum = Serial1.read();
    }
  } else {
    Serial.println("NO Break");
  }
}

void LinResponse(byte* sending) {
  if (1<<FE1) {
    delayMicroseconds(frameerrordelay);                 //after Frame Error Tbit to Sync Field
    if (Serial1.available() > 0) {
      sync = Serial1.read();
      if (sync == 0x55) {
        delayMicroseconds(syncfieldPIDinterbytespace);  //Interbyte Space
        if (Serial1.available() > 0) {
          PID = Serial1.read();
          if (PID == 0x11) {
            delayMicroseconds(responsespace);           //after PID Tbit space
            for (int i=0;i<numbers;i++) {
              Serial1.write(sending[i]);
              if (interbytespace == 0) {
                delayMicroseconds(1);
              } else {
                delayMicroseconds(interbytespace);
              }
              Serial1.write(MessageCRC(message,0));
            }
          }
        }
      }
    }
  }
}

void LinWrite(byte* sending) {
  Serial1.end();
  pinMode(0,OUTPUT);
  digitalWrite(1, LOW);
  delayMicroseconds(breakfield*Tbit);                 //after Frame Error Tbit to Sync Field
  digitalWrite(1, HIGH);
  delayMicroseconds(breakdelimiter*Tbit);
  Serial1.begin(linspeed);
  Serial1.write(0x55);
  Serial1.write(0x03);
  delayMicroseconds(responsespace);
  for (int i=0;i<numbers;i++) {
    Serial1.write(sending[i]);
    if (interbytespace == 0) {
      delayMicroseconds(1);
    } else {
      delayMicroseconds(interbytespace);
    }
  }
  Serial1.write(MessageCRC(sending,0));
}
  
void PrintlnLin() {
  if(sync != 0) {
//    Serial.print("Break Field   Sync:  ");
//    Serial.print(sync,HEX);
    Serial.print("PID:  ");
    Serial.print(PID & 0x3F,HEX);            //PID without CRC
    Serial.print("   CRC:  ");
    Serial.print(PIDCRC(PID));
    Serial.print("  Message:  ");
    for (int i=0;i<numbers;i++) {
      Serial.print(message[i],HEX);
      Serial.print(";");
    }
    Serial.print("  Checksum:  ");
    Serial.print(checksum,HEX);
    Serial.print("  ChecksumCRC:  ");
    Serial.println(MessageCRC(message,0),HEX);
  }
}

void ClearFrame() {
  sync = 0;
  PID = 0;
  for (int i=0;i<numbers;i++) {
    message[i] = 0;
  }
  checksum = 0;
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

int PIDCRC(int PID) {
  int P0 = ((PID>>0) + (PID>>1) + (PID>>2) + (PID>>4)) & 1;
  int P1 = ~((PID>>1) + (PID>>3) + (PID>>4) + (PID>>5)) & 1;
  return (P0 | (P1<<1));
}

byte MessageCRC(byte* message, uint16_t sum) {
  for (int i=0;i<numbers;i++) {
    sum += message[i];
  }
  while(sum>>8)  // In case adding the carry causes another carry
    sum = (sum&255)+(sum>>8); 
  return (~sum);
}
