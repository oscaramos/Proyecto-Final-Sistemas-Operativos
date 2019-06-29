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

#define MAX_BUF 1024
#define DATA_SHM_KEY 54321
#define OK_SHM_KEY 98765
#define SHMSZ 1024

using namespace std;

//////////////////////////SCHEDULING//////////////////////

list<int> scheduling;
/////////////////////////////////////////////////////////

//////////////////////SHM////////////////////

int createSHM(int shm_key)
{
    int shmid;
    int msgflg = 0666;
    size_t size = SHMSZ;
    key_t key = shm_key;
    if((shmid = shmget(key,size,msgflg)) <= 0)
        printf("Fallo al crear SHM[%d], es posible que este ocupado\n",shm_key);
    else
        printf("Conexion SHM[%d] establecida\n",shm_key);
    return shmid;
}
void mostrarData();
void AdquirirData(char *&shmptr)
{
    char *sptr = shmptr;
    scheduling.push_back(atoi(sptr));
    scheduling.sort();
    scheduling.reverse();
    *shmptr='*';
    mostrarData();
}

void mostrarData()
{
    int data = *(scheduling.begin());
    scheduling.pop_front();
    printf("El dato enviado es %d\n",data);
}

void reciveData(char *&shmptr)
{
    while(*shmptr=='*');
    AdquirirData(shmptr);
}
void sendOkSHM(char data[], char *&shmptr)
{
    printf("Enviaremos el OK\n");
    char *sptr = shmptr;
    int c;
    for(c = 0; c < strlen(data); c++)
        *sptr++ = data[c];///Enviando el dato SHM
    printf("Ok enviado\n");
}
/////////////////////////////////////////////////////////////////

int main()
{
    int shmidData = createSHM(DATA_SHM_KEY);
    int shmidOk = createSHM(OK_SHM_KEY);
    char *sdata = (char *)shmat(shmidData,NULL,0);
    char *sok = (char *)shmat(shmidOk,NULL,0);
    while(1)
    {
        reciveData(sdata);
        sendOkSHM("Ok",sok);
    }
    return 0;
}
