#define LED 13
#define NTC A0
#define LDR A1
#define MIC A2
#define LITTLEBIT A3

#define NTC_EN  12
#define LDR_EN  11
#define MIC_EN  10
#define LITTLEBIT_EN  9

#define NTCNOMINAL 10000      
#define NTCNOMINALT 25   
#define NTCB 4220

#define TXINTERVAL      60000
#define SAMPLEINTERVAL  100
unsigned long txStartTime;
unsigned long sampleStartTime;
boolean firstLoop = true;

float VPerBit = 5.0 / 1024.0;
float OneOverVinR2 = 1.0 / (5.0 * 10000.0);
float fcdLuxConv =  1.0 / 0.0929;
int OhmPerFcd = 165;

int getLux(int adIn) {
  float Rldr = ((5.0 * 10000.0) / (adIn * VPerBit)) - 10000;
  /* crude approximation for 'standard' LDRs */
  return (int)(500000.0 / Rldr);
}

float getDegC(int adIn) {
  float Rntc = ((5.0 * 10000.0) / (adIn * VPerBit)) - 10000;

  /* borrowed/stolen from adafruit */
  float steinhart;
  steinhart = Rntc / NTCNOMINAL;               // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= NTCB;                           // 1/B * ln(R/Ro)
  steinhart += 1.0 / (NTCNOMINALT + 273.15);   // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
  return steinhart;
}

float fwdAvgFloat(float old, float val, float fract) {
  return (old * (1-fract)) + (val * fract);
}

int fwdAvgInt(int old, int val, float fract) {
  return (int)round(fwdAvgFloat((float)old, (float)val, fract));
}

void blink (unsigned int onDelay, unsigned int offDelay) {
  digitalWrite(LED, HIGH);
  delay(onDelay);
  digitalWrite(LED, LOW);
  delay(offDelay);
}

void blinkRepeated(unsigned int repeat, unsigned int onDelay, unsigned int offDelay) {
  if (repeat > 0) {
    for (; repeat > 0; repeat--) {
      blink(onDelay, offDelay);
    }
  } else {
    for (;;) 
      blink(onDelay, offDelay);
  }
}

int readLine(char *bfr, int len) {
  int i = 0;
  int retval = 0;
  
  while (1) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\r')
        continue;
      if (c == '\n') {
        break;
      }
      
      bfr[i++] = c;
      
      if (i == len - 1) {
        retval = i-1;
        break;
      }
    } else {
      blink(50, 50);
    }
  }
   
  bfr[i] = '\0';
}

#define BUFSIZE 32

void init_lora() {
  char bfr[BUFSIZE];
  int accepted = 0;
  
  do {
    blink(50, 250);
  } while (Serial.available() <= 0);

  readLine(bfr, BUFSIZE);
  blinkRepeated(3, 100, 100);
  
  Serial.print("mac join abp\r\n");
  readLine(bfr, BUFSIZE);
  
  if (!strncmp(bfr, "ok", 2)) {    
    readLine(bfr, BUFSIZE);
    
    if (!strncmp(bfr, "accepted", 8)) {
      blinkRepeated(3, 100, 250);
      digitalWrite(LED, HIGH);
      accepted = 1;
    }
  }
  
  /* if not accepted blink forever */
  if (!accepted) {
    blinkRepeated(0, 50, 50);
  }
  
  delay(1000);
}

void setup() {
  Serial.begin(57600);

  pinMode(NTC_EN, INPUT);
  pinMode(LDR_EN, INPUT);
  pinMode(MIC_EN, INPUT);
  pinMode(LITTLEBIT_EN, INPUT);
  pinMode(LED, OUTPUT);


  delay(1000);
  Serial.print("mac join abp\r\n");
  delay(5000);

/* FIXME: make init work */
//  init_lora();

  txStartTime = millis();
  sampleStartTime = millis();
}

int cntr = 0;

void loop() {
  char bfr[64];
  float ntc;
  uint16_t lux;
  uint16_t sound;
  uint16_t littlebit;
  unsigned long currentTime = millis();

  if (firstLoop) {
    ntc = getDegC(analogRead(NTC));
    lux = getLux(analogRead(LDR));
    sound = analogRead(MIC);
    littlebit = analogRead(LITTLEBIT);
  }

  if (currentTime >= sampleStartTime + SAMPLEINTERVAL) {
    blinkRepeated(2, 50, 100);
    ntc = fwdAvgFloat(ntc, getDegC(analogRead(NTC)), 0.9);
    lux = fwdAvgInt(lux, getLux(analogRead(LDR)), 0.9);
    sound = fwdAvgInt(sound, analogRead(MIC), 0.5);
    littlebit = fwdAvgInt(littlebit, analogRead(LITTLEBIT), 0.9);
    sampleStartTime = millis();
  }
  
  if (currentTime >= txStartTime + TXINTERVAL) {
    float sendTemp = 99.99;
    uint16_t sendLux = 9999, sendSound = 9999, sendLittleBit = 9999;

    if (!digitalRead(NTC_EN)) sendTemp = ntc;
    if (!digitalRead(LDR_EN)) sendLux = lux;
    if (!digitalRead(MIC_EN)) sendSound = sound;
    if (!digitalRead(LITTLEBIT_EN)) sendLittleBit = littlebit;
    
    int port = random(1, 223);    
    sprintf(bfr, "mac tx cnf %d %.2u%.2u%.4d%.4d%.4d\r\n\0", port, 
                                                             (unsigned char)floor(sendTemp), 
                                                             (unsigned char)ceil((sendTemp - floor(sendTemp)) * 100),
                                                             sendLux, 
                                                             sendSound, 
                                                             sendLittleBit);
    Serial.print(bfr);
    
    /*
    readLine(bfr, 64);
    if (!strncmp(bfr, "ok", 2)) {
      readLine(bfr, 64);
      if (!strncmp(bfr, "mac_tx_ok", 9)) {
        blink(250, 250);
      } else {
        blinkRepeated(3, 250, 250);
      }
    }
    */

    txStartTime = millis();
  }
}
