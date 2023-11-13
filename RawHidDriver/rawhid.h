#pragma once

#include <hidapi.h>
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <synchapi.h>
#include <time.h>
#include "logger.h"

// Structure to hold information required for HID device usage.
typedef struct {
    uint16_t vendor_id;   // Vendor ID of the device
    uint16_t product_id;  // Product ID of the device
    uint16_t usage_page;  // Usage page
    uint8_t usage;        // Usage ID
} hid_usage_info;

// Function prototypes
hid_device* get_handle(struct hid_usage_info* device_info);
void open_usage_path(struct hid_usage_info* device_info, hid_device** handle);
int write_to_handle(hid_device** handle, unsigned char* message, size_t size);
