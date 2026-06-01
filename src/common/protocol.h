#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/* Packet framing definitions */
#define RF_PREAMBLE  0xAA
#define RF_TRAINING  0x55
#define RF_ADDRESS   0x42

/* Nominal pulse-width durations in microseconds */
#define T_SHORT_US   500
#define T_LONG_US    1000

/* Receiver decoding timing tolerances (in microseconds) */
#define LOW_MIN_US        300
#define LOW_MAX_US        700

#define HIGH_0_MIN_US     350
#define HIGH_0_MAX_US     650

#define HIGH_1_MIN_US     800
#define HIGH_1_MAX_US     1200

#endif /* PROTOCOL_H */
