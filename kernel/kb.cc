#include "kb.h"

Queue<char, InterruptSafeLock> q{};

kb::kb(VGA* vga): vga(vga) {}

void kb::kbInit() {
    // legacy is already disabled?
    initializeController();
}

void kb::initializeController() {
    // disable interrupts:
    // cli();

    // disable devices
    while (inb(STATUS_REG) & 0x2);
    outb(CMD_REG, 0xAD);
    while (inb(STATUS_REG) & 0x2);
    outb(CMD_REG, 0xA7);

    Debug::printf("Disabled devices\n");

    // flushing the output buffer
    // do i need to wait before flushing?
    // if ((inb(STATUS_REG) & 0x1) == 0)
    //     while ((inb(STATUS_REG) & 0x1) == 0) {Debug::printf("waiting\n");}
    inb(DATA_PORT);

    Debug::printf("flushed output buffer\n");

    // set configuration byte to enable keyboard port.
    while (inb(STATUS_REG) & 0x2);
    outb(CMD_REG, 0x20); // tell device I want to read Controller config byte.
    while ((inb(STATUS_REG) & 0x1) == 0);
    uint8_t configByte = inb(DATA_PORT) | 0x37; // and it with what I need to be set

    Debug::printf("read config byte\n");

    while ((inb(STATUS_REG) & 0x1));
    // now we can write the config byte
    outb(CMD_REG, 0x60); // tell device I want to write config byte
    while (inb(STATUS_REG) & 0x2);
    outb(CMD_REG, configByte); // write config byte

    Debug::printf("wrote new config byte\n");

    // self-test, write 0xAA to 0x64
    while (inb(STATUS_REG) & 0x2);
    outb(CMD_REG, 0xAA); // Self test to 8042 Command register
    while ((inb(STATUS_REG) & 0x1) == 0);
    ASSERT(inb(DATA_PORT) == 0x55); // read response from controller

    Debug::printf("passed self test\n");

    // enable device
    while (inb(STATUS_REG) & 0x2);
    outb(CMD_REG, 0xAE); // enable first port?
    
    Debug::printf("enabled first port\n");

    // while (inb(STATUS_REG) & 0x2);
    // // reset device
    // outb(CMD_REG, 0xFF);
    // while ((inb(STATUS_REG) & 0x1) == 0);
    // ASSERT(inb(DATA_PORT) == 0xFA);

    // Debug::printf("reset device\n");

    // detect device types?

    // // disable scanning
    // while (inb(STATUS_REG) & 0x2);
    // outb(CMD_REG, 0xF5); // disable scanning
    // Debug::printf("sent disable scan command\n");
    // while ((inb(STATUS_REG) & 0x1) == 0);
    // ASSERT(inb(DATA_PORT) == 0xFA);
    
    // Debug::printf("disable scanning\n");

    // // // identify command
    // if (inb(STATUS_REG) & 0x2) while (inb(STATUS_REG) & 0x2);
    // outb(CMD_REG, 0xF2); // identify command
    // Debug::printf("sent identify command\n");
    // while ((inb(STATUS_REG) & 0x1) == 0);
    // int byte1 = inb(DATA_PORT);
    // Debug::printf("this is byte 1: %x\n", byte1);
    // while ((inb(STATUS_REG) & 0x1) == 0);
    // int byte2 = inb(DATA_PORT);
    // Debug::printf("this is byte 2: %x\n", byte2);




    // start polling/interrupts
    // int x = 0;
    // while (1) {
    //     while (inb(STATUS_REG & 0x1)) {Debug::printf("waiting");}
    //     // Debug::printf("got a character maybe? %d\n", inb(DATA_PORT));
    //     // vga->putPixel(10 + x, 10 + x, DARK_GRAY);
    //     x++;
    // }

    while (inb(STATUS_REG & 0x1)) {
        Debug::printf("waiting");
    }
    int val = inb(DATA_PORT);
    Debug::printf("Character? %x\n", val);
    vga->putPixel(10, 10, 0);


    // convert scan code to ascii, find table online.
    // send ascii to wherever i need it to go.    
}
















































// bool kb::detect_usb_controller() {
//     for (int bus = 0; bus < 256; bus++) {
//         for (int device = 0; device < 32; device++) {
//             for (int function = 0; function < 8; function++) {
//                 uint32_t config = readPCISpace(bus, device, function, 0);
//                 uint16_t vendor_id = config & 0xFFFF;
//                 uint16_t device_id = (config >> 16) & 0xFFFF;

//                 if (vendor_id == 0xFFFF) {
//                     continue; // No device present
//                 }

//                 if (is_usb_controller(vendor_id, device_id)) {
//                     Debug::printf("Found USB controller at Bus %d, Device %d, Function %d\n", bus, device, function);
//                 }
//             }
//         }
//     }
//     Debug::printf(":(sad\n");
//     // No USB controller found
//     return false;
// }

// uint32_t kb::readPCISpace(uint8_t bus, uint8_t device, uint8_t func, uint8_t reg) {
//     uint32_t addr = (1 << 31) | (bus << 16) | (device << 11) | (func << 8) | (reg & 0xFC);
//     outl(0xCF8, addr);
//     return inl(0xCFC);
// }

// bool kb::is_usb_controller(uint16_t vendor_id, uint16_t device_id) {
//     if (vendor_id == 0x8086) {
//         if (device_id == 0x7020 || device_id == 0x24CD) { // UHCI or EHCI
//             return true;
//         }
//     } else if (vendor_id == 0x1b36) {
//         if (device_id == 0x000d) { // QEMU XHCI
//             return true;
//         }
//     }
//     return false;
// }

// // Initialize the USB controller
// void kb::init_usb_controller() {
//     uint32_t config = readPCISpace(0, 4, 0, 0);
//     uint16_t vendor_id = config & 0xFFFF;
//     uint16_t device_id = (config >> 16) & 0xFFFF;
//     // Check if the first PCI device on the first bus is a USB controller
//     if (!is_usb_controller(vendor_id, device_id)) {
//         // Error: no USB controller found
//         Debug::printf("why am I here\n");
//         return;
//     }
//     Debug::printf("Controller found\n");
    
//     // Read the command register and set the appropriate bits
//     // uint16_t cmd_reg = pci_config_read(USB_BUS, USB_DEVICE, USB_FUNCTION, USB_COMMAND_REG);
//     // cmd_reg |= USB_CMD_IO_SPACE | USB_CMD_BUS_MASTER | USB

// }