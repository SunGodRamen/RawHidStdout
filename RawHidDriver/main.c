#include <stdio.h>
#include <stdbool.h>
#include "tcp_client.h"
#include "logger.h"
#include "rawhid.h"
#include "windows.h"
#include "config.h"

#define PING_REQUEST 0x01
#define PONG_RESPONSE 0x02

#define PING_INTERVAL 5000 // Ping every 5 seconds
#define PING_TIMEOUT 1000  // Timeout after 1 second

#define RECONNECT_INTERVAL 60000

bool send_ping(hid_device* handle) {
    unsigned char ping_message[32] = { 0x00, 0x00, 0x00, 0x00 };  // Account for the ignored first byte
    ping_message[1] = 0x01;

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

// Global variable to control the main loop
volatile bool keepRunning = true;

// Control handler function
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
        // Handle the CTRL+C signal.
    case CTRL_C_EVENT:
        printf("Ctrl+C event\n");
        keepRunning = false; // Set the flag to false to exit the main loop
        return TRUE;

    default:
        return FALSE;
    }
}

int main() {
    
    // Register the control handler
    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
        printf("ERROR: Could not set control handler");
        return 1;
    }

    init_logger(LOG_FILE); // Initialize the logger
    set_log_level(LOGLEVEL_DEBUG); // Set the desired log level
    write_log(LOGLEVEL_DEBUG, "Logger initialized.");
    
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
        write_log(LOGLEVEL_ERROR, "Could not find the device.");
        close_logger(); // Clean up the logger
        return -1;
    }

    // Define server information
    tcp_socket_info server_info;
    server_info.ip = SERVER_IP;  // The IP address of the server, replace with actual
    server_info.port = SERVER_PORT;  // The port number of the server, replace with actual

    // Initialize TCP client and connect to the server
    SOCKET serverSocket = init_client(&server_info);
    if (serverSocket == INVALID_SOCKET) {
        hid_close(handle);
        hid_exit();
        write_log(LOGLEVEL_ERROR, "Failed to initialize TCP client.");
        close_logger();
        return -1;
    }

    if (handle) {
        // We have successfully connected to the device
        // Now we can start listening for messages
        hid_set_nonblocking(handle, true);
        DWORD last_ping_time = 0;
        while (keepRunning) {
            if (GetTickCount() - last_ping_time >= PING_INTERVAL) {
                if (send_ping(handle)) {
                    if (wait_for_pong(handle, PING_TIMEOUT)) {
                        // Pong received within the timeout, all is good
                        last_ping_time = GetTickCount();
                    }
                    else {
                        // No pong received, attempt to reconnect
                        write_log(LOGLEVEL_WARN, "Attempting to reconnect...");
                        handle = get_handle(&usage_info);
                        if (handle) {
                            // If we successfully got a handle, try to open the usage path
                            open_usage_path(&usage_info, &handle);
                        }
                        else {
                            // Handle error: could not find the device
                            write_log(LOGLEVEL_ERROR, "Could not find the device.");
                            close_logger(); // Clean up the logger
                            return -1;
                        }
                    }
                }
                last_ping_time = GetTickCount();
            }
            unsigned char buf[64]; // Buffer for incoming data
            char hexData[3 * 3 + 1]; // Each byte -> 2 hex chars, 3 bytes total, plus 1 for null terminator

            // Read data from the device
            int res = hid_read(handle, buf, sizeof(buf));
            if (res > 0) {
                // Convert the first three bytes of buf to a hex string
                snprintf(hexData, sizeof(hexData), "%02X %02X %02X", buf[0], buf[1], buf[2]);

                // Log the converted hex string
                write_log(LOGLEVEL_DEBUG, hexData);

                // Send the hex string over TCP
                int bytesSent = send_to_server(serverSocket, hexData, strlen(hexData));
                if (bytesSent < 0) {
                    // Handle error in sending
                    write_log(LOGLEVEL_ERROR, "Failed to send hex data to server.");
                    // Reconnection logic if necessary
                }
            }
            else if (res < 0) {
                // Handle error in reading from HID device
                write_log(LOGLEVEL_ERROR, "Error reading from device.");
                break;
            }

            Sleep(20); // Sleep to prevent a tight loop
        }
    }
    else {
        // Handle error: could not open usage path
        hid_close(handle);
        hid_exit();
        write_log(LOGLEVEL_ERROR, "Could not open the usage path.");
        close_logger();
        return -1;
    }

    // Clean up and close the device handle
    hid_close(handle);
    hid_exit();
    write_log(LOGLEVEL_INFO, "Application exiting due to Ctrl+C.");
    close_logger(); // Clean up the logger
    return 0;
}
