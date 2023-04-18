#ifndef SNAKE_H
#define SNAKE_H

#include "vga.h"
#include "queue.h"

const int MAGIC_NUM = 21;

struct Pos {
    uint32_t x;
    uint32_t y;
    Pos* next;

    Pos(uint32_t x_val, uint32_t y_val) : x(x_val), y(y_val) {}
};

class Snake {
    char GREEN;
    char BLACK;
    char RED;
    int* currentDir;
    uint8_t board[20][10]; // 0 for empty, 1 for snake, 2 for food
    Pos* head;
    Pos* food;
    int score;
    VGA* myVGA;
    Queue<Pos, NoLock>* body;
    bool done;
public:
    Snake(VGA* vga) : myVGA(vga) {
        GREEN = myVGA->getColor(0x00, 0xFF, 0x55);
        BLACK = 0;
        RED = myVGA->getColor(0xFF, 0x00, 0x00);
        score = 0;
        currentDir = new int[2];
        currentDir[0] = 1;
        currentDir[1] = 0;
        head = new Pos(10, 5);
        body = new Queue<Pos, NoLock>();
        for (int i = 0; i < 20; i ++) {
            board[i][0] = 1;
            board[i][9] = 1;
        }
        for (int i = 0; i < 10; i ++) {
            board[0][i] = 1;
            board[19][i] = 1;
        }
    };
    
    void run();
    void homeScreen();
    void endScreen();
    void updateBoard();
    void startGameScreen();
    void generateFood();
    void moveSnake();
    void changeDirection();
    int rand(int x, int y);
};

#endif