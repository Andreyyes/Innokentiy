#include <Adafruit_NeoPixel.h>
#include <SBUS.h>
#include <Servo.h>
#include <Wire.h>
#include "core.h"
#include "constants.h"
//#include <Thread.h>

Servo servoTiltClav;
Servo servoTorisonClav;
Servo servoManipulator;
Servo servoSensor;
int servo[4] = {SERV_MID_MANI, SERV_MID_DAT, SERV_MID_TILT, SERV_MID_TORISON};

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(9, 22, NEO_GRB + NEO_KHZ800);

SBUS sbus(Serial3);

int flagTypeControl;

//масивы которые будут передаваться по телеметрии
byte REG_ArrayCD[16] = {0x1, 0xCD, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
byte REG_ArrayAB[16] = {0x1, 0xAB, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//пины мотора манипулятора
int pwmLpin[2] = {2, 3};
int inPin[2] = {24, A11};//26

//пины моторов
int inApin[2] = {7, 4};
int inBpin[2] = {8, 9};
int pwmpin[2] = {5, 6};

//пины дольномеров
uint8_t usRangePin[3][2] = {{40, 38}, {A14, A15}, {A12, A13}};
uint8_t ikRangePin[3] = {A8, A9, A10};

//Пин управления поливом
uint8_t waterPin = 50;

int cnt = 0;

byte flagSensorLine;

/*=================================ИНИЦИАЛИЗАЦИЯ=========================================*/
void setup()
{
  pixels.begin();
  colorWipe(pixels.Color(255, 0, 0));
  pixels.show();

  Wire.begin(0x04);
  Wire.onRequest(requestEvent);

  sbus.begin();
  //Serial.begin(115200);
  coreSetup();
  registerHandleSerialKeyValue(&handleSerialKeyValue);
  registerHandleSerialCmd(&handleSerialCmd);
  
  pinMode(waterPin, OUTPUT);//Пин полива
  digitalWrite(waterPin, LOW);

  for (int i = 0; i < 3; i++) {
    pinMode(usRangePin[i][0], OUTPUT);//trig
    pinMode(usRangePin[i][1], INPUT);//echo
  }

  for (int i = 0; i < 2; i++) {
    pinMode(pwmLpin[i], OUTPUT);
    pinMode(inPin[i], OUTPUT);
    digitalWrite(inPin[i], LOW);
  }

  for (int i = 0; i < 2; i++) {
    pinMode(inApin[i], OUTPUT);
    pinMode(inBpin[i], OUTPUT);
    pinMode(pwmpin[i], OUTPUT);
  }
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(inApin[i], LOW);
    digitalWrite(inBpin[i], LOW);
  }

  for (int i = 0; i < 3; i++)
  {
    pinMode(ikRangePin[i], INPUT);
  }

  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 2; j++)
    {
      pinMode(usRangePin[i][j], INPUT);
    } 
  }

  //servoTiltClav.attach(16);
  //servoTorisonClav.attach(17);
  //servoManipulator.attach(18);
  //servoSensor.attach(19);

  servoTiltClav.attach(28);
  servoTorisonClav.attach(32);
  servoManipulator.attach(34);
  servoSensor.attach(36);
  
  servoTiltClav.writeMicroseconds(SERV_MID_TILT);
  servoTorisonClav.writeMicroseconds(1300);
  servoManipulator.writeMicroseconds(SERV_MID_MANI);
  //servoSensor.writeMicroseconds(SERV_MID_DAT);
  servoSensor.writeMicroseconds(1943);

  colorWipe(pixels.Color(0, 255, 0));
  pixels.show(); 
}


bool flagSBUS = false;
ISR(TIMER2_COMPA_vect)
{
  flagSBUS = true;
}

int camCount = 0;


#define SCAN_STOP 0
#define SCAN_WORK 1
uint16_t scanMicroseconds = SERV_MID_MANI;
uint16_t scanState = SCAN_STOP;

void handleSerialKeyValue(String key, String value) {
  if ((key == "angle")&&(scanState == SCAN_WORK)) {
    int16_t angle = value.toInt();
    angle = constrain(angle, -90, 155);
    uint16_t microseconds = map(angle, -90, 155, 550, 2400);
    Serial.print("Microseconds: ");
    Serial.println(microseconds);
    servoManipulator.writeMicroseconds(microseconds);
  }
}

void handleSerialCmd(String cmd) {
  
}

/*=================================ГЛАВНЫЙ ЦИКЛ=========================================*/
void loop()
{
  coreLoop();
  telemPutIntAB(YAW,flagSensorLine);

  if (flagSBUS) {
    flagSBUS = false;
    sbus.process();
  }

  int left = digitalRead(41);
  int bLeft = digitalRead(43);
  int mid = digitalRead(45);
  int bRight = digitalRead(47);
  int right = digitalRead(49);

  telemPutBitsCD(LATI, left, bLeft, mid, bRight, right, 0);

  switchCam();

  int kleshnyaSzatie = printSBUSStatus(SV_C);
  if (abs(kleshnyaSzatie) > 260) kleshnyaSzatie = 0; 
  motorLGo(0, kleshnyaSzatie * (-1));  

  //управление манипулятором и остановка моторов
  if (printSBUSStatus(SV_F) > 0) {
    motorOff(0);
    motorOff(1);
    setServo270(servoTorisonClav, printSBUSStatus(RSH)* (-1), 3, SERV_MAX_TORISON, SERV_MIN_TORISON);
    setServo270(servoTiltClav, printSBUSStatus(LSW), 2, SERV_MAX_TILT, SERV_MIN_TILT);
    if (printSBUSStatus(SV_B) < 0) {
      if (scanState == SCAN_STOP) {
        Serial1.print("?startscan\n");
        scanState = SCAN_WORK;
      }
    } else {
      if (scanState == SCAN_WORK) {
        Serial1.print("?stopscan\n");
        scanState = SCAN_STOP;
      }
      setServo270(servoManipulator, printSBUSStatus(RSW), 0, SERV_MAX_MANI, SERV_MIN_MANI);
    }    
    motorLGo(1, printSBUSStatus(LSH));//наклон маниаулятора
    if (printSBUSStatus(SV_A) < 0) {
      digitalWrite(waterPin, HIGH);
    } else {
      digitalWrite(waterPin, LOW);
    }
    return;
  } else {
    flagTypeControl = setFlagTypeControl();
  }

  if (flagTypeControl == MOD_MOVE) {
    routeMove();
    setServo270(servoTorisonClav, printSBUSStatus(LSH)* (-1), 2, SERV_MAX_TILT, SERV_MIN_TILT);
    //setServo270(servoManipulator, printSBUSStatus(LSW), 0, SERV_MAX_MANI, SERV_MIN_MANI);
    return;
  }
  if (flagTypeControl == MOD_TANK) {
    motorGo(0, printSBUSStatus(LSH));
    motorGo(1, printSBUSStatus(RSH));
    return;
  }

  if ((flagTypeControl == MOD_RIGHT) || (flagTypeControl == MOD_LEFT)) {
    rangeRan(100, printSBUSStatus(SV_A) > 0, printSBUSStatus(SV_B) > 0);//тип датчика и какая сторона
    return;
  }

  if ((flagTypeControl == MOD_TOP) || (flagTypeControl == MOD_DOWN)) {
    line(100);
    //setServo270(servoSensor, flagTypeControl == 60, 1, SERV_MAX_DAT, SERV_MIN_DAT);
    if (flagTypeControl == MOD_TOP) {
      servoSensor.writeMicroseconds(SERV_MAX_DAT);
      flagSensorLine = 100;
    } else {
      servoSensor.writeMicroseconds(SERV_MIN_DAT);
      flagSensorLine = 200;
    }
    return;
  }
  
}

/*=================================ТЕЛЕМЕТРИЯ=========================================*/
//телеметрия LATI и LONG


void telemPutBitsCD(byte regNum, byte b0, byte b1, byte b2, byte b3, byte b4, byte b5) {
  long rez = b0 * 1000000 + b1 * 100000 + b2 * 10000 + b3 * 1000 + b4 * 100 + b5 * 10;
  telemPutLongCD(regNum, rez);
}

void telemPutLongCD(byte regNum, long value) {
  REG_ArrayCD[regNum] = value >> 24;
  REG_ArrayCD[regNum + 1] = value >> 16;
  REG_ArrayCD[regNum + 2] = value >> 8;
  REG_ArrayCD[regNum + 3] = value;
}

void telemPutIntAB (int regNum, int value) {
  REG_ArrayAB[regNum] = value >> 8;
  REG_ArrayAB[regNum + 1] = value;
}

void telemPutIntCD (int regNum, int value) {
  REG_ArrayCD[regNum] = value >> 8;
  REG_ArrayCD[regNum + 1] = value;
}

//установить значение телеметрии
void telemPutGPS(byte value) {
  REG_ArrayAB[GPS] = value;
  REG_ArrayCD[GPS] = value;
}

//прерывание передача телеметрии
bool flagArray = false;
void requestEvent()
{
  if (flagArray) {
    for (int i = 0; i < 16; i++) {
      Wire.write(REG_ArrayAB[i]);
    }
  } else {
    for (int i = 0; i < 16; i++) {
      Wire.write(REG_ArrayCD[i]);
    }
  }
  flagArray = !flagArray;
}

/*====================================SBUS===========================================*/
//принятие сигнала с пульта от -255 до 255
float printSBUSStatus(int n)
{
  float z = sbus.getChannel(n);
  z = (z - 999.5) / 2.73;
  return z;
}

/*====================================СЕРВО===========================================*/
void setServo270(Servo s, int n, int num, int servMax, int servMin) {
  cnt++;
  if (cnt > 40) {
    cnt = 0;
    if ((n < 15) && (n > -15)) {
      n = 0;
    } else {
      servo[num] = servo[num] + prSpeed(n) / 3;
      if (servo[num] > servMax) {
        servo[num] = servMax;
      } else if (servo[num] < servMin) {
        servo[num] = servMin;
      }
      s.writeMicroseconds(servo[num]);
    }
  }
}

/*====================================ДАТЧИКИ===========================================*/
//определение дистанции n-ого ультрозвукового дальномера
int range(int n) {
  //int duration, cm;
  digitalWrite(usRangePin[n][0], LOW);
  delayMicroseconds(2);
  digitalWrite(usRangePin[n][0], HIGH);
  delayMicroseconds(10);
  digitalWrite(usRangePin[n][0], LOW);
  return round(pulseIn(usRangePin[n][1], HIGH) / 58);
}

int ikRange(int n) {
  int z = analogRead(ikRangePin[n]);

  return z;//65*pow(z*0.0048828125, -1.10);
}

/*====================================МОТОРЫ===========================================*/
//функция установки скорости мотороа motor - номер мотора, pwm - скорость (отрицательная в обратную сторону)
void motorGo(int motor, int pwm)
{
  if (pwm == 0) {
    motorOff(motor);
    return;
  }
  if (pwm < 0) {
    digitalWrite(inApin[motor], HIGH);
    digitalWrite(inBpin[motor], LOW);
  } else {
    digitalWrite(inApin[motor], LOW);
    digitalWrite(inBpin[motor], HIGH);
  }
  analogWrite(pwmpin[motor], abs(prSpeed(pwm)));

  if (motor == 0) {telemPutIntAB(SPEED, pwm);} else
  {telemPutIntCD(RISE, pwm);}
  
}

//функция остановки мотороа true - плавное false - резкое
void motorOff(int motor)
{
  boolean f;
  if (printSBUSStatus(SV_B) > 0) {
    f = HIGH;
  } else {
    f = LOW;
  }
  digitalWrite(inApin[motor], f);
  digitalWrite(inBpin[motor], f);
  analogWrite(pwmpin[motor], 0);
}

//движение мотора на манипуляторе
void motorLGo(int motor, int pwm)
{
  if (abs(pwm) > 100) {
    if (pwm > 0) {
      digitalWrite(inPin[motor], HIGH);
    } else {
      digitalWrite(inPin[motor], LOW);
    }
  } else {
    pwm = 0;
  };
  analogWrite(pwmLpin[motor], abs(pwm));
}

/*====================================АВТОМАТИКА===========================================*/
//движение на определённой дистанции true - ультразуковый false - инфакрасные
int d;
boolean sFlag;
void rangeRan(byte sp, boolean typeR, int dist) {
  int left;
  int right;
  int mid;
  if (/*typeR*/false) {
    left = range(1);
    right = range(2);
    mid = range(0);
  } else {
    left = ikRange(1);
    right = ikRange(0);
    mid = ikRange(2);

    right = (900 - right);
    mid = (900 - mid);
    left = (900 - left);
  }

  telemPutIntAB(ROLL, left);
  telemPutIntAB(PITC, mid);
  telemPutIntAB(DIST, right);
  telemPutIntAB(ALT, d);

  if (dist) {
    motorOff(1);
    motorOff(0);
    if (left > right) {
      d = right;
      sFlag = true;
      telemPutLongCD(LONG, 110);
      //telemPutBitsCD(LONG, 0,0,0,0,1,1);
    } else {
      d = left;
      sFlag = false;
      telemPutLongCD(LONG, 1100000);
      //telemPutBitsCD(LONG, 1,1,0,0,0,0);
    }
    return;
  }
  int coeff = 1;
  int e = (d - right);
  if (sFlag) {
    if (e > 0) {
      motorGo(0, sp);
      motorGo(1, sp + abs(e) * coeff);
    } else {
      motorGo(0, sp + abs(e) * coeff);
      motorGo(1, sp);
    }
  } else {
    if (e < 0) {
      motorGo(0, sp);
      motorGo(1, sp + abs(e) * coeff);
    } else {
      motorGo(0, sp + abs(e) * coeff);
      motorGo(1, sp);
    }
  }
}

//функция движения по линии
void line(byte sp) {
  int right = digitalRead(41);
  int bRight = digitalRead(43);
  int mid = digitalRead(45);
  int bLeft = digitalRead(47);
  int left = digitalRead(49);

  telemPutBitsCD(LATI, left, bLeft, mid, bRight, right, 0);

  if (left == right) {
        motorGo(0,120);
        motorGo(1,100);
        return;
    }else
    if (left == mid){
        motorGo(1,255);
        motorGo(0,-255);
     
    } else {
        motorGo(1,-255);
        motorGo(0,255);
    }
}

/*====================================УПРАВЛЕНИЕ_ПУЛЬТ===========================================*/
//функция управления движением с одного стика
void routeMove() {
  int f = printSBUSStatus(RSH);
  int d = printSBUSStatus(RSW);
  int ml = f;
  int mr = f;
  if (d >= 0) {
    ml = ml - d;
    mr = mr + d;
  } else {
    d = abs(d);
    mr = mr - d;
    ml = ml + d;
  }
  if (mr > 255) {
    mr = 255;
  }
  if (ml > 255) {
    ml = 255;
  }
  motorGo(0, ml);
  motorGo(1, mr);
}

/*====================================КАМЕРА===========================================*/
void switchCam() {
  int stat = printSBUSStatus(SV_G);
  
  //if (stat != camStatus) {
  byte camPin[2] = {0, 0};
  if (stat < 30 && stat > -30) {
    camPin[0] = 1;
  } else {
    if (stat > 0) {
      camPin[1] = 1;
    }
  }
  digitalWrite(53, camPin[0]);
  digitalWrite(51, camPin[1]);

}

/*=======================================СВЕТОДИОДЫ========================================*/
void colorWipe(uint32_t c) {
  for (uint16_t i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, c);
    pixels.show();
  }
}

/*=======================================ОБЩИЕ========================================*/
float prSpeed(float n) {
  float z = sbus.getChannel(SV_BR);
  z = (abs((z - 306))) / 1388;
  return round(z * n);
}

int setFlagTypeControl() {
  int type = printSBUSStatus(SV_A);
  if (                type < -204) {
    return MOD_TOP;
  }
  if (type >= -204 && type < -100) {
    return MOD_MOVE;
  }
  if (type >= -100 && type < 0) {
    return MOD_RIGHT;
  }
  if (type >= 0 && type < 100) {
    return MOD_DOWN;
  }
  if (type >= 100 && type < 204) {
    return MOD_TANK;
  }
  if (type >= 204) {
    return MOD_LEFT;
  }
}
