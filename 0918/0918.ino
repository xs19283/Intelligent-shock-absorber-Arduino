//////////////////////庫//////////////////////
#include <Servo.h>
#include "HX711.h"

//////////////////////實例化物件//////////////////////
Servo myservo;
HX711 scale;

//////////////////////五大模式變數//////////////////////
volatile int NowMode;
int CtrlGoStop;

//////////////////////開關跟按鈕、LED、馬達變數//////////////////////
#define NUMBER 5
#define LED 7
#define BUTTONRED 12
#define BUTTONBULE 13
#define SW 8
int red, bule;      //按鈕變數
int OnOff;          //監控是否自動手動變數
int Seg = 1;        //為手動調整伺服馬達變數

//////////////////////霍爾相關變數及陣列//////////////////////
float Hx711Array[NUMBER];
float Hx711SortArray[NUMBER];
int HallStartValue = 0;
int SendHx711;
int MedienHx711;

//////////////////////Z軸相關變數及陣列//////////////////////
float ZAxisArray [NUMBER];
float ZSortArray [NUMBER];
int ZStartValue = 0;
int SendZ;
int MedienZ;
const int ZPin = A3;

//////////////////////X軸相關變數及陣列//////////////////////
float XAxisArray[NUMBER];
float XSortArray[NUMBER];
int XStartValue = 0;
int SendX;
int MedienX;
const int XPin = A2;

//////////////////////外部中斷控制變數//////////////////////
int CtrlInti = 0;
int IntBreak = 3;
int HILO;

void setup() {
  Serial.begin(115200);
  //////////////////////設定開關跟按鈕//////////////////////
  pinMode(BUTTONRED, INPUT);
  pinMode(BUTTONBULE, INPUT);
  pinMode(SW, INPUT_PULLUP);
  //////////////////////設定伺服馬達//////////////////////
  myservo.attach(6);
  //////////////////////設定應變規//////////////////////
  // HX711.DOUT  - pin #A1
  // HX711.PD_SCK - pin #A0
  scale.begin(A1, A0);
  scale.set_scale(3780.f);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();               // reset the scale to 0
  //////////////////////在程式初始時先放五個 Hall&X&Z 元素進陣列//////////////////////
  PutValueArray(Hx711Array);
  PutValueArray(XAxisArray, XPin, &XStartValue, "X");
  PutValueArray(ZAxisArray, ZPin, &ZStartValue, "Z");
  //////////////////////藍芽初始化//////////////////////
  /*pinMode(9, OUTPUT);
    digitalWrite(9, HIGH);
    delay(1000);*/
  //////////////////////外部中斷設定//////////////////////
  //attachInterrupt(0, InteHall, HIGH); //assign int0
  /*pinMode(IntBreak, INPUT);*/
  //////////////////////所有設定已完成 LED亮起//////////////////////
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
}

//////////////////////外部中斷涵式//////////////////////
void InteHall() {
  CtrlInti = 1;
  digitalWrite(LED, LOW);
  if (SendX > 20 & SendZ > 20) {
    MotorCmd(2);
    NowMode = 5;
  }

  CtrlInti = 0;
}

//////////////////////Hx711陣列初始化完放五個元素//////////////////////
void PutValueArray(float Array[]) {
  for (int i = 0; i < NUMBER; i++) {
    Array[i] = scale.get_units();
  }
  HallStartValue = scale.get_units();
}

void PutValueArray(float Array[], int Pin, int* StartValue, String Axis) {
  if (Axis == "X") {
    for (int i = 0; i < NUMBER; i++) {
      Array[i] = ((analogRead(Pin) - 331.5) / 65 * 9.8);
    }
    *StartValue = ((analogRead(Pin) - 331.5) / 65 * 9.8);
  }
  if (Axis == "Z") {
    for (int i = 0; i < NUMBER; i++) {
      Array[i] = ((analogRead(Pin) - 340) / 68 * 9.8);
    }
    *StartValue = ((analogRead(Pin) - 340) / 68 * 9.8);
  }
}

//////////////////////Hx711陣列放值進第一元素//////////////////////
void ShiftRightArray(float Array[]) {
  for (int i = NUMBER - 1; i > 0; i--) {
    Array[i] = Array[i - 1];
  }
  Array[0] =  scale.get_units();
}

void ShiftRightArray(float Array[], String Axis, int Pin) {
  if (Axis == "Z") {
    for (int i = NUMBER - 1; i > 0; i--) {
      Array[i] = Array[i - 1];
    }
    Array[0] = ((analogRead(Pin) - 340) / 68 * 9.8);
  }
  if (Axis == "X") {
    for (int i = NUMBER - 1; i > 0; i--) {
      Array[i] = Array[i - 1];
    }
    Array[0] = ((analogRead(Pin) - 331.5) / 65 * 9.8);
  }
}

//////////////////////陣列排序//////////////////////
void SortByArray(float SortArray[], float Array[]) {
  float temp;

  for (int i = 0; i < NUMBER; i++) {
    SortArray[i] = Array[i];
  }

  for (int i = 0; i < NUMBER - 1; i++) {
    for (int j = 0; j < NUMBER - 1; j++) {
      if (SortArray[j + 1] < SortArray[j]) {
        temp = SortArray[j];
        SortArray[j] = SortArray[j + 1];
        SortArray[j + 1] = temp;
      }
    }
  }
}

//////////////////////標準差計算涵式//////////////////////
void VariancePulsByLoad (float SortArray[], int* SendData) {
  float Sum1, Sum2, ToTal;

  Sum1 = pow(SortArray[1] - SortArray[2], 2);
  Sum2 = pow(SortArray[3] - SortArray[2], 2);
  ToTal = sqrt(Sum1 + Sum2);
  *SendData = ToTal;
}

//////////////////////馬達控制函式//////////////////////
void MotorCmd(int angle) {
  switch (angle) {
    case 1:
      myservo.write(0);
      break;
    case 2:
      myservo.write(25);
      break;
    case 3:
      myservo.write(50);
      break;
    case 4:
      myservo.write(80);
      break;
    case 5:
      myservo.write(100);
      break;
    case 6:
      myservo.write(125);
      break;
  }
}

//////////////////////計算中位數//////////////////////
void CalculateByMedian(float SortArray[], int StartValue, int* Send) {

  float Median, total;
  Median = SortArray[2];
  float CalculateValue = abs(StartValue - Median);
  *Send = CalculateValue;
}

//////////////////////按鈕控制馬達//////////////////////
void BlueAndRed() {

  red = digitalRead(BUTTONRED);
  bule = digitalRead(BUTTONBULE);
  if (red == HIGH && Seg < 6) {
    Seg++;
    MotorCmd(Seg);
    delay(250);
  } else if (bule == HIGH && Seg > 1) {
    Seg--;
    MotorCmd(Seg);
    delay(250);
  }
}

//////////////////////藍芽傳值涵式//////////////////////
void BluetoothSendData() {

  //Serial.println(85);
  Serial.print("X  ");
  Serial.println(SendX);
  Serial.print("Z  ");
  Serial.println(SendZ);
  Serial.print("Hx711  ");
  Serial.println(SendHx711);

  /*
    Serial.write(85);
    Serial.write(analogRead(XPin));
    Serial.write(analogRead(ZPin));
    Serial.write(analogRead(HallPin));
    Serial.write(1);
  */
}

//////////////////////模式選擇//////////////////////
void NowModeSwitch() {
  HILO = digitalRead(IntBreak);

  if (HILO == LOW) {
    InteHall();
  } else if (SendZ >= 20  && CtrlInti == 0) {
    digitalWrite(LED, HIGH);
    MotorCmd(6);
    NowMode = 4;
  } else if (SendX > 9 && CtrlInti == 0) {
    digitalWrite(LED, HIGH);
    MotorCmd(2);
    NowMode = 3;
  } else if (SendHx711 > 15 && CtrlInti == 0) {
    digitalWrite(LED, HIGH);
    if (SendHx711 > 50) {
      MotorCmd(5);
      NowMode = 2;
    } else if (SendHx711 > 25) {
      MotorCmd(4);
      NowMode = 2;
    } else if (SendHx711 > 10) {
      MotorCmd(3);
      NowMode = 2;
    }
  } else if (SendZ <= 10) {
    digitalWrite(LED, HIGH);
    MotorCmd(1);
    NowMode = 1;
  }
}

//////////////////////手動自動開關判斷//////////////////////
void SwitchOnOff() {
  OnOff = digitalRead(SW);
  if (OnOff == HIGH) {
    SendX = (((float)analogRead(XPin) - 331.5) / 65 * 9.8);
    SendZ = (((float)analogRead(ZPin) - 340) / 68 * 9.8);
    ShiftRightArray(Hx711Array);
    //ShiftRightArray(ZAxisArray, "Z", ZPin);
    //ShiftRightArray(XAxisArray, "X", XPin);
    SortByArray(Hx711SortArray, Hx711Array);
    //SortByArray(ZSortArray, ZAxisArray);
    //SortByArray(XSortArray, XAxisArray);
    VariancePulsByLoad(Hx711SortArray, &SendHx711);
    //CalculateByMedian(XSortArray, XStartValue, &MedienX);
    //CalculateByMedian(ZSortArray, ZStartValue, &MedienZ);
    NowModeSwitch();
    BluetoothSendData();
  } else {
    BlueAndRed();
  }
}

void loop() {
  SwitchOnOff();
}
