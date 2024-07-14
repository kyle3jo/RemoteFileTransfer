#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "protocol.h"

#define TCP_PORT 41189

void claim_children();
void daemon_init(void);
void appendLog(FILE *log);

void server_client(int nsd, char clientAddress[], int clientPort, FILE *log);
void pwd(int nsd, char clientAddress[], int clientPort, FILE *log);
void server_dir(int nsd, char clientAddress[], int clientPort, FILE *log);
void server_cd(int nsd, char clientAddress[], int clientPort, FILE *log);
void get(int nsd, char clientAddress[], int clientPort, FILE *log);
void put(int nsd, char clientAddress[], int clientPort, FILE *log);

int main(int argc, char *argv[])
{
    if (argc == 2) // allows server to be run in specified directory
    {
        chdir(argv[1]);
    }

    FILE *log = NULL; // file for server log

    unsigned short port;
    int sd, nsd;
    socklen_t cli_addrlen;
    struct sockaddr_in serv_ad, cli_ad;
    pid_t pid;

    // 0. Setup log file

    // open log file for writing
    log = fopen("log.txt", "w");
    if (log == NULL)
    {
        perror("fopen() Cannot open log file: ");
    }
    fclose(log);

    // 1. Define port number

    port = TCP_PORT;

    // 2. Become daemon

    daemon_init();

    // 3. Create socket to listen

    // log
    appendLog(log);
    fprintf(log, "Creating socket.\n");
    fclose(log);

    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        // log - creation of socket failed
        perror("Error. Creating server socket");
        appendLog(log);
        fprintf(log, "Failed to create server socket.\n");
        fclose(log);
        exit(1);
    }

    // 4. Build server socket

    // log
    appendLog(log);
    fprintf(log, "Building socket.\n");
    fclose(log);

    bzero((char *)&serv_ad, sizeof(serv_ad));
    serv_ad.sin_family = AF_INET;
    serv_ad.sin_port = htons(port);
    serv_ad.sin_addr.s_addr = htonl(INADDR_ANY);

    // 5. Bind address to socket

    // log
    appendLog(log);
    fprintf(log, "Binding socket.\n");
    fclose(log);

    if (bind(sd, (struct sockaddr *)&serv_ad, sizeof(serv_ad)) < 0)
    {
        // log - binding of socket failed
        perror("Error. Failed to bind server socket");
        appendLog(log);
        fprintf(log, "Failed to bind server socket.\n");
        fclose(log);
        exit(1);
        ;
    }

    // log - socket create and binding succesful
    appendLog(log);
    fprintf(log, "Server sucessfully started.\n");
    fclose(log);

    // 6. Become the listening socket

    if (listen(sd, 5) == 0)
    {
        //log - server listening
        appendLog(log);
        fprintf(log, "Server is listening..\n");
        fclose(log);
    }

    // 7. Connect with client

    while (1) // infinte loop
    {
        cli_addrlen = sizeof(cli_ad); // accept request from client
        nsd = accept(sd, (struct sockaddr *)&cli_ad, (socklen_t *)&cli_ad);
        if (nsd < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            exit(1);
        }

        // log - client connection
        appendLog(log);
        fprintf(log, "Client (%s:%d) succesfully connected to the server.\n", inet_ntoa(cli_ad.sin_addr), ntohs(cli_ad.sin_port));
        fclose(log);

        // process to create a child for each client->
        if ((pid = fork()) < 0)
        {
            perror("fork");
            exit(1);
        }
        else if (pid > 0)
        {
            close(nsd);
            continue; // parent process waits for next client
        }
        // child process continues to server client
        close(sd);

        server_client(nsd, inet_ntoa(cli_ad.sin_addr), ntohs(cli_ad.sin_port), log);
    }

    // log
    appendLog(log);
    fprintf(log, "Server has ended.\n");
    fclose(log);
    exit(0);
}

void server_client(int nsd, char clientAddress[], int clientPort, FILE *log)
{
    int nr, nw, i = 0;
    char cmd[1024 * 5]; // max cmd
    char cmd2[1024 * 5];
    char spwd[1024 * 5];
    char spwd2[1024 * 5];
    DIR *cwd;
    struct dirent *entry;
    int files = 0;

    while (++i)
    {
        // nr = readn(sd, cmd, sizeof(cmd));
        char op;
        cmd[0] = '\0';
        re_opcode(nsd, &op);

        // log
        appendLog(log);
        fprintf(log, "Client (%s:%d) sent one byte code '%c'.\n", clientAddress, clientPort, op);
        fclose(log);

        if (op == PWD_CODE)
        {
            // log
            appendLog(log);
            fprintf(log, "Client (%s:%d) requested command 'pwd'.\n", clientAddress, clientPort);
            fclose(log);

            pwd(nsd, clientAddress, clientPort, log);
        }
        else if (op == DIR_CODE)
        {
            // log
            appendLog(log);
            fprintf(log, "Client (%s:%d) requested command 'dir'.\n", clientAddress, clientPort);
            fclose(log);

            server_dir(nsd, clientAddress, clientPort, log);
        }
        else if (op == CD_CODE)
        {
            // log
            appendLog(log);
            fprintf(log, "Client (%s:%d) requested command 'cd'.\n", clientAddress, clientPort);
            fclose(log);

            server_cd(nsd, clientAddress, clientPort, log);
        }

        else if (op == GET_CODE)
        {
            // log
            appendLog(log);
            fprintf(log, "Client (%s:%d) requested command 'get'.\n", clientAddress, clientPort);
            fclose(log);

            get(nsd, clientAddress, clientPort, log);
        }

        else if (op == PUT_CODE)
        {
            // log
            appendLog(log);
            fprintf(log, "Client (%s:%d) requested command 'put'.\n", clientAddress, clientPort);
            fclose(log);

            put(nsd, clientAddress, clientPort, log);
        }

        else
        {
            exit(0);
        }
    }
}

// method to claim children processes: (provided by Hong's examples)
void claim_children()
{
    pid_t pid = 1;
    while (pid > 0) // Claims all zombies
    {
        pid = waitpid(0, (int *)0, WNOHANG);
    }
}

// method to make process daemon: (provided by Hong's examples)
void daemon_init(void)
{
    pid_t pid;
    struct sigaction act;

    if ((pid = fork()) < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (pid > 0)
    {
        printf("Daemon PID: %d\n", pid);
        exit(0);
    }

    // child process continues...
    setsid(); // becomes the session leader
    // chdir("/"); //changes working directory
    umask(0); // clears file mode creation mask

    // this will catch SIGCHLD so that we can claim with 'claim_children()'
    act.sa_handler = claim_children;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGCHLD, (struct sigaction *)&act, (struct sigaction *)0);
}

// method to fopen log file for appending
void appendLog(FILE *log)
{

    fflush(log);
    // open log file for appending
    log = fopen("log.txt", "a");
    if (log == NULL)
    {
        perror("fopen() Cannot open log file: ");
    }
}

// method(s) for serving clients?

void pwd(int nsd, char clientAddress[], int clientPort, FILE *log)
{
    char cwd[1024 * 5];
    getcwd(cwd, sizeof(cwd));

    if (wr_opcode(nsd, PWD_CODE) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): '%c' (command: pwd).\n", clientAddress, clientPort, PWD_CODE);
        fclose(log);

        return;
    }
    // log
    appendLog(log);
    fprintf(log, "Server responded to Client (%s:%d): '%c' (command: pwd).\n", clientAddress, clientPort, PWD_CODE);
    fclose(log);

    if (wr_twobyte(nsd, strlen(cwd)) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): length of current directory (command: pwd).\n", clientAddress, clientPort);
        fclose(log);

        //error in sending length of cwd
        return;
    }
    // log
    appendLog(log);
    fprintf(log, "Server responded to Client (%s:%d): length of current directory (command: pwd).\n", clientAddress, clientPort);
    fclose(log);

    if (wr_data(nsd, cwd, strlen(cwd)) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): name of working directory (command: pwd).\n", clientAddress, clientPort);
        fclose(log);

        //error sending cwd
        return;
    }
    // log
    appendLog(log);
    fprintf(log, "Server responded to Client (%s:%d): name of working directory (command: pwd).\n", clientAddress, clientPort);
    fclose(log);

    // log
    appendLog(log);
    fprintf(log, "Client (%s:%d) succesfully used 'pwd'.\n", clientAddress, clientPort);
    fclose(log);
}

void server_dir(int nsd, char clientAddress[], int clientPort, FILE *log)
{
    char cmd2[2000];
    cmd2[0] = '\0';
    int files = 0;
    char spwd2[1024 * 5];
    DIR *cwd;
    struct dirent *entry;

    getcwd(spwd2, 1024 * 5);
    cwd = opendir(spwd2);

    cmd2[0] = '\0';
    while ((entry = readdir(cwd)))
    {
        files++;
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            strcat(cmd2, entry->d_name);
            strcat(cmd2, " ");
        }
    }
    closedir(cwd);

    if (wr_opcode(nsd, DIR_CODE) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): '%c' (command: dir).\n", clientAddress, clientPort, DIR_CODE);
        fclose(log);

        return;
    }
    // log
    appendLog(log);
    fprintf(log, "Server responded to Client (%s:%d): '%c' (command: dir).\n", clientAddress, clientPort, DIR_CODE);
    fclose(log);

    if (wr_fourbyte(nsd, strlen(cmd2)) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): length of current directory (command: dir).\n", clientAddress, clientPort);
        fclose(log);

        return;
    }
    // log
    appendLog(log);
    fprintf(log, "Server responded to Client (%s:%d): length of current directory (command: dir).\n", clientAddress, clientPort);
    fclose(log);

    if (wr_data(nsd, cmd2, strlen(cmd2)) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): list of directory (command: dir).\n", clientAddress, clientPort);
        fclose(log);

        return;
    }
    // log
    appendLog(log);
    fprintf(log, "Server responded to Client (%s:%d): list of directory (command: dir).\n", clientAddress, clientPort);
    fclose(log);

    // log
    appendLog(log);
    fprintf(log, "Client (%s:%d) succesfully used 'dir'.\n", clientAddress, clientPort);
    fclose(log);
}

void server_cd(int nsd, char clientAddress[], int clientPort, FILE *log)
{
    int ack;
    int buf;

    if (re_twobyte(nsd, &buf) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): length of directory name (command: cd).\n", clientAddress, clientPort);
        fclose(log);

        return;
    }
    // log
    appendLog(log);
    fprintf(log, "Server responded to Client (%s:%d): length of directory name (command: cd).\n", clientAddress, clientPort);
    fclose(log);

    char path[buf + 1];

    if (re_data(nsd, path, buf) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): directory name (command: cd).\n", clientAddress, clientPort);
        fclose(log);

        return;
    }
    // log
    appendLog(log);
    fprintf(log, "Server responded to Client (%s:%d): directory name (command: cd).\n", clientAddress, clientPort);
    fclose(log);

    if (wr_opcode(nsd, CD_CODE) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): '%c' (command: cd).\n", clientAddress, clientPort, CD_CODE);
        fclose(log);

        return;
    }
    // log
    appendLog(log);
    fprintf(log, "Server responded to Client (%s:%d): '%c' (command: cd).\n", clientAddress, clientPort, CD_CODE);
    fclose(log);

    path[buf] = '\0';

    if (chdir(path) == 0)
    {
        // log
        appendLog(log);
        fprintf(log, "Server responded to Client (%s:%d): 0, directory changed successfully. (command: cd).\n", clientAddress, clientPort);
        fclose(log);

        wr_twobyte(nsd, 4);
    }

    else
    {
        // log
        appendLog(log);
        fprintf(log, "Server responded to Client (%s:%d): -1, directory failed to change. (command: cd).\n", clientAddress, clientPort);
        fclose(log);

        wr_twobyte(nsd, 400000);
    }

    // log
    appendLog(log);
    fprintf(log, "Client (%s:%d) succesfully used 'cd'.\n", clientAddress, clientPort);
    fclose(log);
}

void get(int nsd, char clientAddress[], int clientPort, FILE *log)
{
    char fileName[256];
    char *fileBuffer;
    char ack;
    struct stat fileInfo;
    int file, fileSize, nameLength;

    // get file name from client
    if (re_twobyte(nsd, &nameLength) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): file name length. (command: get).\n", clientAddress, clientPort);
        fclose(log);

        return;
    }
    // log
    appendLog(log);
    fprintf(log, "Server responded to Client (%s:%d): file name length. (command: get).\n", clientAddress, clientPort);
    fclose(log);

    if (re_data(nsd, fileName, sizeof(fileName)) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): file name. (command: get).\n", clientAddress, clientPort);
        fclose(log);

        return;
    }
    // get file information and its size
    stat(fileName, &fileInfo);
    fileSize = fileInfo.st_size;
    // log
    appendLog(log);
    fprintf(log, "Server received file name from Client (%s:%d): %s (command: get)\n", clientAddress, clientPort, fileName);
    fclose(log);

    // send the file size to the client
    if (wr_fourbyte(nsd, fileSize) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): four bytes, file size. (command: get).\n", clientAddress, clientPort);
        fclose(log);

        return;
    }
    write(nsd, &fileSize, sizeof(int));
    // log
    appendLog(log);
    fprintf(log, "Server sent the file size to Client (%s:%d): %d (command: get)\n", clientAddress, clientPort, fileSize);
    fclose(log);

    if (re_opcode(nsd, &ack) == -1) // reads ack from client
    {
        return;
    }
    // Allocate memory to the buffer
    fileBuffer = malloc(fileSize);

    // open the file to be read
    if ((file = open(fileName, O_RDONLY)) < 0)
    {
        perror("fopen() Cannot open file for sending");
        // log
        appendLog(log);
        fprintf(log, "Server failed to open file for Client (%s:%d): %s. (command: get).\n", clientAddress, clientPort, fileName);
        fclose(log);

        return;
    }

    // log
    appendLog(log);
    fprintf(log, "Server is reading and sending file to Client (%s:%d): %d (command: get)\n", clientAddress, clientPort, fileSize);
    fclose(log);
    // read and write loop
    int bytesRead, progress;
    int totalBytes = 0;
    if (fileSize > 0)
    {
        // read the file. the loop will stop when it reaches end of file.
        while ((bytesRead = read(file, fileBuffer, sizeof(fileBuffer))) != 0)
        {
            if (bytesRead < 0)
            {
                perror("read() Cannot read the file");
                // log
                appendLog(log);
                fprintf(log, "Server failed to read the file for Client (%s:%d): %s. (command: get).\n", clientAddress, clientPort, fileName);
                fclose(log);

                return;
            }
            //printf("bytesRead = %d\n", bytesRead); //debug

            // send the file to the client
            if ((write(nsd, fileBuffer, bytesRead)) < 0) //write fourbyte
            {
                perror("write() Cannot send the file");
                // log
                appendLog(log);
                fprintf(log, "Server failed to send the file for Client (%s:%d): %s. (command: get).\n", clientAddress, clientPort, fileName);
                fclose(log);
                break;
            }
        }

        if (bytesRead == 0)
        {
            // log
            appendLog(log);
            fprintf(log, "Server successfully sent the file to Client (%s:%d): %s. (command: get).\n", clientAddress, clientPort, fileName);
            fclose(log);
        }
    }

    // close file
    close(file);

    // log
    appendLog(log);
    fprintf(log, "Client (%s:%d) succesfully used 'get'.\n", clientAddress, clientPort);
    fclose(log);
}

void put(int nsd, char clientAddress[], int clientPort, FILE *log)
{
    char op, ack;
    short buf;

    int fileSize, fileHandle;
    char *fileBuffer;
    char fileName[256];
    int file;

    // get file name from client
    if (re_twobyte(nsd, &fileHandle) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): file name length. (command: put).\n", clientAddress, clientPort);
        fclose(log);

        return;
    }
    if (re_data(nsd, fileName, sizeof(fileName)) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): file name. (command: put).\n", clientAddress, clientPort);
        fclose(log);

        return;
    }

    printf("Received file name from client: %s\n", fileName);

    // get file size from the client
    if (re_fourbyte(nsd, &fileSize) == -1)
    {
        // log
        appendLog(log);
        fprintf(log, "Server failed to respond to Client (%s:%d): file size. (command: put).\n", clientAddress, clientPort);
        fclose(log);

        return;
    }
    read(nsd, &fileSize, sizeof(int));
    printf("Received file size from the client: %d\n", fileSize);

    // check if file exists in the client
    if (fileSize == 0)
    {
        //change this so it prints to client
        if (wr_opcode(nsd, PUT_ERR) == -1)
        {
            // log
            appendLog(log);
            fprintf(log, "Server responded to Client (%s:%d): file does not exist (command: put).\n", clientAddress, clientPort);
            fclose(log);
            return;
        }
    }
    // log
    appendLog(log);
    fprintf(log, "Server responded to Client (%s:%d): file exists (command: put).\n", clientAddress, clientPort);
    fclose(log);

    // allocate memory to the buffer
    fileBuffer = malloc(fileSize);
    printf("allocating memory\n");

    // open the file to be written
    if ((file = open(fileName, O_CREAT | O_EXCL | O_WRONLY, 0666)) < 0)
    {
        if (wr_opcode(nsd, PUT_ERR) == -1)
        {
            perror("open() Cannot open file for sending");
            // log
            appendLog(log);
            fprintf(log, "Server failed to open file for Client (%s:%d): %s. (command: put).\n", clientAddress, clientPort, fileName);
            fclose(log);
            return;
        }
    }
    if (wr_opcode(nsd, PUT_SER) == -1)
    {
        return;
    }

    // log
    appendLog(log);
    fprintf(log, "Server is reading and writing file from Client (%s:%d): %d (command: put)\n", clientAddress, clientPort, fileSize);
    fclose(log);
    // read and write loop
    int bytesRead;
    int totalBytes = 0;
    if (fileSize > 0)
    {

        // it will stop if total bytes read is equivalent to file size
        while (totalBytes != fileSize)
        {
            // receive the file from the client.
            bytesRead = read(nsd, fileBuffer, sizeof(fileBuffer));
            //printf("bytesRead = %d\n", bytesRead); //debug

            if (bytesRead < 0)
            {
                perror("read() Cannot read the file");
                // log
                appendLog(log);
                fprintf(log, "Server failed to read the file from Client (%s:%d): %s. (command: get).\n", clientAddress, clientPort, fileName);
                fclose(log);
                return;
            }

            // write the file
            if (write(file, fileBuffer, bytesRead) < 0)
            {
                perror("write() Cannot write the file received from the server");
                // log
                appendLog(log);
                fprintf(log, "Server failed to write the file from Client (%s:%d): %s. (command: get).\n", clientAddress, clientPort, fileName);
                fclose(log);
                return;
            }

            totalBytes += bytesRead;
        }

        //printf("totalBytes = %d\n", totalBytes); //debug
        if (totalBytes == fileSize)
        {
            // log
            appendLog(log);
            fprintf(log, "Server successfully received the file from Client (%s:%d): %s. (command: get).\n", clientAddress, clientPort, fileName);
            fclose(log);
        }
    }

    // close file
    close(file);

    // log
    appendLog(log);
    fprintf(log, "Client (%s:%d) succesfully used 'put'.\n", clientAddress, clientPort);
    fclose(log);
}