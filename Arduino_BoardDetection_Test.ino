#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <string.h>

#define BLACK 1  // JC -> 'X'
#define WHITE 2  // Evil TA -> 'O'
#define EMPTY 0  // Empty -> '.'

#define CAR_DIR_RF 1
#define CAR_DIR_ST 2
#define CAR_DIR_LF 3
#define CAR_DIR_FW 4
#define CAR_DIR_TA 5
#define CAR_DIR_LC 6
#define CAR_DIR_RC 7
#define TURN_DELAY 150
#define BLACK_THRESHOLD 150
#define LIGHT_THRESHOLD 300
#define ULTRASONIC_TIMEOUT 5000

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

#define SPEED_HI_R 198
#define SPEED_LOW_R 110
#define SPEED_HI_L 180
#define SPEED_LOW_L 100

#define OFF_TIME_MS 50  
#define ON_TIME_MS 400

#define P3_MINIMUM_DELTA 300

bool phase_lock = false;
unsigned long phase_lock_start = 0;

unsigned long OffStart = 0;
bool OffStable = false;
unsigned long OnStart = 0;
bool OnStable = false;

int g_direction = 0;
int speed_l = 180;
int speed_r = 153;
int car_phase = 1;
bool car_stop = false;
String command = "";
byte buffer[10];
int board [9] = {0,};
int board_location = 0;
bool Detected_Left = false;
bool Detected_Right = false;
int p2i = 0;
int p3i = 0;
int p4i = 0;

SoftwareSerial bt_serial (8,9);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
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

  lcd.init();
  lcd.backlight();

  bt_serial.begin(9600);
  digitalWrite(LED, LOW);
}

bool It_isLeft() {
  int ret = analogRead(LT_MODULE_L);
  bt_serial.print("L=");
  bt_serial.print(ret);
  return(ret > BLACK_THRESHOLD) ? (true) : (false);
}

bool It_isFront() {
  int ret = analogRead(LT_MODULE_F);
  bt_serial.print(" F=");
  bt_serial.print(ret);
  return(ret > BLACK_THRESHOLD) ? (true) : (false);
}

bool It_isRight() {
  int ret = analogRead(LT_MODULE_R);
  bt_serial.print(" R=");
  bt_serial.println(ret);
  return(ret > BLACK_THRESHOLD) ? (true) : (false);
}

bool It_isDark() {
  int lum = analogRead(LIGHT_SENSOR);
  return(lum > LIGHT_THRESHOLD) ? (true) : (false);
}

long mstocm (long microseconds) {
  return microseconds / 29 / 2;
}

void ResetLineTimers() {
  OffStart = 0;
  OffStable = false;
  OnStart = 0;
  OnStable = false;
}

bool OffTimeDelay(bool signal) {
  unsigned long now = millis();

  if (!signal) {
    if (OffStart == 0) {
      OffStart = now;
      OffStable = false;
    }
    if (now - OffStart >= OFF_TIME_MS) {
      OffStable = true;
    }
  } 
  else {
    OffStart = 0;
    OffStable = false;
  }
  return OffStable;
}

bool OnTimeDelay(bool signal) {
  unsigned long now = millis();

  if(signal) {
    if (OnStart == 0) {
      OnStart = now;
      OnStable = true;
    }
    else if (now - OnStart <= ON_TIME_MS) {
      OnStable = false;
    }
    else {
      OnStart = 0;
      OnStable = true;
    }
  }
  else {
    OnStable = false;
  }
  return OnStable;
}

bool Is_LeftClose() {
  long duration, cm;
  digitalWrite(TRIG_L, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_L, LOW);

  duration = pulseIn(ECHO_L, HIGH, ULTRASONIC_TIMEOUT);
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

  duration = pulseIn(ECHO_R, HIGH, ULTRASONIC_TIMEOUT);
  cm = mstocm(duration);
  if (cm == 0) {
    cm = 999;
  }
  return(cm < 30) ? (true) : (false);
}

int Detect_Course() {
  //String output = "";
  bool ll = It_isLeft();
  bool ff = It_isFront();
  bool rr = It_isRight();
  //output = output + ll + "/" + ff + "/" + rr;
  //bt_serial.println(output);
  if (OnTimeDelay(ll && ff && rr)) {
    g_direction = CAR_DIR_FW;
    bt_serial.println("Intersection Detected");
    return 1;
  } 
  else if (!ll && !rr && OffTimeDelay(ff)){
    bt_serial.println("Reached DEAD END");
    return 2;
  }
  else if (ll && !ff && !rr){
    g_direction = CAR_DIR_LC;
    return 0;
  }
  else if (!ll && !ff && rr){
    g_direction = CAR_DIR_RC;
    return 0;
  }
  else {
    g_direction = CAR_DIR_FW;
    return 0;
  }
}

void Check_Phase_Lock() {
  unsigned long now = millis();
  if (now - phase_lock_start > P3_MINIMUM_DELTA) {
    phase_lock = false;
  }
}

void Update_Phase2 (){
  int co = Detect_Course();
  Check_Phase_Lock();
  if (co == 1 && (board_location == 0 || board_location == 1)){
    if(Detected_Left == true && board_location < 3) {
      board[8-board_location]+=1;
      bt_serial.println("Detected Left");
    }
    if(Detected_Right == true && board_location < 3) {
      board[5-board_location]+=1;
      bt_serial.println("Detected Right");
    }
    board_location += 1;
    if (board_location == 2) {
      phase_lock_start = millis();
      phase_lock = true;
    }
    Detected_Left = false;
    Detected_Right = false;
  }
  else if (co == 2 && board_location == 2 && !phase_lock) {
    if(Detected_Left == true && board_location < 3) {
      board[board_location]+=1;
      bt_serial.println("Detected Left");
    }
    bt_serial.println("Reached DEAD END");
    Detected_Left = false;
    Detected_Right = false;
    g_direction = CAR_DIR_TA;
    speed_r = SPEED_HI_R;
    speed_l = SPEED_HI_L;
    board_location = 0;
    ResetLineTimers();
    car_phase+=1;
  }
  else{
    bool lo = Is_LeftClose();
    bool ro = Is_RightClose();
    if (lo == true) {
      Detected_Left = true;
    }
    if (ro == true) {
      Detected_Right = true;
    }
  }
}

void Update_Phase3() {
  //String output = "";
  bool ll = It_isLeft();
  bool ff = It_isFront();
  bool rr = It_isRight();
  //output = output + ll + "/" + ff + "/" + rr;
  //bt_serial.println(output);
  if (ll) {
    g_direction = CAR_DIR_LF;    
  }
  else if (ff) {
    g_direction = CAR_DIR_FW;
  }
  else if (rr) {
    g_direction = CAR_DIR_RF;
  }
  else if (!ll && !rr && OffTimeDelay(ff)) {
    g_direction = CAR_DIR_TA;
    ResetLineTimers();
    car_phase+=1;
  }
  else {
    g_direction = CAR_DIR_TA;
  }
}

void Update_Phase4() {
  int co = Detect_Course();
  if (co == 1 && (board_location == 0 || board_location == 1)){
    if(Detected_Left == true && board_location < 3) {
      board[board_location]+=1;
      bt_serial.println("Detected Left");
    } 
    board_location += 1;
    Detected_Left = false;
  }
  else if (co == 2 && board_location == 2) {
    if(Detected_Left == true && board_location < 3) {
      board[board_location]+=1;
      bt_serial.println("Detected Left");
    }
    Detected_Left = false;
    g_direction = CAR_DIR_ST;
    board_location = 0;
    car_phase+=1;
  }
  else{
    bool lo = Is_LeftClose();
    if (lo == true) {
      Detected_Left = true;
    }
  }
}

void Display_Board() {
  String output = "";
  for (int i = 0; i < 9 ; i++) {
    if (board[i] == 1) {
      output += String(i+1);
      output += " ";
    }
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(output);
}


void car_update() {
  if(g_direction == CAR_DIR_FW){
    digitalWrite(EN1, LOW);
    digitalWrite(EN2, HIGH);
    digitalWrite(EN3, LOW);
    digitalWrite(EN4, HIGH);
    analogWrite(ENA, speed_r);
    analogWrite(ENB, speed_l);
    //bt_serial.println("Going Forward");
  }
  else if (g_direction == CAR_DIR_RF) {
    digitalWrite(EN1, HIGH);
    digitalWrite(EN2, LOW);
    digitalWrite(EN3, LOW);
    digitalWrite(EN4, HIGH);
    analogWrite(ENA, speed_r);
    analogWrite(ENB, speed_l);
    bt_serial.println("Turning Right");
    bool ff = It_isFront();
    while(!ff){
      digitalWrite(EN1, HIGH);
      digitalWrite(EN2, LOW);
      digitalWrite(EN3, LOW);
      digitalWrite(EN4, HIGH);
      analogWrite(ENA, speed_r);
      analogWrite(ENB, speed_l);
      ff = It_isFront();
    }
  }
  else if (g_direction == CAR_DIR_LF) {
    digitalWrite(EN1, LOW);
    digitalWrite(EN2, HIGH);
    digitalWrite(EN3, HIGH);
    digitalWrite(EN4, LOW);
    analogWrite(ENA, speed_r);
    analogWrite(ENB, speed_l);
    bt_serial.println("Turning Left");
    bool ff = It_isFront();
    while(!ff){
      digitalWrite(EN1, LOW);
      digitalWrite(EN2, HIGH);
      digitalWrite(EN3, HIGH);
      digitalWrite(EN4, LOW);
      analogWrite(ENA, speed_r);
      analogWrite(ENB, speed_l);
      ff = It_isFront();
    }
  }
  else if (g_direction == CAR_DIR_TA) {
    bool ll = It_isLeft();
    bool ff = It_isFront();
    bool rr = It_isRight();
    bt_serial.println("Turning Around");
    delay(TURN_DELAY);
    while(!ff){
      digitalWrite(EN1, LOW);
      digitalWrite(EN2, HIGH);
      digitalWrite(EN3, HIGH);
      digitalWrite(EN4, LOW);
      analogWrite(ENA, speed_r);
      analogWrite(ENB, speed_l);
      if(rr || ll) {
        break;
      }
      ll = It_isLeft();
      ff = It_isFront();
      rr = It_isRight();
    }    
  }
  else if (g_direction == CAR_DIR_LC) {
    digitalWrite(EN1, LOW);
    digitalWrite(EN2, HIGH);
    digitalWrite(EN3, HIGH);
    digitalWrite(EN4, LOW);
    analogWrite(ENA, speed_r);
    analogWrite(ENB, speed_l);
    //bt_serial.println("Correcting Left");
  }
  else if (g_direction == CAR_DIR_RC) {
    digitalWrite(EN1, HIGH);
    digitalWrite(EN2, LOW);
    digitalWrite(EN3, LOW);
    digitalWrite(EN4, HIGH);
    analogWrite(ENA, speed_r);
    analogWrite(ENB, speed_l);
    //bt_serial.println("Correcting Right");
  }
  else{
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
  }
}

void loop() {
  switch(car_phase){
    case 1:
      g_direction = CAR_DIR_ST;
      if (bt_serial.available()) {
        char data = (char) bt_serial.read();
        if (data != '\n') {
          command += data;
        }
        else {
          command.trim();
          if (command.equals("start")) {
            car_stop = false;
            digitalWrite(LED, LOW);
            car_phase +=1;
          }
          command = "";
        }
      }
      break;
    case 2:
      if(p2i == 0) {
        speed_r = SPEED_LOW_R;
        speed_l = SPEED_LOW_L;
        bt_serial.println("In Phase 2");
        p2i++;
      }
      Update_Phase2();
      /*if(p2i % 2 == 1) {
        g_direction = CAR_DIR_ST;
        delay(50);
      }*/
      break;
    case 3:
      if(p3i == 0) {
        bt_serial.println("In Phase 3");
        p3i++;
      }
      Update_Phase3();
      break;
    case 4:
      if(p4i == 0) {
        bt_serial.println("In Phase 4");
        p4i++;
      }
      /*
      if(p4i % 2 == 1) {
        g_direction = CAR_DIR_ST;
        delay(50);
      }*/
      Update_Phase4();
      break;
    case 5:
      Display_Board();
      break;
    default:
      g_direction = CAR_DIR_ST;
      break;
  }
  car_update();
}
