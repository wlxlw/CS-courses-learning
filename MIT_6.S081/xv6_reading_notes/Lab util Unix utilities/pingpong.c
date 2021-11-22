/*使用2个管道实现全双工通信*/
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define WE  1
#define RE  0

int
main(int argc, char *argv[])
{
  int p[2][2];
  char* c = malloc(50);
  if(pipe(p[0])<0 || pipe(p[1])<0)
    fprintf(2,"failed to create pipe");

  if(fork()==0){//in child process
    close(p[1][RE]);
    close(p[0][WE]);
    char child_message[] = "pong";

    if(read(p[0][RE],c,4)==4){
    }
    else{
     fprintf(2,"error");
     exit(1);
    }
    close(p[0][RE]);
    printf("%d: received ping\n",getpid());

    write(p[1][WE],child_message,strlen(child_message));
    close(p[1][WE]);

    exit(0);
  }
  else{//in parent child
    close(p[1][WE]);
    close(p[0][RE]);
    char parent_message[] = "ping";

    write(p[0][WE],parent_message,strlen(parent_message));
    close(p[0][WE]);

    if(read(p[1][RE],c,4)==4){
    }
    else{
     fprintf(2,"error");
     exit(1);
    }
    close(p[1][RE]);

    printf("%d: received pong\n",getpid());
    exit(0);
  }
  
/*使用1个管道实现半双工通信
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define WE  1
#define RE  0

int
main(int argc, char *argv[])
{
  int p[2];
  char* c = malloc(50);
  if(pipe(p)<0)
    fprintf(2,"failed to create pipe");

  if(fork()==0){//in child process
    char child_message[] = "pong";
    if(read(p[RE],c,4)==4){
    }
    printf("%d: received ping\n",getpid());
    write(p[WE],child_message,strlen(child_message));
    exit(0);
  }
  else{//in parent child
    char parent_message[] = "ping";
    write(p[WE],parent_message,strlen(parent_message));
    if(read(p[RE],c,4)==4){
    }
    printf("%d: received pong\n",getpid());
    exit(0);
  }
}
}*/