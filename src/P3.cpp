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
#include <list>
#include <thread>
#include <mutex>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define SHMSZ 1024
#define MAX_BUF 1024
#define DATA_IPC_KEY 12321
#define OK_IPC_KEY 56789
#define DATA_SHM_KEY 54321
#define OK_SHM_KEY 98765

using namespace std;

list<int> scheduling;
mutex mtx;
///////////////////////SCHEDULING/////////////////////////
void pushData(int data)
{
    mtx.lock();
        scheduling.push_back(data);
        scheduling.sort();//Se ordena de menor a mayor por defecto
    mtx.unlock();
}
bool isEmpty()
{
    mtx.lock();
        bool ans = scheduling.empty();
    mtx.unlock();
    return ans;
}

int popTop()
{
    mtx.lock();
        int top = *(scheduling.begin());
        scheduling.pop_front();
    mtx.unlock();
    return top;
}
///////////////////////SHARED MEMORY//////////////////////

int createSHM(int shm_key)
{
    int shmid;
    int msgflg = IPC_CREAT | 0666;
    size_t size = SHMSZ;
    key_t key = shm_key;
    if((shmid = shmget(key,size,msgflg)) <= 0)
        printf("Fallo al crear SHM[%d], es posible que este ocupado\n", shm_key);
    else
        printf("Conexion SHM[%d] establecida\n",shm_key);
    return shmid;
}
int cnt_send = 1;
void sendDataSHM(int data, char *shmptr)
{
    char buf[MAX_BUF];
    sprintf(buf,"%d",data);
    char *sptr = shmptr;
    struct msgbuf sbuf;
    sbuf.mtype = 1;
    int c;
    printf("Se enviara el dato %s\n",buf);
    for(c = 0; c < strlen(buf); c++)
        *sptr++ = buf[c];///Enviando el dato SHM
    *sptr = '\n';
}
void waitOKSHM(char *shmptr)
{
    char *rptr = shmptr;
    while(*rptr == '*')
    {
        // printf("Esperando OK\n");
        // sleep(1);
    }
    // printf("Ok recibido\n");
    *shmptr='*';
}
////////////////////////IPC///////////////////////////////
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


void sendDataIPC(int msgid, char data[])
{
    struct msgbuf sbuf;
    sbuf.mtype = 1;
    strcpy(sbuf.mtext, data);
    int buflen = strlen(sbuf.mtext)+1;
    if (!(msgsnd(msgid, &sbuf, buflen, IPC_NOWAIT) < 0))
        printf("[%d]Enviado %d\n", cnt_send++, atoi(data));
    else
        printf ("Error, mensaje IPC no enviado!!\n"); // Puede que este lleno el ipc
}
void reciveData(int msgid,int msgidOk)
{
    struct msgbuf rbuf;
    rbuf.mtype = 2;
    while(1)
    {
        if((msgrcv(msgid,&rbuf, MAX_BUF,1,0)) < 0)
            printf("El mensaje no se recibio %s\n",rbuf.mtext);
        else
            printf("Se recibio el mensaje %s\n",rbuf.mtext);
            pushData(atoi(rbuf.mtext));
            sendDataIPC(msgidOk,"Ok");
    }
}

//////////////////////////Envio y Recepcion de Datos///////////////////////////////

void RecivePaquetes()
{
    // printf("Empieza Thread 2\n");
    int ipcRcv = createIPC(DATA_IPC_KEY);
    int ipcOk = createIPC(OK_IPC_KEY);
    reciveData(ipcRcv,ipcOk);
}
void SendPaquetes()
{
    // printf("Empieza Thread 1\n");
    int shmidData = createSHM(DATA_SHM_KEY);
    int shmidOk = createSHM(OK_SHM_KEY);
    char *sdata = (char *)shmat(shmidData,NULL,0);
    char *sok = (char *)shmat(shmidOk,NULL,0);
    while(1)
    {
        if(!isEmpty())
        {
            int data = popTop();
            printf("Se envia paquete %d\n",data);
            sendDataSHM(data,sdata);
            waitOKSHM(sok);//Aquí esperar
        }
        else
        {
            while(isEmpty());
        }
    }
}


int main(int argc, char **argv)
{
    thread t2(SendPaquetes);
    thread t3(RecivePaquetes);
    // printf("Todo bien\n");
    t2.join();
    t3.join();
    return 0;
}
