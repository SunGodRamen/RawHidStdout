#include <stdio.h>
#include <stdbool.h>
#include "logger.h"
#include "rawhid.h"
#include "windows.h"
#include "config.h"

#define PING_REQUEST 0x01
#define PONG_RESPONSE 0x02
#define PING_INTERVAL 5000 // Ping every 5 seconds
#define PING_TIMEOUT 1000  // Timeout after 1 second

bool send_ping(hid_device* handle) {
    unsigned char ping_message[32] = { 0 };
    ping_message[0] = PING_REQUEST;

    int res = hid_write(handle, ping_message, sizeof(ping_message));
    if (res < 0) {
        write_log(LOGLEVEL_ERROR, "Failed to send ping.");
        return false;
    }
    return true;
}

bool wait_for_pong(hid_device* handle, unsigned int timeout) {
    DWORD start_time = GetTickCount();
    unsigned char buf[64]; // Buffer for incoming data

    while (GetTickCount() - start_time < timeout) {
        int res = hid_read_timeout(handle, buf, sizeof(buf), 100);
        if (res > 0) {
            if (buf[0] == PONG_RESPONSE) {
                return true; // Pong received
            }
            // If it's not a pong, assume it's a message from the keyboard
        }
        else if (res < 0) {
            write_log(LOGLEVEL_ERROR, "Error reading from device.");
            return false;
        }
    }
    return false; // Timeout reached, no pong received
}

int main() {
    init_logger(LOG_FILE); // Initialize the logger
    set_log_level(LOGLEVEL_DEBUG); // Set the desired log level
    // Define the usage information for the QMK keyboard
    hid_usage_info usage_info;
    usage_info.vendor_id = VENDOR_ID; // Example Vendor ID, replace with actual
    usage_info.product_id = PRODUCT_ID; // Example Product ID, replace with actual
    usage_info.usage_page = TARGET_USAGE_PAGE; // Example Usage Page for raw HID, replace with actual
    usage_info.usage = TARGET_USAGE; // Example Usage for raw HID, replace with actual

    // Attempt to get a handle to the HID device
    hid_device* handle = get_handle(&usage_info);
    if (handle) {
        // If we successfully got a handle, try to open the usage path
        open_usage_path(&usage_info, &handle);
    }
    else {
        // Handle error: could not find the device
        fprintf(stderr, "Could not find the device.\n");
        return -1;
    }

    if (handle) {
        // We have successfully connected to the device
        // Now we can start listening for messages
        DWORD last_ping_time = 0;
        while (1) {
            if (GetTickCount() - last_ping_time >= PING_INTERVAL) {
                if (send_ping(handle)) {
                    if (wait_for_pong(handle, PING_TIMEOUT)) {
                        // Pong received within the timeout, all is good
                        last_ping_time = GetTickCount();
                    }
                    else {
                        // No pong received, attempt to reconnect
                        write_log(LOGLEVEL_WARN, "Attempting to reconnect...");
                        // Implement reconnection logic here...
                        // ...
                    }
                }
                last_ping_time = GetTickCount();
            }
            unsigned char buf[64]; // Buffer for incoming data
            // Read data from the device
            int res = hid_read(handle, buf, sizeof(buf));
            if (res > 0) {
                write_log_byte_array(LOGLEVEL_DEBUG, buf, res);
                // Process the incoming data
                for (int i = 0; i < res; ++i) {
                    printf("%02hhx ", buf[i]); // Print each byte as a 2-digit hexadecimal number
                }
                printf("\n"); // End of data line
                fflush(stdout); // Ensure the data is sent to stdout immediately
            }
            else if (res < 0) {
                // Handle error
                write_log(LOGLEVEL_ERROR, "Error reading from device.");
                break;
            }
            Sleep(10);
        }
    }
    else {
        // Handle error: could not open usage path
        write_log(LOGLEVEL_ERROR, "Could not open the usage path.");
        return -1;
    }

    // Clean up and close the device handle
    hid_close(handle);
    hid_exit();
    close_logger(); // Clean up the logger
    return 0;
}
