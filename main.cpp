#include <iostream>
#include <fstream>
#include <string>

using namespace std;

// ==============================================================================
// PART 1: LOGIC ENGINE (Copy this to Arduino)
// ==============================================================================

#define BLACK 1  // JC -> 'X'
#define WHITE 2  // Evil TA -> 'O'
#define EMPTY 0  // Empty -> '.'

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

// ==============================================================================
// PART 2: VISUAL TEST RUNNER (PC Only, delete the following before uploading to Arduino)
// ==============================================================================

void print_visual_board(int b[]) {
    cout << "   -------------" << endl;
    for (int i = 0; i < 9; i++) {
        char c = '.';
        if (b[i] == BLACK) c = 'X';
        if (b[i] == WHITE) c = 'O';

        if (i % 3 == 0) cout << "   | ";
        cout << c << " | ";
        if ((i + 1) % 3 == 0) cout << endl << "   -------------" << endl;
    }
}

int main() {
    ifstream file("tests.txt");
    if (!file.is_open()) {
        cerr << "ERROR: Could not open 'tests.txt'." << endl;
        cin.get();
        return 1;
    }

    int board[9];
    int expected;
    int test_case_num = 1;
    int passed_count = 0;
    int total_count = 0;

    cout << "\n=== TIC TAC TOE TESTER ===\n" << endl;

    while (file >> board[0] >> board[1] >> board[2]
        >> board[3] >> board[4] >> board[5]
        >> board[6] >> board[7] >> board[8]
        >> expected) {

        total_count++;
        int actual = get_optimal_move(board);

        cout << "Test Case #" << test_case_num++ << ":" << endl;
        print_visual_board(board);

        if (actual == expected) {
            cout << "   RESULT: PASS (Move: " << actual << ")" << endl;
            passed_count++;
        }
        else {
            cout << "   RESULT: *** FAIL ***" << endl;
            cout << "   (Expected: " << expected << ", Got: " << actual << ")" << endl;
           
        }
        cout << endl;
    }
    file.close();
    cout << "------------------------------------" << endl;
    cout << "SUMMARY: " << passed_count << " / " << total_count << " Tests Passed." << endl;
    cin.get();
    return 0;
}