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

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /*SCL, SDA,*/ /* reset=*/ U8X8_PIN_NONE);

char pcm[] = "PCM ";
char dsd[] = "DSD ";
char fs32[] = "32";
char fs44[] = "44.1";
char fs48[] = "48";
char fs88[] = "88.2";
char fs96[] = "96";
char fs176[] = "176.4";
char fs192[] = "192";
char fs352[] = "352.8";
char fs384[] = "384";
char nofs[] = "";
uint8_t SW[] = {14, 15, 16};
uint8_t LED[] = {7, 8, 9};
uint8_t SW3 = 21;
uint8_t RPI_OK = 9;
uint8_t PDN = 17;
uint8_t IR = 6;
boolean go = LOW;

void setup() {
  int i;
  for (i=0; i<=2; i++) {
    pinMode(LED[i], OUTPUT);
    pinMode(SW[i], INPUT_PULLUP);
  }
  pinMode(SW3, INPUT_PULLUP);
  pinMode(RPI_OK, INPUT);
  pinMode(PDN, OUTPUT);
  pinMode(LED, OUTPUT);

  Serial.begin(115200);
  FreqCount.begin(10);  // 周波数測定の開始．測定間隔を10ミリ秒に設定
  Wire.begin();
  Wire.setClock(400000);
  while(go == LOW) {
    go = digitalRead(RPI_OK);
  }
  u8g2.begin();
  u8g2.clearBuffer();
  delay(500);
  digitalWrite(PDN, HIGH);
  initAK4493S();
  initOledDisplay();
  delay(4000);
}

void loop() {
  uint16_t FSR = freqCounter();
  displayOledFSR(FSR);
  delay(10);
}

uint8_t i2cReadRegister(uint8_t regadr) {
  Wire.beginTransmission(AK4493S_ADR);
  Wire.write(regadr);
  Wire.endTransmission();
  Wire.requestFrom(AK4493S_ADR, 1);
  return Wire.read();
}

void i2cWriteRegister(uint8_t regadr, uint8_t wdata) {
  Wire.beginTransmission(AK4493S_ADR);
  Wire.write(regadr);
  Wire.write(wdata);
  Wire.endTransmission();
}

uint8_t readDipSwich() {
  uint8_t switchStatus;
  int i;
  for (i=0; i<=2; i++) {
    switchStatus = switchStatus << 1 | digitalRead(SW[i]);
  }
  return(switchStatus);
}

uint16_t freqCounter() {
  if (FreqCount.available()) {
    unsigned long count = 100 * FreqCount.read(); // 測定間隔を10ミリ秒としたので100倍にする
    if ( (count >= 44100) && (count <= 44200) ) count = 44;
    else if ( (count >= 48000) && (count <= 48100)) count = 48;
    else if ( (count >= 88200) && (count <= 88300)) count = 88;
    else if ( (count >= 96000) && (count <= 96100)) count = 96;
    else if ( (count >= 176400) && (count <= 176600) ) count = 176;
    else if ( (count >= 192100) && (count <= 192299) ) count = 192;
    else if ( (count >= 352700) && (count <= 352900) ) count = 352;
    else if ( (count >= 384300) && (count <= 384400)) count = 384;
    //Serial.println(count);
    return(uint16_t (count));
  }  
}

void initOledDisplay() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvB12_tr);
  u8g2.drawStr(10,14,"RBD-A4493S");
  u8g2.setFont(u8g2_font_helvR10_tr);
  u8g2.drawStr(20,36,"Designed by");
  u8g2.setFont(u8g2_font_helvB12_tr);
  u8g2.drawStr(20,60,"LINUXCOM");
  u8g2.sendBuffer();
}

void displayOledFSR(uint16_t FSR) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_helvR24_tr);
  u8g2.setCursor(22, 48);
  if (FSR == 32) u8g2.print(fs32);
  else if (FSR == 44) u8g2.print(fs44);
  else if (FSR == 48) u8g2.print(fs48);
  else if (FSR == 88) u8g2.print(fs88);
  else if (FSR == 96) u8g2.print(fs96);
  else if (FSR == 176) u8g2.print(fs176);
  else if (FSR == 192) u8g2.print(fs192);
  else if (FSR == 352) u8g2.print(fs352);
  else if (FSR == 384) u8g2.print(fs384);
//  else if (FSR == 2822) u8g2.print(fs282);
//  else if (FSR == 5644) u8g2.print(fs564);
//  else if (FSR == 1128) u8g2.print(fs1128);
//  else if (FSR == 2256) u8g2.print(fs2256);
  else u8g2.print(nofs);
  u8g2.setFont(u8g2_font_helvR10_tr);
  if (FSR <=384) u8g2.print(" kHz");
  else if (FSR >=2822) u8g2.print(" MHz");
  u8g2.sendBuffer();
}

void initAK4493S() {
  /**********************
   * AK4493SEQ 初期設定
   **********************/
//  i2cWriteRegister(AK4499S_ADR, Control1, 0x8F);
//  i2cWriteRegister(Control2, 0x22);
//  i2cWriteRegister(Control3, 0x00);
//  i2cWriteRegister(LchATT, 0xFF);
//  i2cWriteRegister(RchATT, 0xFF);
//  i2cWriteRegister(Control4, 0x00);
//  i2cWriteRegister(Dsd1, 0x00);
//  i2cWriteRegister(Control5, 0x01);
//  i2cWriteRegister(SoundControl, 0x00);
//  i2cWriteRegister(Dsd2, 0x00);
//  i2cWriteRegister(Control6, 0x04);
//  i2cWriteRegister(Control7, 0x00);
//  i2cWriteRegister(Control8, 0x80);
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
