#ifndef PORT_H
#define PORT_H

#include "debug.h"
#include "machine.h"

class Port {
    
    public:
        uint32_t index_port = 0;
        uint32_t read_port = 0;
        uint32_t write_port = 0;

        Port();

        void writeWithIndex(int index, int value);

        int readWithIndex(int index);

        void write(int value);

        int read();
};

#endif
