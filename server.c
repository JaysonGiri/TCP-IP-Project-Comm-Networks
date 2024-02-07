/*
 * Author: Sanjay Giri Prakash
 * Date: Nov. 8, 2023 (Due: Dec. 7, 2023)
 * Course: CSCI-3550 (Sec:002)
 * Description: PROJECT-01, Server.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

/* Socket [file] descriptor. */
int listen_sockfd = -1;

/* Socket descriptor for accepted client. */
int cl_sockfd;

/* Define a place for the size of the inbuf 100MiB. */
#define BUF_SIZE (100 * 1024 * 1024)

/* For storing incoming data from 'recv()'. */
char *inbuf = NULL;

/* To keep track of *actual* vs *expected* bytes. */
int bytes_read;

/* To store the 'file descriptor' for the output file. */
int fd_out;

/* This array will hold dynamic filenames that we can construct using the "snprintf()" function. */
char fname[80];

/* To keep track of bytes written when we call 'write()'. */
int bytes_written;

/* Declaring variable that keep tracks input files to produce output files. */
int file_cntr;

/* To store the numeric string port number. */
const char *port_str;

/* To store the converted port number. */
unsigned short int port;

/* Declaring SIGINT_handler function. */
void SIGINT_handler(int sig);

/* Declaring cleanup function. */
void cleanup(void);

/* Declaring process function. */
void process(void);

int main(int argc, char *argv[])
{
    /* Setting for one of the socket options. */
    int val = 1;

    /* Declare an IP address structure and socket structure. */

    /* For Internet address.*/
    struct in_addr ia;

    /* For socket address. */
    struct sockaddr_in sa;

    /* Socket address of the client, once connected. */
    struct sockaddr_in cl_sa;

    /* Compute, and store the size of the socket address structure
    so we can pass it INDIRECTLY as a pointer to 'accept()'. */
    socklen_t cl_sa_size = sizeof(cl_sa);

    /* Checking if user provided Port Number. */
    if (argc < 2)
    {
        /* Issue an ERROR message the proper way. */
        fprintf(stderr, "server: USAGE: server <listen_Port>\n");

        cleanup();

        /* Terminate the program. */
        exit(EXIT_FAILURE);
    } /* end if() */

    port_str = argv[1];

    /* Converting the numeric string into an unsigned short int. */
    port = (unsigned short int)atoi(port_str);

    /* Setting the Internet address 127.0.0.1. */
    ia.s_addr = inet_addr("127.0.0.1");

    /* Next fill the socket address structure fields. */

    /* Use IPv4 addresses. */
    sa.sin_family = AF_INET;

    /* Attach IP address structure. */
    sa.sin_addr = ia;

    /* Set port number in Network Byte Order. */
    sa.sin_port = htons(port);

    /* Our 'socket address' (stored in 'sa') is now ready. */

    /* Create a new TCP socket. */
    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    /* Checking for Privileged port numbers. */
    if ((port >= 0) && (port <= 1023))
    {
        /* Issue an ERROR message the proper way. */
        fprintf(stderr, "server: ERROR: Port number is privileged.\n");

        cleanup();

        /* Terminate the program. */
        exit(EXIT_FAILURE);
    } /* end if() */

    /* Check if the new TCP socket we created succeeded. */
    if (listen_sockfd < 0)
    {
        /* Issue an ERROR message the proper way. */
        fprintf(stderr, "server: ERROR: Failed to create socket.\n");

        cleanup();

        /* Terminate the program. */
        exit(EXIT_FAILURE);
    } /* end if() */

    fprintf(stdout, "server: Server running.\n");

    /* Making the listening socket reusable. */
    if (setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&val, sizeof(int)) != 0)
    {
        /* Issue an ERROR message the proper way. */
        fprintf(stderr, "server: ERROR: setsockopt() failed.\n");

        cleanup();

        /* Terminate the program. */
        exit(EXIT_FAILURE);
    } /* end if() */

    /* Bind our listening socket to the socket address specified by 'sa' on the server side. */
    if (bind(listen_sockfd, (struct sockaddr *)&sa, sizeof(sa)) != 0)
    {
        /* Issue an ERROR message the proper way. */
        fprintf(stderr, "server: ERROR: Failed to bind socket.\n");

        cleanup();

        /* Terminate the program. */
        exit(EXIT_FAILURE);
    } /* end if() */

    /* Turn an already-bound socket into a 'listening' socket to start accepting connection requests from clients. */
    if (listen(listen_sockfd, 32) != 0)
    {
        /* Issue an ERROR message the proper way. */
        fprintf(stderr, "server: ERROR: listen(): Failed.\n");

        cleanup();

        /* Terminate the program. */
        exit(EXIT_FAILURE);
    } /* end if() */

    fprintf(stdout, "server: Awaiting TCP connections over port %s...\n", argv[1]);

    /* Install 'signal handler' catching [CTRL]+[C]. */
    signal(SIGINT, SIGINT_handler);

    /* Clear all the bytes of this structure to zero. */
    memset((void *)&cl_sa, 0, sizeof(cl_sa));

    while (1)
    {
        /* Waiting for a connection request from ANY client. */
        cl_sockfd = accept(listen_sockfd, (struct sockaddr *)&cl_sa, &cl_sa_size);
        file_cntr++;

        /* Checking if we have a valid socket. */
        if (cl_sockfd > 0)
        {
            /* Success! */
            fprintf(stdout, "server: Connection accepted! (file desc = %d)\n", cl_sockfd);

            /* Allocate Memory. */
            inbuf = (void *)malloc(BUF_SIZE);

            /* Check if allocation was successful. */
            if (inbuf == NULL)
            {
                /* Issue an ERROR message the proper way. */
                fprintf(stderr, "server: ERROR: Failed to allocate memory.\n");

                cleanup();

                /* Terminate the program. */
                exit(EXIT_FAILURE);
            } /* end if() */

            fprintf(stdout, "server: Receiving file...\n");

            /* Attempt to read data from a socket. */
            bytes_read = recv(cl_sockfd, (void *)inbuf, BUF_SIZE, MSG_WAITALL);

            /* Closing Client Socket. */
            if (cl_sockfd > -1)
            {
                fprintf(stdout, "server: Finished. Connection closed (file desc = %d).\n", cl_sockfd);

                /* Close the socket. */
                close(cl_sockfd);

                /* Mark it as unused */
                cl_sockfd = -1;
            } /* end if() */

            /* Check what 'recv()' returns and decide what to do based on that. */
            if (bytes_read >= 0)
            {
                /* Calling Process to write to file. */
                process();
            } /* end if() */
            /* Otherwise, we have an error. */
            else
            {
                /* Issue an ERROR message the proper way. */
                fprintf(stderr, "server: ERROR: Reading from socket.\n");

                cleanup();

                /* Terminate the program. */
                exit(EXIT_FAILURE);
            }
        } /* end if() */

        /* Otherwise... */
        else
        {
            /* Issue an ERROR message the proper way. */
            fprintf(stderr, "server: ERROR: While attempting to accept a connection.\n");

            cleanup();

            /* Terminate the program. */
            exit(EXIT_FAILURE);
        }
    }

    /* Proper way to finish a program in a POSIX-system. */
    exit(EXIT_SUCCESS);
} /* End main() */

/* Signal handler for SIGINT (via [CTRL]+[C]). */
void SIGINT_handler(int sig)
{

    /* Letting the user know we are shutting down the server. */
    fprintf(stderr, "\nserver: Server interrepted.  Shutting down.\n");

    /* Calling cleanup. */
    cleanup();

    /* Terminate the program... for 'reals' this time. */
    exit(EXIT_FAILURE);

} /* end SIGINT_handler() */

/* Cleanup function to release memory and closing input and output files before terminating program. */
void cleanup(void)
{
    /* The proper way to release memory. */
    if (inbuf != NULL)
    {
        /* Release it */
        free(inbuf);

        /* Don't leave a 'dangling' pointer. */
        inbuf = NULL;

    } /* end if() */

    /* Close the output file. */
    if (fd_out >= 0)
    {
        /* Close the file. */
        close(fd_out);

        /* Mark it as close to avoid 're-closing' again. */
        fd_out = -1;

    } /* end if() */

    /* Goodbye, client socket...  :*(  */
    if (cl_sockfd > -1)
    {
        /* Close the socket. */
        close(cl_sockfd);

        /* Mark it as unused. */
        cl_sockfd = -1;
    } /* end if() */

    /* Goodbye, listening socket...  :*(  */
    if (listen_sockfd > -1)
    {
        fprintf(stdout, "server: Goodbye!\n");

        /* Close the socket. */
        close(listen_sockfd);

        /* Mark it as unused. */
        listen_sockfd = -1;
    } /* end if() */

} /* end cleanup() */

void process(void)
{

    /* Construct a name dynamically. (Will result in "file-01.dat"). */
    sprintf(fname, "file-%02d.dat", file_cntr);

    /* Open the file for WRITING, truncate it to zero bytes if it already exists. */
    fd_out = open(fname, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);

    /* Check if this succeeded. */
    if (fd_out < 0)
    {
        /* Issue an ERROR message the proper way. */
        fprintf(stderr, "server: ERROR: Unable to create: %s\n.", fname);

        cleanup();

        /* Exit. */
        exit(EXIT_FAILURE);

    } /* end if() */

    /* Write the data in the buffer... */
    bytes_written = write(fd_out, inbuf, (size_t)bytes_read);

    /* To let user know that program is writing to the output file. */
    fprintf(stdout, "server: Saving file: %s...\n", fname);

    /* Check that we ACTUALLY wrote the number of bytes we wanted! */
    if (bytes_written != bytes_read)
    {
        /* Issue an ERROR message the proper way. */
        fprintf(stderr, "server: ERROR: Unable to write : %s\n.", fname);

        cleanup();

        /* Terminate. */
        exit(EXIT_FAILURE);

    } /* end if() */

    /* Close the file. */
    close(fd_out);

    fprintf(stdout, "server: Done.\n\n");
} /* end process() */