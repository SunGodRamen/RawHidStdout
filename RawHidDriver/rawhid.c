#include "rawhid.h"

/**
 * Opens a HID device based on vendor and product IDs.
 *
 * @param usage_info Pointer to a hid_usage_info struct containing device details.
 * @return A handle to the HID device, or NULL if an error occurs.
 */
hid_device* get_handle(hid_usage_info* usage_info) {
    // Check if the usage_info argument is NULL
    if (!usage_info) {
        write_log(LOGLEVEL_ERROR, "RAWHID - Usage info is NULL");
        return NULL; // Return NULL to indicate failure
    }

    write_log_format(LOGLEVEL_INFO, "RAWHID - Attempting to open HID device with Vendor ID: 0x%x, Product ID: 0x%x",
        usage_info->vendor_id, usage_info->product_id);
    // Open the HID device using vendor and product IDs
    hid_device* handle = hid_open(usage_info->vendor_id, usage_info->product_id, NULL);
    if (!handle) {
        write_log_format(LOGLEVEL_ERROR, "RAWHID - Failed to get handle for Vendor ID: 0x%x, Product ID: 0x%x",
            usage_info->vendor_id, usage_info->product_id);
        return NULL; // Return NULL to indicate failure
    }

    // Log success and return the handle
    write_log_format(LOGLEVEL_INFO, "RAWHID - Successfully got handle for Vendor ID: 0x%x, Product ID: 0x%x",
        usage_info->vendor_id, usage_info->product_id);
    return handle;
}

/**
 * Opens a HID device based on its usage path.
 *
 * @param usage_info Pointer to a hid_usage_info struct containing device details.
 * @param handle Double pointer to the handle where the HID device handle will be stored.
 */
void open_usage_path(hid_usage_info* usage_info, hid_device** handle) {
    // Check for invalid arguments
    if (!handle || !usage_info) {
        write_log(LOGLEVEL_ERROR, "RAWHID - Invalid arguments");
        return;
    }

    // Check if handle is NULL
    if (*handle == NULL) {
        write_log(LOGLEVEL_ERROR, "RAWHID - No handle to open");
        return;
    }

    write_log_format(LOGLEVEL_INFO, "RAWHID - Enumerating HID devices for Vendor ID: 0x%x, Product ID: 0x%x",
        usage_info->vendor_id, usage_info->product_id);
    // Enumerate HID devices using vendor and product IDs
    struct hid_device_info* enum_device_info = hid_enumerate(usage_info->vendor_id, usage_info->product_id);
    if (!enum_device_info) {
        write_log_format(LOGLEVEL_ERROR, "RAWHID - Failed to enumerate devices for Vendor ID: 0x%x, Product ID: 0x%x",
            usage_info->vendor_id, usage_info->product_id);
        return;
    }

    // Loop through the enumerated devices and open the one that matches the usage page and usage
    for (; enum_device_info != NULL; enum_device_info = enum_device_info->next) {
        if (enum_device_info->usage_page == usage_info->usage_page &&
            enum_device_info->usage == usage_info->usage) {

            // Open the device by its path
            *handle = hid_open_path(enum_device_info->path);
            if (*handle) {
                write_log_format(LOGLEVEL_INFO, "RAWHID - Successfully opened device with Usage Page: 0x%x, Usage: 0x%x",
                    usage_info->usage_page, usage_info->usage);
                break;
            }
            else {
                write_log_format(LOGLEVEL_ERROR, "RAWHID - Failed to open device with Usage Page: 0x%x, Usage: 0x%x",
                    usage_info->usage_page, usage_info->usage);
            }
        }
    }

    // Free the enumeration list
    hid_free_enumeration(enum_device_info);
}

/**
 * Writes a message to the given HID handle.
 *
 * @param handle The handle to the HID device.
 * @param message Pointer to the message buffer.
 * @param size The size of the message in bytes.
 * @return The number of bytes written, or -1 if an error occurs.
 */
int write_to_handle(hid_device** handle, unsigned char* message, size_t size) {
    // Check for invalid arguments
    if (*handle == NULL || !message) {
        write_log(LOGLEVEL_ERROR, "RAWHID - Invalid arguments");
        return -1; // Return -1 to indicate failure
    }

    // Log the writing action
    write_log_format(LOGLEVEL_DEBUG, "RAWHID - Attempting to write %d bytes to handle", size);

    // Write the message to the HID device
    int result = hid_write(handle, message, size);
    if (result < 0) {
        write_log(LOGLEVEL_ERROR, "RAWHID - Failed to write to handle");
        return result;
    }

    write_log(LOGLEVEL_DEBUG, "RAWHID - Wrote to handle", message);
    return result; // Return the number of bytes written or -1 if an error occurs
}
