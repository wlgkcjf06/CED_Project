#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

#define CAR_DIR_RF 1
#define CAR_DIR_ST 2
#define CAR_DIR_LF 3
#define CAR_DIR_FW 4
#define CAR_DIR_TA 5
#define TURN_DELAY 100
#define BLACK_THRESHOLD 150
#define LIGHT_THRESHOLD 300

int g_direction = 0;
int speed = 170;
int car_phase = 1;
bool car_stop = false;
String command = "";
byte buffer[10];
int board [9] = {0,};
int board_location = 0;

#define LIGHT_SENSOR A3
#define LT_MODULE_L A2
#define LT_MODULE_F A1
#define LT_MODULE_R A0
#define TRIG_L 10
#define TRIG_R 12
#define ECHO_L 11
#define ECHO_R 13
#define LED 1

#define ENA 6
#define ENB 5
#define EN1 7
#define EN2 3
#define EN3 4
#define EN4 2

SoftwareSerial bt_serial (8,9);

void setup() {
  // put your setup code here, to run once:
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(EN1, OUTPUT);
  pinMode(EN2, OUTPUT);
  pinMode(EN3, OUTPUT);
  pinMode(EN4, OUTPUT);
  pinMode(TRIG_L, OUTPUT);
  pinMode(TRIG_R, OUTPUT);
  pinMode(LED, OUTPUT);

  pinMode(LT_MODULE_L, INPUT);
  pinMode(LT_MODULE_F, INPUT);
  pinMode(LT_MODULE_R, INPUT);
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(ECHO_L, INPUT);
  pinMode(ECHO_R, INPUT);

  Serial.begin(9600);
  bt_serial.begin(9600);
  digitalWrite(LED, LOW);
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

long mstocm (long microseconds) {
  return microseconds / 29 / 2;
}

bool Is_LeftClose() {
  long duration, cm;
  digitalWrite(TRIG_L, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_L, LOW);

  duration = pulseIn(ECHO_L, HIGH);
  cm = mstocm(duration);
  if (cm == 0) {
    cm = 999;
  }
  return(cm < 30) ? (true) : (false);
}

bool Is_RightClose() {
  long duration, cm;
  digitalWrite(TRIG_R, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_R, LOW);

  duration = pulseIn(ECHO_R, HIGH);
  cm = mstocm(duration);
  if (cm == 0) {
    cm = 999;
  }
  Serial.print("Right:");
  Serial.println(cm);
  return(cm < 30) ? (true) : (false);
}

int Detect_Course() {
  bool ll = It_isLeft();
  bool ff = It_isFront();
  bool rr = It_isRight();
  if (ll & ff & rr) {
    return 1;
  } 
  else if (!ll & !ff & !rr){
    return 2;
  }
  else {
    return 0;
  }
}

void Update_Phase1() {
  bool ll = It_isLeft();
  bool ff = It_isFront();
  bool rr = It_isRight();
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
        car_phase +=1;
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
  else if (ll) {
    g_direction = CAR_DIR_LF;
  }
  else if (ff) {
    g_direction = CAR_DIR_FW;
  }
  else if (rr) {
    g_direction = CAR_DIR_RF;
  }
  else {
    g_direction = CAR_DIR_TA;
  }
}

void Update_Phase2 (){
  int co = Detect_Course();
  bool Detected_Left = false;
  bool Detected_Right = false;
  if(co == 0) {
    bool lo = Is_LeftClose();
    bool ro = Is_RightClose();
    g_direction = CAR_DIR_FW;
    if (lo == true) {
      Detected_Left = true;
    }
    if (ro == true) {
      Detected_Right = true;
    }
  }
  else{
    if(Detected_Left == true) {
      board[8-board_location]+=1;
    }
    if(Detected_Right == true) {
      board[5-board_location]+=1;
    }
    board_location +=1;
    if(co == 2) {
      g_direction = CAR_DIR_TA;
      car_phase+=1;
      board_location = 0;
    }
    Detected_Left, Detected_Right = false, false;
  }
}

void Update_Phase3() {
  bool ll = It_isLeft();
  bool ff = It_isFront();
  bool rr = It_isRight();
  if (ll) {
    g_direction = CAR_DIR_LF;
  }
  else if (ff) {
    g_direction = CAR_DIR_FW;
  }
  else if (rr) {
    g_direction = CAR_DIR_RF;
  }
  else {
    g_direction = CAR_DIR_TA;
    car_phase+=1;
  }
}

void Update_Phase4() {
  int co = Detect_Course();
  bool Detected_Left = false;
  if(co == 0) {
    bool lo = Is_LeftClose();
    g_direction = CAR_DIR_FW;
    if (lo == true) {
      Detected_Left = true;
    }
  }
  else{
    if(Detected_Left == true) {
      board[board_location]+=1;
    }
    board_location +=1;
    if(co == 2) {
      g_direction = CAR_DIR_ST;
    }
    Detected_Left = false;
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
    delay(TURN_DELAY);
    bool ff = It_isFront();
    while(!ff){
      digitalWrite(EN1, HIGH);
      digitalWrite(EN2, LOW);
      digitalWrite(EN3, LOW);
      digitalWrite(EN4, HIGH);
      analogWrite(ENA, speed);
      analogWrite(ENB, speed);
      ff = It_isFront();
    }
  }
  else if (g_direction == CAR_DIR_LF) {
    digitalWrite(EN1, LOW);
    digitalWrite(EN2, HIGH);
    digitalWrite(EN3, HIGH);
    digitalWrite(EN4, LOW);
    analogWrite(ENA, speed);
    analogWrite(ENB, speed);
    delay(TURN_DELAY);
    bool ff = It_isFront();
    bool ll = It_isLeft();
    bool rr = It_isRight();
    while(!ff){
      digitalWrite(EN1, LOW);
      digitalWrite(EN2, HIGH);
      digitalWrite(EN3, HIGH);
      digitalWrite(EN4, LOW);
      analogWrite(ENA, speed);
      analogWrite(ENB, speed);
      ff = It_isFront();
      ll = It_isLeft();
      rr = It_isRight();
      if (ll || rr) {
        break;
      }
    }
  }
  else if (g_direction == CAR_DIR_TA) {
    bool ff = It_isFront();
    while(!ff){
      digitalWrite(EN1, LOW);
      digitalWrite(EN2, HIGH);
      digitalWrite(EN3, HIGH);
      digitalWrite(EN4, LOW);
      analogWrite(ENA, speed);
      analogWrite(ENB, speed);
      ff = It_isFront();
    }    
  }
  else{
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
  }
}

void loop() {
  car_update();
  switch(car_phase){
    case 1:
      Update_Phase1();
      break;
    case 2:
      Update_Phase2();
      break;
    case 3:
      Update_Phase3();
      break;
    case 4:
      Update_Phase4();
      break;
    case 5:
      //Insert Tic-Tac-Toe Code Here
      break;
    default:
      g_direction = CAR_DIR_ST;
      break;
  }
}
