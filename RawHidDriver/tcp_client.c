#include "tcp_client.h"

/**
 * Initializes the TCP client and connects to the server.
 *
 * @param server_info Pointer to a tcp_socket_info struct containing server details.
 * @return A valid socket to the server, or INVALID_SOCKET on failure.
 */
SOCKET init_client(tcp_socket_info* server_info) {
    write_log(LOGLEVEL_DEBUG, "TCP Client - Entering init_client");

    WSADATA wsaData;  // WinSock Data

    // Initialize WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        write_log_format(LOGLEVEL_ERROR, "TCP Client - Failed to initialize WinSock. Error Code: %d", WSAGetLastError());
        return INVALID_SOCKET;
    }

    // Check for null server info
    if (!server_info) {
        write_log(LOGLEVEL_ERROR, "TCP Client - Server information is NULL");
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Create the client socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        write_log_format(LOGLEVEL_ERROR, "TCP Client - Failed to create socket. Error Code: %d; Server IP: %s, Port: %d",
            WSAGetLastError(), server_info->ip, server_info->port);
        WSACleanup();
        return INVALID_SOCKET;
    }

    struct sockaddr_in serverAddr;  // Server address
    serverAddr.sin_family = AF_INET;

    // Convert IP address from string to binary
    if (inet_pton(AF_INET, server_info->ip, &(serverAddr.sin_addr)) <= 0) {
        write_log_format(LOGLEVEL_ERROR, "TCP Client - Invalid IP address or error in inet_pton. Error Code: %d; Server IP: %s, Port: %d",
            WSAGetLastError(), server_info->ip, server_info->port);
        cleanup_client(clientSocket);
        return INVALID_SOCKET;
    }

    // Set server port
    serverAddr.sin_port = htons(server_info->port);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        write_log_format(LOGLEVEL_ERROR, "TCP Client - Connect failed. Error Code: %d; Server IP: %s, Port: %d",
            WSAGetLastError(), server_info->ip, server_info->port);
        cleanup_client(clientSocket);
        return INVALID_SOCKET;
    }

    write_log(LOGLEVEL_INFO, "TCP Client - Successfully connected to the server");

    return clientSocket;  // Return the connected socket
}

/**
 * Reads a 64-bit (8 bytes) message from the server.
 *
 * @param serverSocket The server socket to read the message from.
 * @param buffer The buffer to store the message.
 * @return The number of bytes read, or -1 on error.
 */
int read_message_from_server(SOCKET serverSocket, char* buffer) {
    write_log(LOGLEVEL_DEBUG, "TCP Client - Entering read_message_from_server");
    // Check for null buffer
    if (!buffer) {
        write_log(LOGLEVEL_ERROR, "TCP Client - Buffer is NULL");
        return -1;
    }

    int totalBytesRead = 0;  // Track the total number of bytes read
    int bytesRead = 0;       // Bytes read in a single recv call

    // Loop until the entire message has been read
    while (totalBytesRead < MESSAGE_SIZE_BYTES) {
        bytesRead = recv(serverSocket, buffer + totalBytesRead, MESSAGE_SIZE_BYTES - totalBytesRead, 0);

        // Check for socket errors
        if (bytesRead == SOCKET_ERROR && WSAGetLastError() != 10053) {
            write_log_format(LOGLEVEL_ERROR, "TCP Client - Error occurred while reading from socket. Error Code: %d", WSAGetLastError());
            return -1;
        }

        // Check for server disconnection
        if (bytesRead == 0) {
            write_log(LOGLEVEL_ERROR, "TCP Client - Server disconnected before sending full message.");
            return totalBytesRead;
        }

        totalBytesRead += bytesRead;  // Update the total bytes read
    }

    write_log_format(LOGLEVEL_DEBUG, "TCP Client - Read %d bytes from server", totalBytesRead);
    write_log_byte_array(LOGLEVEL_DEBUG, buffer, totalBytesRead);
    write_log(LOGLEVEL_DEBUG, "TCP Client - Exiting read_message_from_server");
    return totalBytesRead;
}

/**
 * Sends data to the server.
 *
 * @param serverSocket The server socket to send the data to.
 * @param data Pointer to the data to be sent.
 * @param dataLength The length of the data in bytes.
 */
int send_to_server(SOCKET serverSocket, const char* data, int dataLength) {
    write_log(LOGLEVEL_DEBUG, "TCP Client - Entering send_to_server");
    // Check for null data or zero length
    if (!data || dataLength <= 0) {
        write_log(LOGLEVEL_ERROR, "TCP Client - Invalid data to send");
        return -1;
    }

    // Send the data
    if (send(serverSocket, data, dataLength, 0) == SOCKET_ERROR) {
        write_log_format(LOGLEVEL_ERROR, "TCP Client - Failed to send data. Error Code: %d", WSAGetLastError());
        return -1;
    }

    write_log_format(LOGLEVEL_DEBUG, "TCP Client - Sent %d bytes to server:", dataLength);
    write_log_byte_array(LOGLEVEL_DEBUG, data, dataLength);

    return 0;
}

/**
 * Cleans up the client by closing the socket and cleaning up WinSock resources.
 *
 * @param serverSocket The server socket to close.
 */
void cleanup_client(SOCKET serverSocket) {
    write_log(LOGLEVEL_DEBUG, "TCP Client - Entering cleanup_client");
    // Close the socket if it's valid
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
    }

    // Cleanup WinSock
    WSACleanup();
    write_log(LOGLEVEL_INFO, "TCP Client - Client resources cleaned up");
    write_log(LOGLEVEL_DEBUG, "TCP Client - Exiting cleanup_client");
}
