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
#define TURN_DELAY 250
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
int speed_l = SPEED_LOW_L;
int speed_r = SPEED_LOW_R;
int car_phase = 1;
bool car_stop = false;
String command = "";
byte buffer[10];
int board [9] = {0};
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
  return(ret > BLACK_THRESHOLD) ? (true) : (false);
}

bool It_isFront() {
  int ret = analogRead(LT_MODULE_F);
  return(ret > BLACK_THRESHOLD) ? (true) : (false);
}

bool It_isRight() {
  int ret = analogRead(LT_MODULE_R);
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

  duration = pulseIn(ECHO_L, HIGH, 2500);
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

  duration = pulseIn(ECHO_R, HIGH, 2500);
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
        digitalWrite(LED, HIGH);
      }
      if (command.equals("start")) {
        car_stop = false;
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
  else if (OffTimeDelay(ff)){
    g_direction = CAR_DIR_TA;
  }
  else{
    g_direction = CAR_DIR_FW;
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

void Interpret_Command (String cmd) {
  String friendly;
  String hostile;
  if (cmd[0] == 'X') {
    cmd = cmd.substring(2);
    for (int i = 0 ; i < cmd.length() ; i++) {
      int pos = hostile[i] - '1';
      if (pos >=0 && pos < 9) board[pos] = 2;
    }
  }
  else if (cmd[0] == 'Z') {
    for (int i = 0 ; i < 9 ; i++) board[i] = 0;
    cmd = cmd.substring(2);
    int index = cmd.indexOf(' ');
    hostile = cmd.substring(0, index);
    friendly = cmd.substring(index+1);
    for (int i = 0 ; i < hostile.length() ; i++) {
      int pos = hostile[i] - '1';
      if (pos >=0 && pos < 9) board[pos] = 2;
    }
    for (int i = 0 ; i < friendly.length() ; i++) {
      int pos = friendly[i] - '1';
      if (pos >=0 && pos < 9) board[pos] = 1;
    }
  }
}

void Receive_Board_Info() {
  while(bt_serial.available() == 0) {
    delay(100);
  }
  while(bt_serial.available() > 0) {
    char c = (char) bt_serial.read();
    command += c;
    if (c == '\n') {
      command.trim();
      Interpret_Command(command);
      command = "";
    }
  }
}

// Helper: Check a specific line. Returns index of empty spot if it's a winning or blocking line .
int check_line(int board[], int player, int a, int b, int c) {
    int p_count = 0;
    int e_count = 0;
    int empty_index = -1;

    if (board[a] == player) p_count++; else if (board[a] == EMPTY) { e_count++; empty_index = a; }
    if (board[b] == player) p_count++; else if (board[b] == EMPTY) { e_count++; empty_index = b; }
    if (board[c] == player) p_count++; else if (board[c] == EMPTY) { e_count++; empty_index = c; }

    if (p_count == 2 && e_count == 1) return empty_index;
    return -1;
}

// Helper: Scan all 8 lines for a critical move (Win or Block)
int find_critical_move(int board[], int player) {
    const int lines[8][3] = {
        {0,1,2}, {3,4,5}, {6,7,8},
        {0,3,6}, {1,4,7}, {2,5,8},
        {0,4,8}, {2,4,6}
    };

    for (int i = 0; i < 8; i++) {
        int move = check_line(board, player, lines[i][0], lines[i][1], lines[i][2]);
        if (move != -1) return move;
    }
    return -1;
}

// New Helper: Count how many winning lines exist for a player
int count_threats(int board[], int player) {
    const int lines[8][3] = {
        {0,1,2}, {3,4,5}, {6,7,8},
        {0,3,6}, {1,4,7}, {2,5,8},
        {0,4,8}, {2,4,6}
    };
    int threats = 0;
    for (int i = 0; i < 8; i++) {
        if (check_line(board, player, lines[i][0], lines[i][1], lines[i][2]) != -1) {
            threats++;
        }
    }
    return threats;
}

// New Helper: Find a move that creates a Fork (2 threats at once)
int find_fork(int board[], int player) {
    for (int i = 0; i < 9; i++) {
        if (board[i] == EMPTY) {
            // Simulate placing the piece
            board[i] = player;
            // Check if this creates >= 2 winning threats
            if (count_threats(board, player) >= 2) {
                board[i] = EMPTY; // Reset
                return i;
            }
            board[i] = EMPTY; // Reset
        }
    }
    return -1;
}

// MAIN LOGIC FUNCTION
int get_optimal_move(int board[]) {
    int move;

    // 1. WIN
    move = find_critical_move(board, BLACK);
    if (move != -1) return move;

    // 2. BLOCK
    move = find_critical_move(board, WHITE);
    if (move != -1) return move;

    // 3. FORK (Attack!)
    move = find_fork(board, BLACK);
    if (move != -1) return move;

    // 4. CENTER
    if (board[4] == EMPTY) return 4;

    // 5. CORNER
    if (board[0] == EMPTY) return 0;
    if (board[2] == EMPTY) return 2;
    if (board[6] == EMPTY) return 6;
    if (board[8] == EMPTY) return 8;

    // 6. SIDE
    if (board[1] == EMPTY) return 1;
    if (board[3] == EMPTY) return 3;
    if (board[5] == EMPTY) return 5;
    if (board[7] == EMPTY) return 7;

    return -1;
}

void Display_Optimal_Move() {
  int move = get_optimal_move(board);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Optimal Move: ");
  lcd.setCursor(14, 0);
  lcd.print(move+1);
  car_phase+=1;
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
      analogWrite(ENA, SPEED_HI_R);
      analogWrite(ENB, SPEED_HI_R);
      ff = It_isFront();
    }
  }
  else if (g_direction == CAR_DIR_LF) {
    digitalWrite(EN1, LOW);
    digitalWrite(EN2, HIGH);
    digitalWrite(EN3, HIGH);
    digitalWrite(EN4, LOW);
    analogWrite(ENA, SPEED_HI_R);
    analogWrite(ENB, SPEED_HI_L);
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
      analogWrite(ENA, SPEED_HI_R);
      analogWrite(ENB, SPEED_HI_L);
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
      Update_Phase1();
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
