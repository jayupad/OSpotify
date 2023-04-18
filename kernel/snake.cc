#include "snake.h"

void Snake::run() {
    homeScreen();
    for (int i = 0; i < 100; i++);
    startGameScreen();
    while (!done) {
        updateBoard();
    }
    endScreen();
};

void Snake::updateBoard() {
    // if (head->x == 19 || head->x == 0 || head->y = 0 || head->y == 9) {
    //     // wall colliision
    //     done = true;
    // }
}


void Snake::homeScreen() {
    myVGA->initializeScreen(BLACK);
    const char* starting_message = "Press <Space Bar> to play";
    myVGA->drawString(80, 100, starting_message, GREEN);
};

void Snake::endScreen() {
    myVGA->initializeScreen(BLACK);
    const char* starting_message = "Game Over. Score: " + (char) score;
    myVGA->drawString(80, 100, starting_message, GREEN);
};

void Snake::startGameScreen() {
    myVGA->initializeScreen(BLACK);
    myVGA->drawRectangle(0, 0, 320, 20, 63, true);
    myVGA->drawRectangle(0, 0, 20, 200, 63, true);
    myVGA->drawRectangle(300, 0, 320, 200, 63, true);
    myVGA->drawRectangle(0, 180, 320, 200, 63, true);
    myVGA->drawRectangle(head->x + 1, head->y + 1, head->x + 20 - 1, head->y + 20 - 1, GREEN, true);
    body->add(head);
};

void Snake::generateFood() {
    int row = head->x;
    int col = head->y;
    while (board[row][col] != 0) {
        int x = rand(head->x, head->y);
        row = x % 18;
        col = x % 8;
    }
    myVGA->drawRectangle(row * 20 + 1, col * 20 + 1, row * 20 + 19, col * 20 + 19, RED, 1);   
    food->x = row;
    food->y = col;
}

int Snake::rand(int x, int y) {
    int seed = x * y + MAGIC_NUM;
    seed = (seed << 13) ^ seed;
    return (seed * (seed * seed * 15731 + 789221) + 1376312589) & 0x7FFFFFFF;
}