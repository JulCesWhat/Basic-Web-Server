/* Don't include this file twice... */
#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <malloc.h>
#include <errno.h>  

#include <stdarg.h>     // Variadic argument lists (for blog function)
#include <time.h>       // Time/date formatting function (for blog function)
#include <unistd.h>     // Standard system calls
#include <signal.h>     // Signal handling system calls (sigaction(2))


 #include <pthread.h>

 pthread_mutex_t mutex; /* a mutex to protect updating the statuscounts */

 int NumOfConnected;


size_t
strlcat(char *dst, const char *src, size_t siz);

size_t
strlcpy(char *dst, const char *src, size_t siz);

void get_content_type(char* content_type, int content_type_size ,char* path);

void send_file_found(FILE* sock, FILE* infile, char* path);

void SendBadReq(FILE* sock);

void send_response(FILE* sock, char*  command, char* path, char* http_version, const char* rootdir);

void blog(const char *fmt, ...);



#endif