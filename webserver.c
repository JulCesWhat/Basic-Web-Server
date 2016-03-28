/* Echo Server: an example usage of EzNet
 * (c) 2016, Bob Jones University
 */
#include <stdbool.h>    // For access to C99 "bool" type
#include <stdio.h>      // Standard I/O functions
#include <stdlib.h>     // Other standard library functions
#include <string.h>     // Standard string functions
#include <errno.h>      // Global errno variable

#include <unistd.h>     // Standard system calls
#include <signal.h>     // Signal handling system calls (sigaction(2))

#include "eznet.h"      // Custom networking library

#include "utils.h"

 #include <pthread.h>

 extern pthread_mutex_t mutex; /* a mutex to protect updating the statuscounts */

 extern int NumOfConnected;

// GLOBAL: settings structure instance
struct settings {
    const char *bindhost;   // Hostname/IP address to bind/listen on
    const char *bindport;   // Portnumber (as a string) to bind/listen on
    const char *bindroot;
    int bindAllowed;
} g_settings = {
    .bindhost = "localhost",    // Default: listen only on localhost interface
    .bindport = "5000",         // Default: listen on TCP port 5000
    .bindroot = "",
    .bindAllowed = 5,
};

// Parse commandline options and sets g_settings accordingly.
// Returns 0 on success, -1 on false...
int parse_options(int argc, char * const argv[]) {
    int ret = -1; 

    char op;
    while ((op = getopt(argc, argv, "h:p:r:w:")) > -1) {
        switch (op) {
            case 'h':
                g_settings.bindhost = optarg;
                break;
            case 'p':
                g_settings.bindport = optarg;
                break;
            case 'r':
                g_settings.bindroot = optarg;
                break;
            case 'w':
                g_settings.bindAllowed = atoi(optarg);
                break;
            default:
                // Unexpected argument--abort parsing
                goto cleanup;
        }
    }

    ret = 0;
cleanup:
    return ret;
}

// GLOBAL: flag indicating when to shut down server
volatile bool server_running = false;

// SIGINT handler that detects Ctrl-C and sets the "stop serving" flag
void sigint_handler(int signum) {
    blog("Ctrl-C (SIGINT) detected; shutting down...");
    server_running = false;
}

// Connection handling logic: reads/echos lines of text until error/EOF,
// then tears down connection.
//void *handle_client(struct client_info *client) {
void *handle_client(void *x_void_ptr) {

    struct client_info *client = (struct client_info *) x_void_ptr;
    FILE *stream = NULL;

    // Wrap the socket file descriptor in a read/write FILE stream
    // so we can use tasty stdio functions like getline(3)
    // [dup(2) the file descriptor so that we don't double-close;
    // fclose(3) will close the underlying file descriptor,
    // and so will destroy_client()]
    if ((stream = fdopen(dup(client->fd), "r+"))== NULL) {
        perror("unable to wrap socket");
        goto cleanup;
    }
    
    // Echo all lines
    char *line = NULL;
    size_t len = 0u;
    ssize_t recd;
    if ((recd = getline(&line, &len, stream)) > 0) {
        printf("\tReceived %zd ??? %zd byte line; echoing...\n", recd, len);

        printf("%s", line);

        char *command = (char*)malloc(recd*sizeof(char));
        char *path = (char*)malloc(recd*sizeof(char));
        char *http_version = (char*)malloc(recd*sizeof(char));

        //Adding this
        sscanf( line, "%s %s %s", command, path, http_version);

                while (getline(&line, &len, stream) > 0 && line[0] != '\r' && line[0] != '\n')
                        printf("%s", line);
                           /* throw it away */  
        printf("%s here \n", client->bindRoot);

        send_response(stream, command, path, http_version, client->bindRoot);

        free(command);
        free(path);
        free(http_version);
        //printf("%s\n", line);
    } else if (recd < 1 )
        {
            printf("Unexpected EOF. Unable to connect to client.\n");
            
            /* Handle request with no newline, which is an invalid request */
            send_response(stream, "GET without newline", "path", "http_version", client->bindRoot);
        
        }

cleanup:
    // Shutdown this client
    if (stream) fclose(stream);
    destroy_client_info(client);
    free(line);
    printf("\tSession ended.\n");
    return NULL;
}

//THE MAIN PROGRAM STARTS HERE
//
//
//THE MAIN PROGRAM STARTS HERE
int main(int argc, char **argv) {
    int ret = 1;

    // Network server/client context
    int server_sock = -1;

    NumOfConnected = 1;

    // Handle our options
    if (parse_options(argc, argv)) {
        printf("usage: %s [-p PORT] [-h HOSTNAME/IP]\n", argv[0]);
        goto cleanup;
    }

    // Install signal handler for SIGINT
    struct sigaction sa_int = {
        .sa_handler = sigint_handler
    };
    if (sigaction(SIGINT, &sa_int, NULL)) {
        LOG_ERROR("sigaction(SIGINT, ...) -> '%s'", strerror(errno));
        goto cleanup;
    }

    // Start listening on a given port number
    server_sock = create_tcp_server(g_settings.bindhost, g_settings.bindport);
    if (server_sock < 0) {
        perror("unable to create socket");
        goto cleanup;
    }
    blog("Bound and listening on %s:%s", g_settings.bindhost, g_settings.bindport);

    pthread_mutex_init(&mutex, NULL);  /* initialize semaphore */       
    server_running = true;
    while (server_running) {
        struct client_info client;

        // Wait for a connection on that socket
        if (wait_for_client(server_sock, &client)) {
            // Check to make sure our "failure" wasn't due to
            // a signal interrupting our accept(2) call; if
            // it was  "real" error, report it, but keep serving.
            if (errno != EINTR) { perror("unable to accept connection"); }
        } else {
            if((g_settings.bindAllowed + 1) > NumOfConnected){
                blog("connection from %s:%d", client.ip, client.port);
                blog("%d current request bein handled", NumOfConnected);

                //Starting threads....fun
                client.bindRoot = g_settings.bindroot;
                pthread_t thread;

                NumOfConnected++;
                if(pthread_create(&thread, NULL, handle_client, &client)){
                    fprintf(stderr, "Error creating thread\n");
                    return 1;
                }

                //handle_client(&client, g_settings.bindroot); // Client gets cleaned up in here
            }else{
                blog("Maximun number of request bein hadnled. Try again later.", client.ip, client.port);
            }
        }
    }
    ret = 0;

cleanup:
    if (server_sock >= 0) close(server_sock);
    return ret;
}
