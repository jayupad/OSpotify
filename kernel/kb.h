#ifndef KB_H
#define KB_H

#include "atomic.h"
#include "queue.h"
#include "machine.h"
#include "debug.h"
#include "vga.h"

// 0x60	Read/Write	Data Port
// 0x64	Read	Status Register
// 0x64	Write	Command Register

#define DATA_PORT 0x60
#define STATUS_REG 0x64
#define CMD_REG 0x64

class kb {
    public:
    VGA* vga;
    const uint8_t USB_BUS = 0;
    const uint8_t USB_DEVICE = 4;
    const uint8_t USB_FUNCTION = 0;
    Queue<char,InterruptSafeLock> q;

    kb(VGA* vga);

    void kbInit();

    void initializeController();

};

#endif