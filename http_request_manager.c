//
// Created by remzi on 11/20/2024.
//

#include "http_request_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>


/**
 * Data structure to pass to the thread
 */
typedef struct {
  ResponseCallback callback;     // Callback function to call when the response is received
  char response[4096];           // Buffer to hold the response
  char post_data[1024];          // Buffer to hold the POST data
  char post_path[1024];          // Buffer to hold the POST path
} ThreadData;

/**
 * Construct a HTTP POST request and send it to the server
 * @param data The data to send in the POST request
 * @return The response from the server
 */
char *construct_http_post_request(const char *data, char *post_path) {
  int sock;
  struct sockaddr_in server;                                            // Struct to hold server details
  struct addrinfo hints, *res;                                          // Structs to hold address info
  char response[4096];                                           // Buffer for the HTTP response
  char request[2048];                                            // Buffer for the HTTP request
  ssize_t bytes_received;

  // Resolve domain name to IP--------------------------------------------------
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(Server, NULL, &hints, &res) != 0) {
    perror("DNS resolution failed");
    exit(EXIT_FAILURE);
  }
  //----------------------------------------------------------------------------


  // Create socket--------------------------------------------------------------
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Could not create socket");
    exit(EXIT_FAILURE);
  }
  //----------------------------------------------------------------------------

  // Populate server details----------------------------------------------------
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);
  server.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;

  freeaddrinfo(res);                                                    // Free the address info to prevent memory leaks
  //----------------------------------------------------------------------------


  // Connect to the remote server-----------------------------------------------
  if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Connection failed");
    close(sock);
    exit(EXIT_FAILURE);
  }
  //----------------------------------------------------------------------------

  // Construct HTTP POST request with headers and data
  snprintf(request, sizeof(request),
           "POST /%s HTTP/1.1\r\n"                                   // POST request can change as GET or PUT
           "Host: %s\r\n"                                            // Server address
           "User-Agent: C99Client/1.0\r\n"                           // User agent  (optional)
           "Content-Type: application/x-www-form-urlencoded\r\n"     // Content type
           "Content-Length: %zu\r\n"                                 // Length of the data
           "Connection: close\r\n\r\n"                               // Connection close, last of the headers
           "%s",post_path , Server, strlen(data), data);             // Data

  // Send request---------------------------------------------------------------
  if (send(sock, request, strlen(request), 0) < 0) {
    perror("Send failed");
    close(sock);
    exit(EXIT_FAILURE);
  }
  //----------------------------------------------------------------------------

  // Receive response-----------------------------------------------------------
  bytes_received = recv(sock, response, sizeof(response) - 1, 0);
  if (bytes_received < 0) {
    perror("Receive failed");
    close(sock);
    exit(EXIT_FAILURE);
  }

  response[bytes_received] = '\0';                                 // Null-terminate the response
  //----------------------------------------------------------------------------
  close(sock);                                                     // Close the socket

  return response;                                                 // Return the response on slave thread
}

/**
 * Thread function to perform the HTTP POST request, this will work as a separate thread
 * @param arg The data to pass to the thread
 * @return NULL
 */
void *perform_request(void *arg) {
  // Cast the argument to ThreadData comes from the main thread
  ThreadData *data = (ThreadData *)arg;

  // Get the response from the HTTP POST request
  const char *response = construct_http_post_request(data->post_data, data->post_path);

  // Copy the response into ThreadData's response field
  strncpy(data->response, response, sizeof(data->response) - 1);

  // Null-terminate the response
  data->response[sizeof(data->response) - 1] = '\0';

   // Print the response
   printf("HTTP Response in slave thread: %s\n", data->response);

  // Call the provided callback
  if (data->callback) {
    data->callback(data->response);
  }

  return NULL;
}


void send_http_post_request(ResponseCallback callback, const char *data, const char  *post_path) {
  // Create a ThreadData struct to pass to the thread---------------------------
  ThreadData dataStruct;
  dataStruct.callback = callback;
  strncpy(dataStruct.post_data, data, sizeof(dataStruct.post_data) - 1);
  dataStruct.post_data[sizeof(dataStruct.post_data) - 1] = '\0';

  strncpy(dataStruct.post_path, post_path, sizeof(dataStruct.post_path) - 1);
  dataStruct.post_path[sizeof(dataStruct.post_path) - 1] = '\0';
  //----------------------------------------------------------------------------

  // Create a thread to perform the HTTP POST request--------------------------
  pthread_t thread;
  pthread_create(&thread, NULL, perform_request, &dataStruct);

  // Start the thread----------------------------------------------------------
  pthread_join(thread, NULL);
}

void get_value_from_api_response(const char *response, const char *key, char *value) {
    size_t key_length = strlen(key);
    size_t response_length = strlen(response);
  //Check is response containing json data
      size_t star_of_json = 0;
    for (size_t i = 0; i < response_length; ++i) {
        if (response[i] == '{') {
            star_of_json = i;
            break;
        }
    }
    if (star_of_json == 0) {
        fprintf(stderr, "Error: Response does not contain json\n");
        return;
    }

    for (size_t i = start_of_json; i < response_length; ++i) {
        // Check if the key matches at the current position
        if (strncmp(&response[i], key, key_length) == 0 && response[i + key_length] == '"') {
            // Locate the start of the value
            const char *value_start = &response[i + key_length + 3]; // Skip `key"` and `:"`
            const char *value_end = strchr(value_start, '"');
            if (!value_end) {
                fprintf(stderr, "Error: Key value not properly terminated in response\n");
                return;
            }

            // Calculate the size of the value
            size_t value_size = value_end - value_start;


            strncpy(value, value_start, value_size);
            (value)[value_size] = '\0'; // Null-terminate the string

            return;
        }
    }

    fprintf(stderr, "Error: Key not found in response\n");
}
