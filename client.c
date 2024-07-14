#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>

#include "protocol.h"

#define TCP_PORT 41189 // we can use yours ;) <3
#define MAX_CMD 1024 * 5

void clientCmd(char *cmd);
void serverCmd(char *cmd, int sd);
int tokenise(char line[], char *tokens[]);

void pwd(int sd);
void server_dir(int sd);
void get(int sd, char *tokens[]);
void put(int sd, char *tokens[]);
void server_cd(int sd, char *tokens[]);

int main(int argc, char *argv[])
{
    unsigned short port;        // hold port number
    char host[60];              // server hostname
    struct sockaddr_in serv_ad; // holds server address
    struct hostent *hp;
    int sd;

    //  1.get server name and port

    if (argc == 2) // this is if the host is given as an argument
    {
        strcpy(host, argv[1]);
        port = TCP_PORT;
    }
    else if (argc == 1) // no argument for host
    {
        strcpy(host, "localhost");
        port = TCP_PORT;
    }
    else // closes program if there are too many arguments
    {
        printf("Please either supply a hostname as an argument, or nothing to use 'localhost'");
        exit(1);
    }

    // 2. Get the host address from host name -> build socket
    bzero((char *)&serv_ad, sizeof(serv_ad));
    serv_ad.sin_family = AF_INET; // TCP/IP
    serv_ad.sin_port = htons(port);
    if ((hp = gethostbyname(host)) == NULL)
    {
        printf("%s is not a valid host\n", host);
        exit(1);
    }
    serv_ad.sin_addr.s_addr = *(u_long *)hp->h_addr;

    // 3.create tcp socket

    sd = socket(PF_INET, SOCK_STREAM, 0);
    if (connect(sd, (struct sockaddr *)&serv_ad, sizeof(serv_ad)) < 0)
    {
        perror("Cannot connect to server");
        exit(1);
    }

    // Post connection code...

    char cmd[MAX_CMD];
    char cmd2[MAX_CMD];
    int wr, re, i = 0;

    printf(">Please enter a command to continue...\n");

    while (++i)
    {
        printf(">");
        cmd[0] = '\0';
        cmd2[0] = '\0';
        scanf("%[^\n]%*c", cmd);

        if (cmd[0] == 'l')
        {

            clientCmd(cmd);
        }
        else if (strcmp(cmd, "quit") == 0)
        {
            printf("\n>goodbye!\n");
            break;
        }
        else
        {

            serverCmd(cmd, sd);
        }
    }
}

void serverCmd(char *cmd, int sd)
{
    int wr, re;
    char cmd2[MAX_CMD];
    char *tokens[256];

    // seperate commands into tokens
    tokenise(cmd, tokens);

    if (strcmp(tokens[0], "dir") == 0)
    {
        server_dir(sd);
    }
    else if (strcmp(tokens[0], "pwd") == 0)
    {

        pwd(sd);
    }
    else if (strcmp(tokens[0], "cd") == 0)
    {

        server_cd(sd, tokens);
    }
    else if (strcmp(tokens[0], "get") == 0)
    {
        get(sd, tokens);
    }
    else if (strcmp(tokens[0], "put") == 0)
    {
        put(sd, tokens);
    }
    else
    {
        printf("invalid command\n");
    }
}

void clientCmd(char *cmd)
{
    pid_t pid;

    if (strlen(cmd) > 4)
    {
        // tokenise and check for lcd
        char *token = strtok(cmd, " ");

        if (strcmp(token, "lcd") == 0)
        {
            token = strtok(NULL, " ");

            if (chdir(token) == -1)
            {
                printf("lcd to %s failed: No such file or directory\n", token);
            }
            else
            {
                printf("directory changed\n");
            }
        }
        else
        {
            printf("invalid command!\n");
        }
    }

    else if (strcmp(cmd, "ldir") == 0)
    {
        if ((pid = fork()) == 0)
        {
            execlp("dir", "dir", NULL);
        }
        else
        {
            wait(NULL);
        }
    }
    else if (strcmp(cmd, "lpwd") == 0)
    {
        if ((pid = fork()) == 0)
        {
            execlp("pwd", "pwd", NULL);
        }
        else
        {
            wait(NULL);
        }
    }
    else
    {
        printf("invalid command\n");
    }
}

// from one of hong's lab examples. splits the commands into tokens
int tokenise(char line[], char *tokens[])
{
    char *tk;
    int length = 0;

    tk = strtok(line, " ");
    tokens[length] = tk;

    while (tk != NULL)
    {
        length++;

        if (length >= 256)
        {
            length = -1;
            break;
        }

        tk = strtok(NULL, " ");
        tokens[length] = tk;
    }

    return length;
}

void pwd(int sd)
{
    char op;
    int buf;

    if (wr_opcode(sd, PWD_CODE) == -1)
    {
        printf("Cannot send command!\n");
        return;
    }

    if (re_opcode(sd, &op) == -1)
    {
        printf("Could not read response...\n");
        return;
    }

    if (op != PWD_CODE)
    {
        printf("Incorrect Opcode!\n");
        return;
    }

    if (re_twobyte(sd, &buf) == -1)
    {
        printf("Size error\n");
        return;
    }

    char cwd[buf + 1];

    if (re_data(sd, cwd, buf) == -1)
    {
        printf("Cannot retrieve server response\n");
        return;
    }

    cwd[buf] = '\0';
    printf("%s\n", cwd);
}

void server_dir(int sd)
{
    char op;
    int buf;

    if (wr_opcode(sd, DIR_CODE) == -1)
    {
        printf("Cannot send command!\n");
        return;
    }

    if (re_opcode(sd, &op) == -1)
    {
        printf("Could not read response...\n");
        return;
    }

    if (op != DIR_CODE)
    {
        printf("Incorrect Opcode!\n");
        return;
    }

    if (re_fourbyte(sd, &buf) == -1)
    {
        printf("Size error\n");
        return;
    }

    char direc[buf + 1];

    if (re_data(sd, direc, buf) == -1)
    {
        printf("Cannot read directory!\n");
        return;
    }

    direc[buf] = '\0';

    printf("%s\n", direc);
}

void server_cd(int sd, char *tokens[])
{
    char op;
    int ack;
    int buf = strlen(tokens[1]);

    if (wr_opcode(sd, CD_CODE) == -1)
    {
        printf("Cannot send command\n");
        return;
    }

    if (wr_twobyte(sd, buf) == -1)
    {
        printf("failed to send path length");
        return;
    }

    if (wr_data(sd, tokens[1], buf) == -1)
    {
        printf("Failed to send path to server!\n");
        return;
    }

    if (re_opcode(sd, &op) == -1)
    {
        printf("Failed to read response");
        return;
    }

    if (op != CD_CODE)
    {
        printf("Invalid opcode");
        return;
    }

    if (re_twobyte(sd, &ack) == -1)
    {
        printf("Path could not change\n");
    }
    else
    {
        printf("Directory changed succesfully!\n");
    }
}

void get(int sd, char *tokens[])
{
    char op, check;
    short buf;

    int fileSize, fileHandle;
    char *fileBuffer;
    char fileName[256];
    int file;
    char response[100];

    if (wr_opcode(sd, GET_CODE) == -1)
    {
        printf("Cannot send command!\n");
        return;
    }

    // get the file name
    strcpy(fileName, tokens[1]);

    // send file name to server
    if (wr_twobyte(sd, strlen(fileName)) == -1)
    {
        printf("Error writing byte\n");
        return;
    }
    if (wr_data(sd, fileName, sizeof(fileName)) == -1)
    {
        printf("Error writing data\n");
        return;
    }
    printf("Sent the file name to the server: %s\n", fileName);

    if (re_fourbyte(sd, &fileSize) == -1)
    {
        printf("Error reading bytes\n");
        return;
    }

    read(sd, &fileSize, sizeof(int));

    printf("Received file size from the server: %d\n", fileSize);

    // check if file exists in the server
    if (fileSize == 0)
    {
        if (wr_opcode(sd, NO) == -1)
        {
            printf("Cannot send ACK\n");
            return;
        }
        printf("%s does not exists on the current server directory.\n", fileName);
        return;
    }
    if (wr_opcode(sd, YES) == -1) // sends response to server to retrieve file
    {
        printf("Cannot send ACK!\n");
        return;
    }

    // allocate memory to the buffer
    fileBuffer = malloc(fileSize);
    printf("allocating memory\n");

    // open the file to be written
    if ((file = open(fileName, O_CREAT | O_EXCL | O_WRONLY, 0666)) < 0)
    {
        perror("open() Cannot open file for sending");
        return;
    }

    printf("opened file %d\n", file);

    // read and write loop
    int bytesRead, progress;
    int totalBytes = 0;
    if (fileSize > 0)
    {

        // it will stop if total bytes read is equivalent to file size
        while (totalBytes != fileSize)
        {
            // receive the file from the server.
            bytesRead = read(sd, fileBuffer, sizeof(fileBuffer));
            //printf("bytesRead = %d\n", bytesRead); //debug

            if (bytesRead < 0)
            {
                perror("read() Cannot read the file");
                return;
            }

            // write the file sent by the server
            if (write(file, fileBuffer, bytesRead) < 0)
            {
                perror("write() Cannot write the file received from the server");
                return;
            }

            totalBytes += bytesRead;
        }

        //printf("totalBytes = %d\n", totalBytes); //debug
        if (totalBytes == fileSize)
        {
            printf("%s download complete!\n", fileName);
        }
    }

    // close file
    close(file);
}

void put(int sd, char *tokens[])
{
    char fileName[256];
    char *fileBuffer;
    char ack;
    struct stat fileInfo;
    int file, fileSize;

    if (wr_opcode(sd, PUT_CODE) == -1)
    {
        printf("Cannot send command!\n");
        return;
    }

    // get the file name
    strcpy(fileName, tokens[1]);

    // send file name to server
    if (wr_twobyte(sd, strlen(fileName)) == -1) //send length of file name
    {
        printf("Cannot send bytes!\n");
        return;
    }
    if (wr_data(sd, fileName, sizeof(fileName)) == -1) //send file name
    {
        printf("Cannot send data!\n");
        return;
    }
    printf("Sent the file name to the server: %s\n", fileName);

    // get file information and its size
    stat(fileName, &fileInfo);
    fileSize = fileInfo.st_size;
    printf("File size is: %d\n", fileSize);

    // send the file size to the server
    if (wr_fourbyte(sd, fileSize) == -1)
    {
        printf("Cannot send bytes!\n");
        return;
    }
    write(sd, &fileSize, sizeof(int));

    // Allocate memory to the buffer
    fileBuffer = malloc(fileSize);

    // open the file to be read
    if ((file = open(fileName, O_RDONLY)) < 0)
    {
        perror("fopen() Cannot open file for sending");
    }

    if (re_opcode(sd, &ack) == -1)
    {
        printf("Cannot read ack!\n");
        return;
    }
    if (ack != PUT_SER)
    {
        printf("There was an error with sending the file!");
        return;
    }

    // read and write loop
    int bytesRead;
    int totalBytes = 0;
    if (fileSize > 0)
    {
        // read the file. the loop will stop when it reaches end of file.
        while ((bytesRead = read(file, fileBuffer, sizeof(fileBuffer))) != 0)
        {
            if (bytesRead < 0)
            {
                perror("read() Cannot read the file");
                return;
            }
            //printf("bytesRead = %d\n", bytesRead); //debug

            // send the file to the client
            if ((write(sd, fileBuffer, bytesRead)) < 0)
            {
                perror("write() Cannot send the file");
                break;
            }
        }

        if (bytesRead == 0)
        {
            printf("%s upload complete!\n", fileName);
        }
    }

    // close file
    close(file);
}