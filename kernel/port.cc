#include "port.h"

Port::Port() {
    
}

void Port::writeWithIndex(int index, int value) {
    outb(index_port, index);
    outb(write_port, value);
}

int Port::readWithIndex(int index) {
    // index port gives us register
    // value port has data that we put into index port
    outb(index_port, index);
    int val = inb(read_port);
    return val;
}

void Port::write(int val) {
    outb(write_port, val);
}

int Port::read() {
    int val = inb(read_port);
    return val;
}