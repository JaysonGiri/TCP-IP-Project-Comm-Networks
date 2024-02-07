/*
 * Author: Sanjay Giri Prakash
 * Date: Nov. 8, 2023 (Due: Dec. 7, 2023)
 * Course: CSCI-3550 (Sec:002)
 * Description: PROJECT-01, Client.
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
int sockfd = -1;

/* Define a place for the size of the outbuf 100MiB. */
#define BUF_SIZE (100 * 1024 * 1024)

/* For storing outgoing data using 'send()'. */
char *outbuf = NULL;

/* Keep track of HOW MANY bytes we've sent when we call 'send()'. */
int bytes_sent;

/* To store the 'file descriptor' for the input file. */
int fd_in;

/* To keep track of bytes read when we call 'read()'. */
int bytes_read;

/* To store the numeric string port number. */
const char *port_str;

/* To store the converted port number. */
unsigned short int port;

/* Declaring SIGINT_handler function. */
void SIGINT_handler(int sig);

/* Declaring cleanup function. */
void cleanup(void);

int main(int argc, char *argv[])
{
    /* Declaring the iterating variable i. */
    int i;

    /* Declare an IP address structure and socket structure. */

    /* For Internet address. */
    struct in_addr ia;

    /* For socket address. */
    struct sockaddr_in sa;

    /* Checking if user provided IP address and Port Number. */
    if (argc < 3)
    {
        /* Issue an ERROR message the proper way. */
        fprintf(stderr, "client: USAGE: client <IP_addr> <port> <files>...\n");

        cleanup();

        /* Terminate the program. */
        exit(EXIT_FAILURE);
    } /* end if() */

    /* Getting the Port number from argv[2]. */
    port_str = argv[2];

    /* Converting the numeric string into an unsigned short int. */
    port = (unsigned short int)atoi(port_str);

    /* Setting the Internet address 127.0.0.1 from argv[1] given by the user. */
    ia.s_addr = inet_addr(argv[1]);

    /* Next fill the socket address structure fields. */

    /* Use IPv4 addresses. */
    sa.sin_family = AF_INET;

    /* Attach IP address structure. */
    sa.sin_addr = ia;

    /* Set port number in Network Byte Order. */
    sa.sin_port = htons(port);

    /* Our 'socket address' (stored in 'sa') is now ready. */

    /* Install 'signal handler' catching [CTRL]+[C]. */
    signal(SIGINT, SIGINT_handler);

    /* Checking for Privileged port numbers. */
    if ((port >= 0) && (port <= 1023))
    {
        /* Issue an ERROR message the proper way. */
        fprintf(stderr, "client: ERROR: Port number is privileged.\n");

        cleanup();

        /* Terminate the program. */
        exit(EXIT_FAILURE);
    } /* end if() */

    /* If statement to check if user provided any files. */
    if (argc >= 4)
    {

        /* Looping through the given files. */
        for (i = 3; i < argc; i++)
        {

            /* Allocate Memory. */
            outbuf = (void *)malloc(BUF_SIZE);

            /* Check if allocation was successful. */
            if (outbuf == NULL)
            {
                /* Issue an ERROR message the proper way. */
                fprintf(stderr, "client: ERROR: Failed to allocate memory.\n");

                cleanup();

                /* Terminate the program. */
                exit(EXIT_FAILURE);
            } /* end if() */

            /* Create a new TCP socket. */
            sockfd = socket(AF_INET, SOCK_STREAM, 0);

            /* Check that it succeeded. */
            if (sockfd < 0)
            {
                /* Issue an ERROR message the proper way. */
                fprintf(stderr, "client: ERROR: Failed to create socket.\n");

                cleanup();

                /* Terminate the program. */
                exit(EXIT_FAILURE);
            } /* end if() */

            fprintf(stdout, "client: Connecting to %s:%s...\n", argv[1], argv[2]);

            /* Attempt to connect with 127.0.0.1:argv[2](port). */
            if (connect(sockfd, (struct sockaddr *)&sa, sizeof(sa)) != 0)
            {
                /* Issue an ERROR message the proper way. */
                fprintf(stderr, "client: ERROR: connecting to %s:%s.\n", argv[1], argv[2]);

                cleanup();

                /* Terminate the program. */
                exit(EXIT_FAILURE);
            } /* end if() */

            fprintf(stdout, "client: Success!\n");

            fprintf(stdout, "client: Sending: %s...\n", argv[i]);

            /* Attempt to open the file, 'argv[i]'. */
            fd_in = open(argv[i], O_RDONLY);

            /* Check IF we failed in opening the file! */
            if (fd_in < 0)
            {
                /* Issue an ERROR message the proper way. */
                fprintf(stderr, "client: ERROR: Failed to open: %s.\n", argv[i]);

                fprintf(stderr, "client: ERROR: Encountered while transferring \"%s\". \n\n", argv[i]);

                /* Continue to next file. */
                continue;

            } /* end if() */

            /* Attempt to READ ANY number of bytes UP TO the max size possible. */
            bytes_read = read(fd_in, outbuf, BUF_SIZE);

            /* Check if we failed to read ANYTHING! */
            if (bytes_read < 0)
            {

                /* Issue an ERROR message the proper way. */
                fprintf(stderr, "client: ERROR: Unable to read file: %s.\n", argv[i]);

                cleanup();

                /* Terminate the program. */
                exit(EXIT_FAILURE);
            } /* end if() */

            /* Closing the file we just read. */
            close(fd_in);

            /* Send the data out a given socket. */
            bytes_sent = send(sockfd, (const void *)outbuf, bytes_read, 0);

            /* Verify that we sent what we ASKED to send. */
            if (bytes_sent != bytes_read)
            {
                /* Issue an ERROR message the proper way. */
                fprintf(stderr, "client: ERROR: While sending data.\n");

                cleanup();

                /* Terminate the program. */
                exit(EXIT_FAILURE);
            } /* end if() */

            /* Goodbye, socket...  :*(  */
            if (sockfd > -1)
            {
                fprintf(stdout, "client: Done.\n\n");

                /* Close the socket. */
                close(sockfd);

                /* Mark it as unused. */
                sockfd = -1;
            } /* end if() */
        }
    }

    /* Else statemet if no files were provided were provided by the user. */
    else
    {
        /* Issue an ERROR message the proper way. */
        fprintf(stderr, "client: USAGE: client <IP_addr> <port> <files>...\n");

        cleanup();

        /* Terminate the program. */
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "client: File transfer(s) complete.\n");

    fprintf(stdout, "client: Closing connection.\n");

    cleanup();

    /* Proper way to finish a program in a POSIX-system. */
    exit(EXIT_SUCCESS);
} /* end main() */

/* SIGINT handler for the client. */
void SIGINT_handler(int sig)
{

    /* Issue an ERROR message the proper way. */
    fprintf(stderr, "client: Client interrupted. Shutting down.\n");

    /* Cleanup after yourself. */
    cleanup();

    /* Exit for 'reals'. */
    exit(EXIT_FAILURE);

} /* end SIGINT_handler() */

/* Cleanup function to release memory, closing files, and socket. */
void cleanup(void)
{
    /* The proper way to release memory. */
    if (outbuf != NULL)
    {
        /* Release it. */
        free(outbuf);

        /* Don't leave a 'dangling' pointer. */
        outbuf = NULL;

    } /* end if() */

    /* Close the input file. */
    if (fd_in >= 0)
    {
        /* Close the file. */
        close(fd_in);

        /* Mark it as close to avoid 're-closing' again */
        fd_in = -1;

    } /* end if() */

    fprintf(stdout, "client: Goodbye!\n");

    /* Goodbye, socket...  :*(  */
    if (sockfd > -1)
    {
        /* Close the socket. */
        close(sockfd);

        /* Mark it as unused. */
        sockfd = -1;
    } /* end if() */

} /* end cleanup() */