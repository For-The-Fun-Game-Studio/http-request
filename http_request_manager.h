/*
 * This file constructed by Remzi ISCI
 *        11/20/2024
 *
 * This file header file for simple http request manager without using any library.
 * Each request will be handled in a separate thread.
 * Request does not use ssl. If you want to use ssl, you need to use openssl or other library like curl.
 * You can this codes freely even in commercial projects, it will be kind if you mention me in your project.
 * For use on windows uncomment define WIN32
 */



#ifndef FATIO_HTTP_REQUEST_MANAGER_H
#define FATIO_HTTP_REQUEST_MANAGER_H

/**
 * Server root address
 * set your server address here
 */
#define Server "httpbin.org"

/**
 * Server port, the most commonly 80 for http
 */
#define PORT 80


/**
 * Callback function type that will be called when the response is received
 * @param response The response from the server
 *
 * simple callback function that takes a string as a parameter holds response data
 *
 *   void handle_response(const char *response) {
 *     printf("HTTP Response: %s\n", response);
 *   }
 */
typedef void (*ResponseCallback)(const char *);



/**
 * Send a HTTP POST request to the server
 * @param callback
 * @param data to send forexaample "name=remzi&age=25"
 * @param post_path path to send post request for example "/user.php"
 *
 *example:
 *    send_http_post_request(handle_response, "name=remzi", "user.php");
 */
 void send_http_post_request(ResponseCallback callback, const char *data, const char  *post_path);

/**
* Get the value of the key from the response
* @param response The response from the server
* @param key The key to get the value
* @param value The value of the key
*/
void get_value_from_api_response(const char *response, const char *key, char *value);


#endif // FATIO_HTTP_REQUEST_MANAGER_H
