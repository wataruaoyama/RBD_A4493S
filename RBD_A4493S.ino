/*
   Copyright (c) 2022/05/06 あおやまわたる

   以下に定める条件に従い，本ソフトウェアおよび関連文書のファイル（以下
   「ソフトウェア」）の複製を取得するすべての人に対し，ソフトウェアを無制
   限に扱うことを無償で許可します．これには，ソフトウェアの複製を使用，複
   写，変更，結合，掲載，頒布，サブライセンス，および/または販売する権利，
   およびソフトウェアを提供する相手に同じことを許可する権利も無制限に含ま
   れます．

   上記の著作権表示および本許諾表示を，ソフトウェアのすべての複製または重
   要な部分に記載するものとします．

   ソフトウェアは「現状のまま」で，明示であるか暗黙であるかを問わず，何ら
   の保証もなく提供されます．ここでいう保証とは，商品性，特定の目的への適
   合性，および権利非侵害についての保証も含みますが，それに限定されるもの
   ではありません．作者または著作権者は，契約行為，不法行為，またはそれ以
   外であろうと，ソフトウェアに起因または関連し，あるいはソフトウェアの使
   用またはその他の扱いによって生じる一切の請求，損害，その他の義務につい
   て何らの責任を負わないものとします．
*/
#include <Wire.h>
#include <avr/pgmspace.h>
#include <U8g2lib.h>
#include <FreqCount.h>

#define AK4493S_ADR 0x10
#define Control1 0x00
#define Control2 0x01
#define Control3 0x02
#define LchATT 0x03
#define RchATT 0x04
#define Control4 0x05
#define Dsd1 0x06
#define Control5 0x07
#define SoundControl 0x08
#define Dsd2 0x09
#define Control6 0x0A
#define Control7 0x0B
#define Control8 0x15

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, /*SCL, SDA,*/ /* reset=*/ U8X8_PIN_NONE);

const char fs32[] PROGMEM = "32";
const char fs44[] PROGMEM = "44.1";
const char fs48[] PROGMEM = "48";
const char fs88[] PROGMEM = "88.2";
const char fs96[] PROGMEM = "96";
const char fs176[] PROGMEM = "176.4";
const char fs192[] PROGMEM = "192";
const char fs352[] PROGMEM = "352.8";
const char fs384[] PROGMEM = "384";
const char sp[] PROGMEM = "Sharp Roll-Off";
const char sw[] PROGMEM = "Slow Roll-Off";
const char sdsp[] PROGMEM = "Short Delay Sharp";
const char sdsw[] PROGMEM = "Short Delay Slow";
const char ssw[] PROGMEM = "Super Slow";
const char ldn[] PROGMEM = "Low Dispersion";
const char bname[] PROGMEM = "RBD-A4493S";
const char dsn[] PROGMEM = "Designed by";
const char logo[] PROGMEM = "LINUXCOM";
const char no_signal[] PROGMEM = "No Signal";

uint8_t SW[] = {14, 15, 16};
uint8_t LED[] = {7, 8, 13};
uint8_t SW3 = 2;
uint8_t RPI_OK = 9;
uint8_t PDN = 17;
uint8_t receiver = 6;
boolean go = LOW;

uint8_t filter;

void setup() {
  int i;
  for (i=0; i<=2; i++) {
    pinMode(LED[i], OUTPUT);
    pinMode(SW[i], INPUT_PULLUP);
  }
  pinMode(SW3, INPUT_PULLUP);
  pinMode(receiver, INPUT);
  pinMode(RPI_OK, INPUT);
  pinMode(PDN, OUTPUT);
  pinMode(LED, OUTPUT);

//  Serial.begin(9600);
  FreqCount.begin(10);  // 周波数測定の開始．測定間隔を10ミリ秒に設定
  Wire.begin();
  Wire.setClock(400000);
  u8g2.begin();
  u8g2.clearBuffer();
  
//  while(go == LOW) {
//    go = digitalRead(RPI_OK);
//  }

  delay(500);
  digitalWrite(PDN, HIGH);
  initAK4493S();
  initOledDisplay();
  delay(4000);
  //Serial.print("SW Status = "); Serial.println(readDipSwitch());
  //Serial.print("SW3 = "); Serial.println(digitalRead(SW3));
  initDigitalFilter(readDipSwitch());
  //Serial.print("filter = "); Serial.println(filter);
}

void loop() {
  uint16_t FSR = freqCounter();
  messageOut(FSR, filter);
  monitorLED(FSR, filter);
  //delay(300);
}

uint8_t i2cReadRegister(uint8_t sladr, uint8_t regadr) {
  Wire.beginTransmission(sladr);
  Wire.write(regadr);
  Wire.endTransmission();
  Wire.requestFrom(sladr, 1);
  return Wire.read();
}

void i2cWriteRegister(uint8_t sladr, uint8_t regadr, uint8_t wdata) {
  Wire.beginTransmission(sladr);
  Wire.write(regadr);
  Wire.write(wdata);
  Wire.endTransmission();
}

uint8_t readDipSwitch() {
  uint8_t switchStatus;
  int i;
  for (i=0; i<=2; i++) {
    switchStatus = switchStatus << 1 | digitalRead(SW[i]);
  }
  return(switchStatus);
}

uint16_t freqCounter() {
  if (FreqCount.available()) {
    uint16_t fs;
    unsigned long count = 100 * FreqCount.read(); // 測定間隔を10ミリ秒としたので100倍にする
    if ( (count >= 31000) && (count <= 33000) ) fs = 32;
    else if ( (count >= 43100) && (count <= 45200) ) fs = 44;
    else if ( (count >= 47000) && (count <= 49000)) fs = 48;
    else if ( (count >= 87200) && (count <= 89200)) fs = 88;
    else if ( (count >= 95000) && (count <= 97000)) fs = 96;
    else if ( (count >= 175400) && (count <= 177400) ) fs = 176;
    else if ( (count >= 191000) && (count <= 193000) ) fs = 192;
    else if ( (count >= 351800) && (count <= 353800) ) fs = 352;
    else if ( (count >= 383000) && (count <= 385000)) fs = 384;
    else if ( (count >= 767000) && (count <= 769000)) fs = 768;
    //Serial.println(count);
    return(fs);
  }  
}

/* OLEDの初期化 */
void initOledDisplay() {
  uint8_t x, y;
  char message_buffer[20];
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB12_tr);
  strcpy_P(message_buffer, bname);  
  x = u8g2.getStrWidth(message_buffer);
  y = u8g2.getFontAscent();
  u8g2.drawStr(64-(x/2),y,message_buffer);
  u8g2.setFont(u8g2_font_helvR10_tr);
  strcpy_P(message_buffer, dsn);    
  x = u8g2.getStrWidth(message_buffer);
  y = u8g2.getFontAscent();
  u8g2.drawStr(64-(x/2),32+(y/2),message_buffer);
  u8g2.setFont(u8g2_font_helvB12_tr);
  strcpy_P(message_buffer, logo);
  x = u8g2.getStrWidth(message_buffer);
  u8g2.drawStr(64-(x/2),63,message_buffer);
  u8g2.sendBuffer();
}

void messageOut(uint16_t FSR, uint8_t filter) {
  u8g2.clearBuffer();
  displayOledFSR(FSR);
  displayFilter(filter);
  u8g2.sendBuffer();
}

void displayOledFSR(uint16_t FSR) {
  uint8_t x,y;
  char message_buffer[20];
  //u8g2.setFont(u8g2_font_helvR24_tr);
  //u8g2.setFont(u8g2_font_fub30_tn);
  u8g2.setFont(u8g2_font_fur30_tr);
  //u8g2.setFont(u8g2_font_freedoomr25_tn);
  if (FSR == 32) strcpy_P(message_buffer, fs32);
  else if (FSR == 44) strcpy_P(message_buffer, fs44);
  else if (FSR == 48) strcpy_P(message_buffer, fs48);
  else if (FSR == 88) strcpy_P(message_buffer, fs88);
  else if (FSR == 96) strcpy_P(message_buffer, fs96);
  else if (FSR == 176) strcpy_P(message_buffer, fs176);
  else if (FSR == 192) strcpy_P(message_buffer, fs192);
  else if (FSR == 352) strcpy_P(message_buffer, fs352);
  else if (FSR == 384) strcpy_P(message_buffer, fs384);
  else strcpy_P(message_buffer, no_signal);
  x = u8g2.getStrWidth(message_buffer);
  y = u8g2.getFontAscent();
  u8g2.drawStr(64-(x/2), 32+((y/2)-8), message_buffer);
}

void displayFilter(uint8_t filter) {
  uint8_t x,y;
  char message_buffer[20];
  u8g2.setFont(u8g2_font_helvR08_tr);
  if (filter == 1) strcpy_P(message_buffer, sp);
  else if (filter == 2) strcpy_P(message_buffer, sw);
  else if (filter == 3) strcpy_P(message_buffer, sdsp);
  else if (filter == 4) strcpy_P(message_buffer, sdsw);
  else if (filter == 5) strcpy_P(message_buffer, ssw);
  else if (filter == 6) strcpy_P(message_buffer, ldn);
  x = u8g2.getStrWidth(message_buffer);
  y = u8g2.getFontAscent();
  u8g2.drawStr(64-(x/2), 60, message_buffer);
}

void initAK4493S() {
  /**********************
   * AK4493SEQ 初期設定
   **********************/
  Wire.beginTransmission(AK4493S_ADR);
  Wire.write(0x00); //
  Wire.write(0x8F); // Control1
  Wire.write(0x22); // Control2
  Wire.write(0x00); // Control3
  Wire.write(0xFF); // LchATT
  Wire.write(0xFF); // RchATT
  Wire.write(0x00); // Control4
  Wire.write(0x00); // Dsd1
  Wire.write(0x01); // Control5
  Wire.write(0x00); // SoundControl
  Wire.write(0x00); // Dsd2
  Wire.write(0x04); // Control6
  Wire.write(0x00); // Control7
  Wire.write(0x08); // Control8
  Wire.endTransmission();
}

void initDigitalFilter(uint8_t switchState) {
  /* シャープロールオフ */
  if (switchState == 0) {
    i2cWriteRegister(AK4493S_ADR, Control3, 0x00);  // SLOWビットを0
    i2cWriteRegister(AK4493S_ADR, Control2, 0x02);  // SDビットを0
    i2cWriteRegister(AK4493S_ADR, Control4, 0x00);  // SSLOWビットを0
    filter = 1;
  }
  /* スローローフオフ */
  else if (switchState == 1) {
    i2cWriteRegister(AK4493S_ADR, Control3, 0x01);  // SLOWビットを1
    i2cWriteRegister(AK4493S_ADR, Control2, 0x02);  // SDビットを0
    i2cWriteRegister(AK4493S_ADR, Control4, 0x00);  // SSLOWビットを0
    filter = 2;
  }
  /* ショートディレイ・シャープロールオフ */
  else if (switchState == 2) {
    i2cWriteRegister(AK4493S_ADR, Control3, 0x00);  // SLOWビットを0
    i2cWriteRegister(AK4493S_ADR, Control2, 0x22);  // SDビットを1
    i2cWriteRegister(AK4493S_ADR, Control4, 0x00);  // SSLOWビットを0
    filter = 3;
  }
  /* ショートディレイ・スローロールオフ */
  else if (switchState == 3) {
    i2cWriteRegister(AK4493S_ADR, Control3, 0x01);  // SLOWビットを1
    i2cWriteRegister(AK4493S_ADR, Control2, 0x22);  // SDビットを1
    i2cWriteRegister(AK4493S_ADR, Control4, 0x00);  // SSLOWビットを0
    filter = 4;    
  }
  /* スーパースロー */
  else if (switchState == 4) {
    i2cWriteRegister(AK4493S_ADR, Control3, 0x00);  // SLOWビットを0
    i2cWriteRegister(AK4493S_ADR, Control2, 0x02);  // SDビットを0
    i2cWriteRegister(AK4493S_ADR, Control4, 0x01);  // SSLOWビットを1
    filter = 5;
  }
  /* 低分散 */
  else if (switchState == 5) {
    i2cWriteRegister(AK4493S_ADR, Control3, 0x00);  // SLOWビットを0
    i2cWriteRegister(AK4493S_ADR, Control2, 0x22);  // SDビットを1
    i2cWriteRegister(AK4493S_ADR, Control4, 0x01);  // SSLOWビットを1
    filter = 6;    
  }
  /* ショートディレイ・スローロールオフ */
//  else {
//    i2cWriteRegister(AK4493S_ADR, Control3, 0x00);  // SLOWビットを0
//    i2cWriteRegister(AK4493S_ADR, Control2, 0x22);  // SDビットを1
//    i2cWriteRegister(AK4493S_ADR, Control4, 0x00);  // SSLOWビットを0
//    filter = 3;    
//  }
}

void monitorLED(uint16_t fs, uint8_t filter) {
  boolean fil_fs = digitalRead(SW3);
  if (fil_fs == LOW) {
    if (fs == 32) {
      digitalWrite(LED[0], LOW);
      digitalWrite(LED[1], LOW);
      digitalWrite(LED[2], LOW);
    }
    else if (fs == 44) {
      digitalWrite(LED[0], HIGH);
      digitalWrite(LED[1], LOW);
      digitalWrite(LED[2], LOW);
    }
    else if (fs == 48) {
      digitalWrite(LED[0], LOW);
      digitalWrite(LED[1], HIGH);
      digitalWrite(LED[2], LOW);
    }
    else if (fs == 88) {
      digitalWrite(LED[0], HIGH);
      digitalWrite(LED[1], HIGH);
      digitalWrite(LED[2], LOW);
    }
    else if (fs == 96) {
      digitalWrite(LED[0], LOW);
      digitalWrite(LED[1], LOW);
      digitalWrite(LED[2], HIGH);
    }
    else if (fs == 176) {
      digitalWrite(LED[0], HIGH);
      digitalWrite(LED[1], LOW);
      digitalWrite(LED[2], HIGH);
    }
    else if (fs == 192) {
      digitalWrite(LED[0], LOW);
      digitalWrite(LED[1], HIGH);
      digitalWrite(LED[2], HIGH);
    }
    else if (fs == 384) {
      digitalWrite(LED[0], HIGH);
      digitalWrite(LED[1], HIGH);
      digitalWrite(LED[2], HIGH);
    }
    else {
      digitalWrite(LED[0], LOW);
      digitalWrite(LED[1], LOW);
      digitalWrite(LED[2], LOW);
    }
  }
  else if (fil_fs == HIGH) {
    if (filter == 1) {
      digitalWrite(LED[0], HIGH);
      digitalWrite(LED[1], LOW);
      digitalWrite(LED[2], LOW);      
    }
    else if (filter == 2) {
      digitalWrite(LED[0], LOW);
      digitalWrite(LED[1], HIGH);
      digitalWrite(LED[2], LOW);
    }
    else if (filter == 3) {
      digitalWrite(LED[0], HIGH);
      digitalWrite(LED[1], HIGH);
      digitalWrite(LED[2], LOW);
    }
    else if (filter == 4) {
      digitalWrite(LED[0], LOW);
      digitalWrite(LED[1], LOW);
      digitalWrite(LED[2], HIGH);
    }
    else if (filter == 5) {
      digitalWrite(LED[0], HIGH);
      digitalWrite(LED[1], LOW);
      digitalWrite(LED[2], HIGH);
    }
    else if ((filter == 6) || (filter == 0)){
      digitalWrite(LED[0], LOW);
      digitalWrite(LED[1], HIGH);
      digitalWrite(LED[2], HIGH);
    }
  }
}
