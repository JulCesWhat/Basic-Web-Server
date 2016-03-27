#include <ctype.h>
#include <stdlib.h>
#include "utils.h"

#define PAGE_NOT_FOUND "/notfound.html"
#define MAX_PATH_SIZE 250

#define    BAD_REQUEST             "<html> "\
                                                    "<head> "\
                                                        "<title>Bad Request File</title> "\
                                                    "</head> "\
                                                     "<body> "\
                                                            "<h1>HTTP/1.0 400 Bad Request document.</h1> "\
                                                            "<p>This webserver only uses GET to serve files.</p> "\
                                                      "</body> "\
                                                    "</html> "


#define    FILE_NOT_FOUND        "<html>"\
                                                    "<head> " "<title>File Not Found</title> " \
                                                    "</head> "\
                                                     "<body> "\
                                                        "<h1>HTTP/1.0 404 File Not Found.</h1> "\
                                                        "<p>The file was not found in the path you entered. Go back and try again.</p> "\
                                                      "</body> "\
                                                    "</html> "





/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
strlcat(char *dst, const char *src, size_t siz)
{
    register char *d = dst;
    register const char *s = src;
    register size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (n-- != 0 && *d != '\0')
        d++;
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
        return(dlen + strlen(s));
    while (*s != '\0') {
        if (n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return(dlen + (s - src));   /* count does not include NUL */
}


/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
    register char *d = dst;
    register const char *s = src;
    register size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0) {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';      /* NUL-terminate dst */
        while (*s++)
            ;
    }

    return(s - src - 1);    /* count does not include NUL */
}









/*send_file_found HELPER FUNCTION
 *  RECEIVES: <content_type> and <content_type_size> to write from <path> array
 *  FUNCTION: Looks at the file extension in <path> and writes an specific content type
 *                      from the file extension to <content_type> 
 *                      <content-type> holds 
 *                           text/html                               for .html/.htm
 *                           image/gif                               for .gif
 *                           image/jpeg                            for .jpeg/.jpg
 *                           text/plain                              for .txt 
 *                           application/octet-stream       for anything else.
 *  RETURNS: nothing
 */
void get_content_type(char* content_type, int content_type_size ,char* path){
    char* ext = strrchr(path, '.'); /*return pointer to the last ocurrence of '.' in path */
    
    if (ext != NULL)
    {
                                
            
            if (   (strcmp(ext, ".html")  == 0 )   ||   ( strcmp(ext, ".htm")  == 0 )   )
                            strlcpy(content_type, "text/html", content_type_size);
            else if  (  strcmp(ext, ".gif")  == 0  )
                            strlcpy(content_type, "image/gif", content_type_size);
            else if  (   (strcmp(ext, ".jpeg")  == 0 )   ||    (strcmp(ext, ".jpg")  == 0 )   )
                            strlcpy(content_type, "image/jpeg", content_type_size);
            else if  (  strcmp(ext, ".txt")  == 0  )
                            strlcpy(content_type, "text/plain", content_type_size);
            else 
                            strlcpy(content_type, "application/octet-stream", content_type_size);
            
    }else 
    {
        strlcpy(content_type, "application/octet-stream", content_type_size);
    }
    
    return;
}


/*  send_response HELPER FUNCTION
 *  RECEIVES: file descriptor <sock>, an opennedd file descriptor <infile>, and the <path> of the file to serve
 *  FUNCTION: Writes everithing from <infile> to file descriptor <sock>
 *  RETURNS: nothing
 */
void send_file_found(FILE* sock, FILE* infile, char* path){
        char buf[MAX_PATH_SIZE] = {0};
        char *content;
        long bytesRead;
        char content_type[MAX_PATH_SIZE] = {0};
       
        get_content_type(content_type, sizeof(content_type), path ); /* gets the type of  file content */
        
        snprintf(buf, sizeof(buf), "HTTP/1.0 200 OK\nContent-type: %s \n\n", content_type ); /*  snprintf = safe string print to buffer */
        fputs(buf, sock); /*  write to file descriptor (socket in client)  */
        
        memset(buf, '\0', sizeof(buf) ); /* refill "buf" with null values */




        fseek( infile , 0L , SEEK_END);
        bytesRead = ftell( infile );
        rewind( infile );

        /* allocate memory for entire content */
        content = (char*)malloc(bytesRead*sizeof(char));
        if( !content ) fclose(infile),fputs("memory alloc fails",stderr),exit(1);

        /* copy the file into the content */
        if( 1!=fread( content , bytesRead, 1 , infile) ){
           fclose(infile),free(content),fputs("entire read fails",stderr),exit(1);
        }

        fputs(content, sock);

        free(content);        

        return;
}



/*  RECEIVES: file descriptor <sock> and the client's request: <command>, <path>, and <http_version>
 *  FUNCTION: Finds the file in <path> and prints it in the screen of client.
 *                      If file not found, it prints a nice error message in the client.
 *                      If the command is invalid, it prints a nice error message in the client.
 *  RETURNS: nothing
 */
void send_response(FILE* sock, char*  command, char* path, char* http_version, const char* rootdir)
{
    char full_path[MAX_PATH_SIZE] = {0}; /* "full_path" will contain the whole path from the root to serve the file */
    char buf[MAX_PATH_SIZE] = {0};
    FILE *infile;
        
    /* Invalid Request. Resques was not GET. Send bad request file */
    if (strcmp(command, "GET" ) != 0 )
    {
        snprintf(buf, sizeof(buf), "HTTP/1.0 200 OK\nContent-type: text/html \n\n"); /*  snprintf = safe string print to buffer */
        fputs(buf, sock); /*  write to file descriptor (socket in client)  */
        
        memset(buf, '\0', sizeof(buf) ); /* refill "buf" with null values */
        snprintf(buf, sizeof(buf), BAD_REQUEST);
        fputs(buf, sock); /*  write to file descriptor (socket in client)  */

        return;
    }
  
    /* Proces valid request */
    strlcpy(full_path, rootdir, sizeof(full_path) ); /* get full path */
    strlcat(full_path, path +1, sizeof(full_path) ); /* concatenate rootdir and path */

    printf("\n%s\n", rootdir);
    printf("\n%s\n", path);
    printf("\n%s\n", full_path);

    infile = fopen(full_path,"r");
    if (infile != NULL){ /*  openned file successfully  */
        send_file_found(sock, infile, path);

        fclose(infile);
        
    } else { /*  error openning file. maybe file was not found  */
        
        memset(buf, '\0', sizeof(buf) ); /* refill "buf" with null values */
        snprintf(buf, sizeof(buf), "HTTP/1.0 200 OK\nContent-type: text/html \n\n"); /*  snprintf = safe string print to buffer */
        fputs(buf, sock); /*  write to file descriptor (socket in client)  */
        
        memset(buf, '\0', sizeof(buf) ); /* refill "buf" with null values */
        snprintf(buf, sizeof(buf), FILE_NOT_FOUND);
        fputs(buf, sock); /*  write to file descriptor (socket in client)  */

    }
    return;
}



// Generic log-to-stdout logging routine
// Message format: "timestamp:pid:user-defined-message"
void blog(const char *fmt, ...) {
    // Convert user format string and variadic args into a fixed string buffer
    char user_msg_buff[256];
    va_list vargs;
    va_start(vargs, fmt);
    vsnprintf(user_msg_buff, sizeof(user_msg_buff), fmt, vargs);
    va_end(vargs);

    // Get the current time as a string
    time_t t = time(NULL);
    struct tm *tp = localtime(&t);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tp);

    // Print said string to STDOUT prefixed by our timestamp and pid indicators
    printf("%s:%d:%s\n", timestamp, getpid(), user_msg_buff);
}