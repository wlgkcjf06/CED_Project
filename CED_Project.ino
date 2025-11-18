#define CAR_DIR_RF 1
#define CAR_DIR_ST 2
#define CAR_DIR_LF 3
#define CAR_DIR_FW 4
#define CAR_DIR_RR 5

int g_direction = 0;
int speed = 100;

#define LT_MODULE_L A2
#define LT_MODULE_F A1
#define LT_MODULE_R A0

#define ENA 6
#define ENB 5
#define EN1 7
#define EN2 3
#define EN3 4
#define EN4 2

void setup() {
  // put your setup code here, to run once:
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(EN1, OUTPUT);
  pinMode(EN2, OUTPUT);
  pinMode(EN3, OUTPUT);
  pinMode(EN4, OUTPUT);

  pinMode(LT_MODULE_L, INPUT);
  pinMode(LT_MODULE_F, INPUT);
  pinMode(LT_MODULE_R, INPUT);

  Serial.begin(9600);
}

bool It_isLeft() {
  int ret = analogRead(LT_MODULE_L);
  //Serial.print("left: ");
  //Serial.println(ret);
  return(ret > 200) ? (true) : (false);
}

bool It_isFront() {
  int ret = analogRead(LT_MODULE_F);
  //Serial.print("front: ");
  //Serial.println(ret);
  return(ret > 200) ? (true) : (false);
}

bool It_isRight() {
  int ret = analogRead(LT_MODULE_R);
  //Serial.print("right: ");
  //Serial.println(ret);
  return(ret > 200) ? (true) : (false);
}

void lt_mode_update() {
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
  else {
    g_direction = CAR_DIR_RR
  }
}

void car_update() {
  // put your main code here, to run repeatedly:
  Serial.print("Car Update: ");
  Serial.println(g_direction);
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
    delay(200);
  }
  else if (g_direction == CAR_DIR_LF) {
    digitalWrite(EN1, LOW);
    digitalWrite(EN2, HIGH);
    digitalWrite(EN3, HIGH);
    digitalWrite(EN4, LOW);
    analogWrite(ENA, speed);
    analogWrite(ENB, speed);
    delay(200);
  }
  else if (g_direction == CAR_DIR_RR) {
    digitalWrite(EN1, HIGH);
    digitalWrite(EN2, LOW);
    digitalWrite(EN3, HIGH);
    digitalWrite(EN4, LOW);
  }
  else{
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
  }
}

void loop() {
  car_update();
  lt_mode_update();
}
