#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include <stddef.h>
#define WE  1
#define RE  0
#define ENDPRIME 31
void primes(int *leftP){
  int num;
  int nowprime;
  close(leftP[WE]);
  read(leftP[RE],&num,sizeof(int));
  if(num==0){
    exit(0);
  }else{
    nowprime = num;
    printf("prime %d\n",num);
    int rightP[2];
    pipe(rightP);
    if(fork()==0){
      close(leftP[RE]);
      primes(rightP);
    }
    else{
      while(read(leftP[RE],&num,sizeof(int))){
        if(num%nowprime!=0){
         write(rightP[WE],&num,sizeof(int));
        }
      }
      close(rightP[RE]);
      close(rightP[WE]);
      wait((int*)0);
      exit(0);
    }
  }
}

int
main(int argc, char* argv[]){
   int p[2];
   pipe(p);
   int i;
   for(i=2;i<=ENDPRIME;i++){
     write(p[WE],&i,sizeof(int));
   }
   primes(p);
   exit(0);
}