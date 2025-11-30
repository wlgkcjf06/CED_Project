#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>


#define ENA 6
#define ENB 5
#define EN1 7
#define EN2 3
#define EN3 4
#define EN4 2

#define LT_L A2
#define LT_F A1
#define LT_R A0
#define LIGHT_SENSOR A3

#define BLACK_TH 150
#define DARK_TH  500   // 암막 더 안정적으로 감지

#define DIR_FW 1
#define DIR_LF 2
#define DIR_RF 3
#define DIR_TA 4
#define DIR_ST 5

int g_direction = DIR_ST;

int BASE_SPEED = 120;
int TURN_SPEED = 110;

void go_straight() {
  digitalWrite(EN1, LOW);
  digitalWrite(EN2, HIGH);
  digitalWrite(EN3, LOW);
  digitalWrite(EN4, HIGH);
  analogWrite(ENA, BASE_SPEED);
  analogWrite(ENB, BASE_SPEED);
}

void turn_left() {
  digitalWrite(EN1, LOW);
  digitalWrite(EN2, HIGH);
  digitalWrite(EN3, HIGH);
  digitalWrite(EN4, LOW);
  analogWrite(ENA, TURN_SPEED);
  analogWrite(ENB, TURN_SPEED);
}

void turn_right() {
  digitalWrite(EN1, HIGH);
  digitalWrite(EN2, LOW);
  digitalWrite(EN3, LOW);
  digitalWrite(EN4, HIGH);
  analogWrite(ENA, TURN_SPEED);
  analogWrite(ENB, TURN_SPEED);
}

void u_turn() {
  digitalWrite(EN1, HIGH);
  digitalWrite(EN2, LOW);
  digitalWrite(EN3, LOW);
  digitalWrite(EN4, HIGH);
  analogWrite(ENA, TURN_SPEED);
  analogWrite(ENB, TURN_SPEED);
}

void stop_car() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

bool isLeft()   { return analogRead(LT_L) > BLACK_TH; }
bool isFront()  { return analogRead(LT_F) > BLACK_TH; }
bool isRight()  { return analogRead(LT_R) > BLACK_TH; }
bool isDark()   { return analogRead(LIGHT_SENSOR) > DARK_TH; }

// -----------------------------------------------------
//  LFS decision logic (Section 1)
//  Left → Front → Right → U-turn
// -----------------------------------------------------
void LFS_update() {
  bool L = isLeft();
  bool F = isFront();
  bool R = isRight();
  bool D = isDark();

  if (D) {
    g_direction = DIR_ST;
    return;
  }

  // STRICT LFS priority
  if (L)        g_direction = DIR_LF;
  else if (F)   g_direction = DIR_FW;
  else if (R)   g_direction = DIR_RF;
  else          g_direction = DIR_TA;
}

void car_update() {
  switch(g_direction) {
    case DIR_FW: go_straight(); break;
    case DIR_LF: turn_left();   break;
    case DIR_RF: turn_right();  break;
    case DIR_TA: u_turn();      break;
    default:     stop_car();    break;
  }
}

void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(EN1, OUTPUT);
  pinMode(EN2, OUTPUT);
  pinMode(EN3, OUTPUT);
  pinMode(EN4, OUTPUT);

  pinMode(LT_L, INPUT);
  pinMode(LT_F, INPUT);
  pinMode(LT_R, INPUT);
  pinMode(LIGHT_SENSOR, INPUT);

  Serial.begin(9600);
  bt_serial.begin(9600);
  lcd.init(); 
  lcd.backlight();
}

//  Loop (Sense → Decide → Act)
// -----------------------------------------------------
void loop() {
  LFS_update();
  car_update();
}
