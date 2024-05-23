#ifndef PROTOCOLO_H

#define PROTOCOLO_H
#define BYTE unsigned char
#define LARGO_DATA 15
#define TX_PIN 27
#define RX_PIN 22
#define DELAY_PIN_E 0
#define DELAY_PIN_R 23

struct Protocolo{
    BYTE CMD; // 4 BITS
    BYTE LNG; // 4 BITS
    BYTE DATA[LARGO_DATA];
    BYTE FCS; // 1 BYTE
    BYTE FRAMES[LARGO_DATA+2];
};

#endif
