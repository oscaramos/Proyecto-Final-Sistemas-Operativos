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
#include <stack>
#include <thread>
#include <mutex>

#define MAX_BUF 1024
#define PIPE_P1toP2 "/tmp/pipe_P1toP2"
#define PIPE_P2toP1 "/tmp/pipe_P2toP1"

using namespace std;


int cnt_gen=1, cnt_send=1;

////////////////////////Pipe////////////////////////

int createPipe(const char* pipe_dir, int flag) // Escribe, envia
{
//    printf("Pipe [%s] flag [%d]\n", pipe_dir, flag);
    int fd;
    mkfifo(pipe_dir, 0666);
    fd = open(pipe_dir, flag); // O_WRONLY, O_RDONLY
//    printf("Terminando pipe\n");
    return fd;
}

// cnt_send = 1;
void sendDataPipe(int fd, int data)
{
    char msg[MAX_BUF];
    sprintf(msg, "%d", data); // Convierte data a char*
    write(fd, msg, strlen(msg)+1);
    printf("[%d]Envio %d\n", cnt_send++, data);
    // printf("DBG: gen=%d, send=%d, lost=%d\n", cnt_gen, cnt_send, cnt_gen-cnt_send);
}

/////////////////////////Stack/////////////////////////////

stack<int> stk; 
mutex mtx;

void pushStack(int data)
{
    mtx.lock();
      stk.push(data);
    mtx.unlock();
}

bool isEmptyStack()
{
    mtx.lock();
      bool ans = stk.empty();
    mtx.unlock();
    return ans;
}


int popTopStack() // get Top & pop
{
    mtx.lock();
      int top = stk.top();
      stk.pop();
    mtx.unlock();
    return top;
}

////////////////////////Error//////////////////////////////

void throwErrorMessage()
{
    printf("Error, no OK recibido\n");
    perror("Error, no OK recibido\n");
    exit(EXIT_FAILURE);
}

////////////////////////Threads////////////////////////////

// Thread 1: receiver signal
// Es el main()


// Thread anonimo
// cnt_gen = 1;
void generateData()
{
    int rn = rand()%100+1; // Nro random entre [1..100]
    printf("[%d]Genero %d\n", cnt_gen++, rn);
    printf("DBG: gen=%d, send=%d, lost=%d\n", cnt_gen, cnt_send, cnt_gen-cnt_send);
    pushStack(rn); 
}

// 
// Thread 2: send data
void sendData()
{
    char msg[MAX_BUF];
    int fd12, fd21;
    fd12 = createPipe(PIPE_P1toP2, O_WRONLY);
    fd21 = createPipe(PIPE_P2toP1, O_RDONLY);
    printf("Conexion Pipes establecida\n");
    while(true)
    {
        if(!isEmptyStack())
	{
	   int data = popTopStack(); // Get Tope y pop tope, como python
	   sendDataPipe(fd12, data);
           read(fd21, msg, MAX_BUF); // Aqui esperara
	}
	else
	{
           while(isEmptyStack()); // Espera haber elementos, busy wait!!, reemplazar con monitores
	}
    }
}

sig_atomic_t cdb = 0; // Tipo dato evita race condition 
void sigHandler(int signum/*, siginfo_t* info, void* old_sa*/)
{
    //  printf("%d\n", cdb++);
     thread t1(generateData);
     t1.detach();
    //  printf("%d\n", cdb--);
}

/*
void setSignal() // Problemas, puede que envie mas datos, o menos
{
   struct sigaction act;
   memset(&act, '\0', sizeof(act));
   act.sa_sigaction = &sigHandler;
   act.sa_flags = SA_SIGINFO; // Posible causa
   sigaction(SIGINT, &act, NULL);
}
*/

void printPID()
{
    printf("PID = %d\n", getpid());
}

// Thread 1
int main(int argc, char** argv)
{
    signal(SIGINT, sigHandler);
    printPID();
    thread t2(sendData);
    while(true) sleep(1); // Recibe signals   
    return 0;
}
