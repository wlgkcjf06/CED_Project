#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <string.h>

#define BLACK 1  // JC -> 'X'
#define WHITE 2  // Evil TA -> 'O'
#define EMPTY 0  // Empty -> '.'

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial bt_serial (8,9);

String command = "";
byte buffer[10];
int board [9] = {0,0,1,0,0,1,1,1,0};


void setup() {
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  bt_serial.begin(9600);
}

void Interpret_Command (String cmd) {
  String friendly;
  String hostile;
  if (cmd[0] == 'X') {
    cmd = cmd.substring(2);
    for (int i = 0 ; i < cmd.length() ; i++) {
      int pos = hostile[i] - '1';
      board[pos] = 2;
    }
  }
  else if (cmd[0] == 'Z') {
    for (int i = 0 ; i < 9 ; i++) board[i] = 0;
    cmd = cmd.substring(2);
    int index = cmd.indexOf(' ');
    hostile = cmd.substring(0, index);
    friendly = cmd.substring(index+1);
    Serial.print("Hostile:");
    Serial.println(hostile);
    Serial.print("Friendly:");
    Serial.println(friendly);
    for (int i = 0 ; i < hostile.length() ; i++) {
      int pos = hostile[i] - '1';
      board[pos] = 2;
    }
    for (int i = 0 ; i < friendly.length() ; i++) {
      int pos = hostile[i] - '1';
      board[pos] = 1;
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
    Serial.println(c);
    if (c == '\n') {
      command.trim();
      Serial.println(command);
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
  Serial.println(move+1);
}

void loop() {
  Receive_Board_Info();
  Display_Optimal_Move();
  delay(90000);
}
