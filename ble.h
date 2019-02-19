#pragma once

#include <stdint.h>

#define BLE_ACCESS_ADDRESS 0x8E89BED6

typedef struct {
    uint8_t address[6];
} ble_device_address_t;