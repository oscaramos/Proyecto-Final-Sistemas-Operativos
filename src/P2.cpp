#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <queue>
#include <thread>
#include <mutex>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_BUF 1024
#define PIPE_P1toP2 "/tmp/pipe_P1toP2"
#define PIPE_P2toP1 "/tmp/pipe_P2toP1"
#define IPC_KEY 12321

using namespace std;


////////////////////////Pipe////////////////////////


int createPipe(const char* pipe_dir, int flag) // Recibe, lee
{
// printf("Pipe [%s] flag [%d]\n", pipe_dir, flag);
    int fd;
    mkfifo(pipe_dir, 0666);
    fd = open(pipe_dir, flag); 
// printf("Terminando pipe\n");
    return fd;
}


////////////////////////IPC////////////////////////////
/*
struct msgbuf
{
    long    mtype;
    char    mtext[MAXBUF];
};
*/

int createIPC(int ipc_key) // Retorna msgid
{
    int msgid;
    int msgflg = IPC_CREAT | 0666;
    key_t key = ipc_key;
    if ((msgid = msgget(key, msgflg )) <= 0)
      printf("Fallo al crear ipc[%d]!!\n", ipc_key);
    else
      printf("Conexion IPC[%d] establecida\n",ipc_key);
    return msgid;
}

int cnt_send = 1;
void sendDataIPC(int msgid, char data[])
{
    struct msgbuf sbuf;
    sbuf.mtype = 1;
    strcpy(sbuf.mtext, data);
    int buflen = strlen(sbuf.mtext)+1;
    if (!(msgsnd(msgid, &sbuf, buflen, IPC_NOWAIT) < 0))
        printf("[%d]Enviado %d\n", cnt_send++, atoi(data));
    else
        printf ("Error, mensaje IPC no enviado!!: %d, %ld, %s, %d \n", 	msgid, sbuf.mtype, sbuf.mtext, (int)buflen); // Puede que este lleno el ipc
}


////////////////////////SubProcess////////////////////////////

int fd_sp12[2]; // fd del SubProceso 1 al 2 [dato]
int fd_sp21[2]; // fd del SubProceso 2 al 1 [ok]

int cnt_rec = 1;
// Sub process 1
void receiverData()
{
    char buf[MAX_BUF];
    close(fd_sp12[0]); // Cierro in
    close(fd_sp21[1]); // Cierro out
    
    int fd_12, fd_21;
    printf("Creacion pipes\n");
    fd_12 = createPipe(PIPE_P1toP2, O_RDONLY);
    fd_21 = createPipe(PIPE_P2toP1, O_WRONLY);
    printf("Conexion Pipes establecida\n");
    while(true)
    {
        read(fd_12, buf, MAX_BUF); // Espero de pipe
        printf("[%d]Recibo %s\n", cnt_rec++, buf); 
	write(fd_sp12[1], buf, strlen(buf)+1); // envio Dato a SP2
        write(fd_21, "OK", strlen("OK")+1); // envio OK a P1
	read(fd_sp21[0], buf, MAX_BUF); // espero ok de SP2
    }
}

// Sub process 2
void processData()
{
    char buf[MAX_BUF];
    close(fd_sp12[1]); // Cierro out
    close(fd_sp21[0]); // Cierro in

    int msgid = createIPC(IPC_KEY);
    while(true)
    {
       read(fd_sp12[0], buf, MAX_BUF); // Espero y obtengo dato
       sendDataIPC(msgid, buf);
       write(fd_sp21[1], "OK", strlen("OK")+1); // Envio OK a SP1 
    }
}

//////////////////////////////Main//////////////////////////////

int main(int argc, char** argv)
{
    printf("PID = %d\n", getpid());	
    pipe(fd_sp12);
    pipe(fd_sp21);
    int pid = fork();
    if(pid == 0) // child, SP1
        receiverData();
    else
	processData();// SP2
    return 0;
}
