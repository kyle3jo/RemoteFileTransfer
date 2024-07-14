//protocol.c

#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include "protocol.h"

int wr_data(int sd, char *buf, int data)
{
    int i, w = 0;

    for (i = 0; i < data; i += w)
    {
        if ((w = write(sd, buf + i, data - i)) <= 0)
        {
            return (w); //error
        }
    }
    return i;
}

int re_data(int sd, char *buf, int data)
{
    int i, r = 1;

    for (i = 0; (i < data) && (r > 0); i += r)
    {
        if ((r = read(sd, buf + i, data - i)) < 0)
        {
            return (r); //error
        }
    }
    return (i);
}

int wr_opcode(int sd, char opcode)
{
    //writes opcode to the socket
    if (write(sd, (char *)&opcode, 1) != 1)
    {
        return (-1); //error
    }

    return 1;
}

int re_opcode(int sd, char *opcode)
{
    char buf;

    //reads opcode from socket
    if (read(sd, (char *)&buf, 1) != 1)
    {
        return -1; //error reading opcode
    }

    *opcode = buf;

    return 1;
}

int wr_twobyte(int sd, int twobyte)
{
    twobyte = htons(twobyte);

    if (write(sd, &twobyte, 2) != 2)
    {
        return -1;
    }

    return 1;
}

int re_twobyte(int sd, int *twobyte)
{
    short buf = 0;

    if (read(sd, &buf, 2) != 2)
    {
        return -1;
    }

    short host = ntohs(buf);
    *twobyte = (int)host;

    return 1;
}

int wr_fourbyte(int sd, int fourbyte)
{
    fourbyte = htonl(fourbyte);

    if (write(sd, &fourbyte, 4) != 4)
    {
        return -1;
    }

    return 1;
}

int re_fourbyte(int sd, int *fourbyte)
{

    int buf = 0;

    if (read(sd, &buf, 4) != 4)
    {
        return -1;
    }

    int host = ntohl(buf);
    *fourbyte = host;

    return 1;
}
