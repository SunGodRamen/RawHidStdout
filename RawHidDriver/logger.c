#include "logger.h"

/**
 * Constants for maximum log size and general buffer size for temporary string operations.
 */
#define MAX_LOG_SIZE 512
#define BUFFER_SIZE 4096

 /**
  * Internal variables to keep track of the log file and mutex.
  * Marked as 'static' to limit their scope to this file.
  */
static FILE* logFile = NULL;
static HANDLE logMutex = NULL;

// Internal variable for log level
static LogLevel currentLogLevel = LOGLEVEL_DEBUG;

/**
 * Internal utility function to write to the log file.
 *
 * @param message The message string to be logged.
 */
static void write_to_log_file(LogLevel level, const char* message);

/**
 * Set the logging level.
 *
 * @param level The logging level.
 */
void set_log_level(LogLevel level) {
    currentLogLevel = level;
}

/**
 * Initialize the logger.
 *
 * @param filePath The path of the file to be used for logging.
 */
void init_logger(char* filePath) {
    errno_t err = fopen_s(&logFile, filePath, "a");
    if (err != 0) {
        perror("Error opening file");
        exit(-1);
    }

    logMutex = CreateMutex(NULL, FALSE, NULL);
    if (logMutex == NULL) {
        fprintf(stderr, "Error: Unable to create mutex.\n");
        fclose(logFile);
        exit(-1);
    }
}

/**
 * Writes a simple log message with a specific logging level.
 *
 * @param level The logging level.
 * @param message The message string to be logged.
 */
void write_log(LogLevel level, const char* message) {
    write_to_log_file(level, message);
}

/**
 * Writes a formatted log message with a specific logging level.
 *
 * @param level The logging level.
 * @param format A format string for the log message.
 * @param ... Variable arguments for the format string.
 */
void write_log_format(LogLevel level, const char* format, ...) {
    char buffer[BUFFER_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    write_to_log_file(level, buffer);
}

/**
 * Converts a byte array to a hexadecimal string.
 *
 * @param data The byte array to convert.
 * @param data_len The length of the byte array.
 * @param out_str The output buffer for the hexadecimal string.
 * @param out_str_size The size of the output buffer.
 */
void bytes_to_hex_string(const unsigned char* data, size_t data_len, char* out_str, size_t out_str_size) {
    const char* hex_digits = "0123456789ABCDEF";
    size_t j = 0;
    for (size_t i = 0; i < data_len && j < out_str_size - 2; ++i) {
        out_str[j++] = hex_digits[(data[i] >> 4) & 0x0F];
        out_str[j++] = hex_digits[(data[i] & 0x0F)];
    }
    out_str[j] = '\0';  // Null-terminate the string
}

/**
 * Logs a byte array as a hexadecimal string.
 *
 * @param level The logging level.
 * @param data The byte array to log.
 * @param data_len The length of the byte array.
 */
void write_log_byte_array(LogLevel level, const unsigned char* data, size_t data_len) {
    char buffer[BUFFER_SIZE]; // Make sure BUFFER_SIZE is large enough to hold the hex string
    bytes_to_hex_string(data, data_len, buffer, sizeof(buffer));
    write_to_log_file(level, buffer);
}

/**
 * Logs an unsigned 64-bit integer in decimal format.
 *
 * @param level The logging level.
 * @param message A message string to prefix the log.
 * @param value The 64-bit unsigned integer to log.
 */
void write_log_uint64_dec(LogLevel level, const char* message, uint64_t value) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s: %llu", message, value);

    write_to_log_file(level, buffer);
}

/**
 * Logs an unsigned 64-bit integer in hexadecimal format.
 *
 * @param level The logging level.
 * @param message A message string to prefix the log.
 * @param value The 64-bit unsigned integer to log.
 */
void write_log_uint64_hex(LogLevel level, const char* message, uint64_t value) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s: 0x%llx", message, value);

    write_to_log_file(level, buffer);
}

/**
 * Logs an unsigned 64-bit integer in binary format.
 *
 * @param level The logging level.
 * @param message A message string to prefix the log.
 * @param value The 64-bit unsigned integer to log.
 */
void write_log_uint64_bin(LogLevel level, const char* message, uint64_t value) {
    char buffer[BUFFER_SIZE];
    char binaryStr[65];

    for (int i = 63; i >= 0; i--) {
        binaryStr[63 - i] = (value & (1ULL << i)) ? '1' : '0';
    }
    binaryStr[64] = '\0';

    snprintf(buffer, sizeof(buffer), "%s: %s", message, binaryStr);

    write_to_log_file(level, buffer);
}

/**
 * Internal utility function to write to log file.
 *
 * @param level The logging level.
 * @param level The logging level.
 * @param message The message string to be logged.
 */
static void write_to_log_file(LogLevel level, const char* message) {
    const char* levelStr = "";
    switch (level) {
    case LOGLEVEL_DEBUG:
        levelStr = "[DEBUG]";
        break;
    case LOGLEVEL_INFO:
        levelStr = "[INFO]";
        break;
    case LOGLEVEL_WARN:
        levelStr = "[WARN]";
        break;
    case LOGLEVEL_ERROR:
        levelStr = "[ERROR]";
        break;
    }

    if (level >= currentLogLevel) {
        return;
    }

    WaitForSingleObject(logMutex, INFINITE);

    // Write to the log file
    if (fprintf(logFile, "%s %s\n", levelStr, message) < 0) {
        fprintf(stderr, "Error: Unable to write to log file.\n");
    }

    fflush(logFile);
    ReleaseMutex(logMutex);
}

/**
 * Close and clean up the logger.
 */
void close_logger() {
    if (logFile) {
        fclose(logFile);
    }
    if (logMutex) {
        CloseHandle(logMutex);
    }
}
