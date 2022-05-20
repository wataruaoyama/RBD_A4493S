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
#include "IRremote.h"

#include <SSD1306Ascii.h>
#include <SSD1306AsciiAvrI2c.h>

//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>

#include <FreqCount.h>

#define I2C_ADDRESS 0x3C

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

//126x64pixel SSD1306 OLED
//Adafruit_SSD1306 display(128, 64, &Wire, -1);

SSD1306AsciiAvrI2c oled;

//char pcm[] = "PCM ";
//char dsd[] = "DSD ";
char fs32[] = "32";
char fs44[] = "44.1";
char fs48[] = "48";
char fs88[] = "88.2";
char fs96[] = "96";
char fs176[] = "176.4";
char fs192[] = "192";
char fs352[] = "352.8";
char fs384[] = "384";
char fs768[] = "768";
char nofs[] = "";
uint8_t SW[] = {14, 15, 16};
uint8_t LED[] = {7, 8, 13};
uint8_t SW3 = 10;
uint8_t RPI_OK = 9;
uint8_t PDN = 17;
uint8_t receiver = 6;
boolean go = LOW;

uint8_t filter;

/*-----( Declare objects )-----*/
IRrecv irrecv(receiver);     // create instance of 'irrecv'
decode_results results;      // create instance of 'decode_results'

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

  Serial.begin(9600);
  FreqCount.begin(10);  // 周波数測定の開始．測定間隔を10ミリ秒に設定
  Wire.begin();
  Wire.setClock(400000);
//  while(go == LOW) {
//    go = digitalRead(RPI_OK);
//  }

  //u8g2.begin();
  //u8g2.clearBuffer();
  
  // SSD1306のスレーブアドレスは0x3C
//  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
//  display.clearDisplay();
//  display.display();
  
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  
  delay(500);
  digitalWrite(PDN, HIGH);
  initAK4493S();
  initOledDisplay();
  delay(4000);
  uint8_t dipsw = readDipSwitch();
  Serial.print("SW Status = "); Serial.println(dipsw);
  uint8_t dip3 = digitalRead(SW3);
  Serial.print("SW3 = "); Serial.println(dip3);
  initDigitalFilter(dipsw);
  Serial.print("filter = "); Serial.println(filter);
  irrecv.enableIRIn(); // Start the receiver
}

void loop() {
  irReceiver();
  uint16_t FSR = freqCounter();
//  displayOledFSR(FSR);
  selectLedDisplay(FSR, filter);
  delay(300);
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
//void initOledDisplay() {
//  // Adafruitのロゴ表示データを消去
//  display.clearDisplay();
//  display.display();
//  /* SSD1306 OLEDディスプレイに表示 */
//  display.setTextColor(SSD1306_WHITE);
//  display.setTextSize(2);
//  display.setCursor(5, 10);
//  display.println("RBD-A4493S");
//  display.setTextSize(1);
//  display.setCursor(30, 32);
//  display.println("Desgined by");
//  display.setTextSize(2);
//  display.setCursor(20, 50);
//  display.println("LINUXCOM");
//  display.display();
//}

void initOledDisplay() {
  oled.setFont(Arial14);
  oled.clear();
  oled.setCursor(20, 0);
  oled.print("RBD-A4493S");
  oled.setCursor(5, 3);
  oled.print("Hi-Res DAC Board");
  oled.setCursor(10, 5);
  oled.print("for Raspberry Pi");
  delay(3000);
  oled.clear();
  oled.setCursor(25, 1);
  oled.print("Designed by");
  oled.setCursor(0, 4);
  oled.set2X();
  oled.print("LINUXCOM");
}

//void displayOledFSR(uint16_t FSR) {
//  display.clearDisplay();
//  display.setTextSize(3);
//  display.setCursor(30, 26);
//  if (FSR == 32) display.println(fs32);
//  else if (FSR == 44) display.println(fs44);
//  else if (FSR == 48) display.println(fs48);
//  else if (FSR == 88) display.println(fs88);
//  else if (FSR == 96) display.println(fs96);
//  else if (FSR == 176) display.println(fs176);
//  else if (FSR == 192) display.println(fs192);
//  else if (FSR == 352) display.println(fs352);
//  else if (FSR == 384) display.println(fs384);
//  else if (FSR == 768) display.println(fs768);
//  //else display.println("No Signal");
//  display.display();
//}

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

//void initBoard() {
//  
//}

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
  else {
    i2cWriteRegister(AK4493S_ADR, Control3, 0x00);  // SLOWビットを0
    i2cWriteRegister(AK4493S_ADR, Control2, 0x22);  // SDビットを1
    i2cWriteRegister(AK4493S_ADR, Control4, 0x00);  // SSLOWビットを0
    filter = 3;    
  }
}

void selectLedDisplay(uint16_t fs, uint8_t filter) {
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
//    else {
//      digitalWrite(LED[0], LOW);
//      digitalWrite(LED[1], LOW);
//      digitalWrite(LED[2], LOW);
//    }
  }
}

/*************************************************************
  IRリモコン受信コントローラ
  ********************************
  
  Apple Reoteまたは秋月電子通商で購入可能なOptoSupplyのリモコンに対応
  
 *************************************************************/
void irReceiver() {

  controlByIR();
  //return(count);
}

//void translateIR() // takes action based on IR code received
//
//// describing Remote IR codes 
//
//{
//
//  switch(results.value)
//
//  {
//  case 0xFFA25D: Serial.println("POWER"); break;
//  case 0xFFE21D: Serial.println("FUNC/STOP"); break;
//  case 0xFF629D: Serial.println("VOL+"); break;
//  case 0xFF22DD: Serial.println("FAST BACK");    break;
//  case 0xFF02FD: Serial.println("PAUSE");    break;
//  case 0xFFC23D: Serial.println("FAST FORWARD");   break;
//  case 0xFFE01F: Serial.println("DOWN");    break;
//  case 0xFFA857: Serial.println("VOL-");    break;
//  case 0xFF906F: Serial.println("UP");    break;
//  case 0xFF9867: Serial.println("EQ");    break;
//  case 0xFFB04F: Serial.println("ST/REPT");    break;
//  case 0xFF6897: Serial.println("0");    break;
//  case 0xFF30CF: Serial.println("1");    break;
//  case 0xFF18E7: Serial.println("2");    break;
//  case 0xFF7A85: Serial.println("3");    break;
//  case 0xFF10EF: Serial.println("4");    break;
//  case 0xFF38C7: Serial.println("5");    break;
//  case 0xFF5AA5: Serial.println("6");    break;
//  case 0xFF42BD: Serial.println("7");    break;
//  case 0xFF4AB5: Serial.println("8");    break;
//  case 0xFF52AD: Serial.println("9");    break;
//  /* Apple Remote */
//  case 0x77E1504F: Serial.println("APPLE UP"); break;
//  case 0x77E1304F: Serial.println("APPLE DOWN");  break;
//  case 0x77E1904F: Serial.println("APPLE LEFT");  break;
//  case 0x77E1604F: Serial.println("APPLE RHIGT"); break;
//  case 0x77E1C04F: Serial.println("APPLE MENU");  break;
//  case 0x77E1FA4F: Serial.println("APPLE PLAY/PAUSE");  break;
//  case 0x77E13A4F: Serial.println("APPLE OK");  break;  // 0x77E1A04F
//  /* OptoSupply */
//  case 0x8F705FA: Serial.println("OptoSupply ^"); break;
//  case 0x8F704FB: Serial.println("OptoSupply o"); break;
//  case 0x8F700FF: Serial.println("OptoSupply v"); break;
//  case 0x8F708F7: Serial.println("OptoSupply <-"); break;
//  case 0x8F701FE: Serial.println("OptoSupply ->"); break;
//  case 0x8F78D72: Serial.println("OptoSupply ^¥"); break;
//  case 0x8F7847B: Serial.println("OptoSupply /^"); break;
//  case 0x8F78877: Serial.println("OptoSupply v/"); break;
//  case 0x8F7817E: Serial.println("OptoSupply ¥v"); break;
//  case 0x8F71FE0: Serial.println("OptoSupply A"); break;
//  case 0x8F71EE1: Serial.println("OptoSupply B"); break;
//  case 0x8F71AE5: Serial.println("OptoSupply C"); break;
//  case 0x8F71BE4: Serial.println("OptoSupply POWER"); break;
//  
//  case 0xFFFFFFFF: Serial.println(" REPEAT"); break;
//
//  default: 
//    //Serial.println(" other button   ");
//
//  }
//
////  delay(100); // Do not get immediate repeat
//
//}

void controlByIR()
{
  uint8_t i;
  static int irkey = 0;
  static bool mute = true;
  
  if (irrecv.decode(&results)) // have we received an IR signal?

  {   
    /* デジタルフィルタの切り替え */
    // リモコンのRIGHT（OptoSupplyは->）が押された場合
    if (results.value == 0x8F701FE) {
      filter++;
      Serial.print("filter = "); Serial.println(filter);
      if (filter == 1) {
        
      }
      else if (filter == 2) {
        
      }
      else if (filter == 3) {
        
      }
      else if (filter == 4) {
        
      }
      else if (filter == 5) {
        
      }
      else if (filter == 6) {
        filter = 0;
      }
    }
    irrecv.resume(); // receive the next value
    //if (results.value != 0xFFFFFFFF) irkey = results.value;
    //Serial.print("filter = "); Serial.println(filter);
  }
  delay(100);
}
