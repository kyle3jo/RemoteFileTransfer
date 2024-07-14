//#define TCP_PORT 41189
#define FILE_SIZE 512

#define PUT_CLI 'A'
#define PUT_SER 'B'
#define PUT_ERR 'Z'

#define GET_CLI 'C'
#define GET_SER 'D'

#define YES 'Y'
#define NO 'N'

#define CD_CODE 'E'

#define DIR_CODE 'F'

#define PWD_CODE 'P'

#define GET_CODE 'C'

#define PUT_CODE 'A'

int wr_data(int sd, char *buf, int data);

int re_data(int sd, char *buf, int data);

int wr_opcode(int sd, char opcode);

int re_opcode(int sd, char *opcode);

int wr_twobyte(int sd, int twobyte);

int re_twobyte(int sd, int *twobyte);

int wr_fourbyte(int sd, int fourbyte);

int re_fourbyte(int sd, int *fourbyte);
