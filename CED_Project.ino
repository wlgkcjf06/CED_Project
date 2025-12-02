#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

#define CAR_DIR_RF 1
#define CAR_DIR_ST 2
#define CAR_DIR_LF 3
#define CAR_DIR_FW 4
#define CAR_DIR_TA 5
#define TURN_DELAY 4000
#define BLACK_THRESHOLD 150
#define LIGHT_THRESHOLD 300

int g_direction = 0;
int speed = 100;
int car_phase = 1;
bool car_stop = false;
String command = "";
byte buffer[10];
bool board[9] = {false};

#define LIGHT_SENSOR A3
#define LT_MODULE_L A2
#define LT_MODULE_F A1
#define LT_MODULE_R A0

#define ENA 6
#define ENB 5
#define EN1 7
#define EN2 3
#define EN3 4
#define EN4 2
#define LED 1
#define TRIG_L 10
#define ECHO_L 11
#define TRIG_R 12
#define ECHO_R 13

SoftwareSerial bt_serial (8,9);

void setup() {
  // put your setup code here, to run once:
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(EN1, OUTPUT);
  pinMode(EN2, OUTPUT);
  pinMode(EN3, OUTPUT);
  pinMode(EN4, OUTPUT);

  pinMode(LED, OUTPUT);
  pinMode(TRIG_L, OUTPUT);
  pinMode(TRIG_R, OUTPUT);
  pinMode(ECHO_L, INPUT);
  pinMode(ECHO_R, INPUT);

  pinMode(LT_MODULE_L, INPUT);
  pinMode(LT_MODULE_F, INPUT);
  pinMode(LT_MODULE_R, INPUT);
  pinMode(LIGHT_SENSOR, INPUT);

  Serial.begin(9600);
  bt_serial.begin(9600);
}

bool It_isLeft() {
  int ret = analogRead(LT_MODULE_L);
  //Serial.print("left: ");
  //Serial.println(ret);
  return(ret > BLACK_THRESHOLD) ? (true) : (false);
}

bool It_isFront() {
  int ret = analogRead(LT_MODULE_F);
  //Serial.print("front: ");
  //Serial.println(ret);
  return(ret > BLACK_THRESHOLD) ? (true) : (false);
}

bool It_isRight() {
  int ret = analogRead(LT_MODULE_R);
  //Serial.print("right: ");
  //Serial.println(ret);
  return(ret > BLACK_THRESHOLD) ? (true) : (false);
}

bool It_isDark() {
  int lum = analogRead(LIGHT_SENSOR);
  //Serial.println(lum);
  return(lum > LIGHT_THRESHOLD) ? (true) : (false);
}

long Measure_Distance(int Trig, int Echo) {
  long duration;
  digitalWrite(Trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trig, LOW);

  duration = pulseIn(Echo, HIGH);
  return duration / 29 / 2;
}

void LFS() {
  bool ll = It_isLeft();
  bool ff = It_isFront();
  bool rr = It_isRight();
  if (ll) {
    g_direction = CAR_DIR_LF;
  }
  else if (ff & !rr) {
    g_direction = CAR_DIR_FW;
  }
  else if (rr) {
    g_direction = CAR_DIR_RF;
  }
  else if (!ll & !ff & !rr) {
    g_direction = CAR_DIR_TA;
  }
}


int Detect_Line() {
  bool ll = It_isLeft();
  bool ff = It_isFront();
  bool rr = It_isRight();
  if(ll & ff & rr) {
    return 1;
  }
  else if (!ll & !ff & !rr) {
    return 2;
  }
  else {
    return 0;
  }
}

void Update_Phase1() {
  bool dar = It_isDark();
  if (bt_serial.available()) {
    char data = (char) bt_serial.read();
    if (data != '\n') {
      command += data;
    }
    else {
      command.trim();
      if (command.equals("stop")) {
        car_stop = true;
        Serial.println("Car Stopped");
        digitalWrite(LED, HIGH);
      }
      if (command.equals("start")) {
        car_stop = false;
        Serial.println("Car Restarted");
        digitalWrite(LED, LOW);
        //car_phase +=1;
      }
      command = "";
    }
    
  }
  if (car_stop) {
    g_direction = CAR_DIR_ST;
  }
  else if (dar) {
    g_direction = CAR_DIR_ST;
  }
  else {
    LFS();
  }
}

void Update_Phase2() {
  long dist_l, dist_r;
  int line_state = Detect_Line();
  dist_l = Measure_Distance(TRIG_L, ECHO_L);
  dist_r = Measure_Distance(TRIG_R, ECHO_R);
  if(line_state == 2) {
    g_direction = CAR_DIR_TA;
  }
  else{
    g_direction = CAR_DIR_FW;
  }
}


void car_update() {
  // put your main code here, to run repeatedly:
  //Serial.print("Car Update: ");
  //Serial.println(g_direction);
  if(g_direction == CAR_DIR_FW){
    digitalWrite(EN1, LOW);
    digitalWrite(EN2, HIGH);
    digitalWrite(EN3, LOW);
    digitalWrite(EN4, HIGH);
    analogWrite(ENA, speed);
    analogWrite(ENB, speed);
  }
  else if (g_direction == CAR_DIR_RF) {
    digitalWrite(EN1, HIGH);
    digitalWrite(EN2, LOW);
    digitalWrite(EN3, LOW);
    digitalWrite(EN4, HIGH);
    analogWrite(ENA, speed);
    analogWrite(ENB, speed); 
    delay(100);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
    delay(50);
  }
  else if (g_direction == CAR_DIR_LF) {
    digitalWrite(EN1, LOW);
    digitalWrite(EN2, HIGH);
    digitalWrite(EN3, HIGH);
    digitalWrite(EN4, LOW);
    analogWrite(ENA, speed);
    analogWrite(ENB, speed);
    delay(100);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
    delay(50);
  }
  else if (g_direction == CAR_DIR_TA) {
    digitalWrite(EN1, HIGH);
    digitalWrite(EN2, LOW);
    digitalWrite(EN3, LOW);
    digitalWrite(EN4, HIGH);
    analogWrite(ENA, speed);
    analogWrite(ENB, speed);
    delay(TURN_DELAY);
  }
  else{
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
  }
}

void loop() {
  car_update();
  if (car_phase == 1){
    Update_Phase1();
  }
  else if (car_phase == 2) {
    Update_Phase2();
  }
  Serial.println(g_direction);
}
