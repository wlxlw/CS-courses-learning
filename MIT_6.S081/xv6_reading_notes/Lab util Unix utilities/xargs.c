#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define STDIN        0
#define MAXARG       32  // max exec argument's length

int
main(int argc, char* argv[]){
  //argv[0]是xargs命令 argv[1]是要多次执行的命令 剩余的是参数
  //复制argv到newargv
  char* newArgv[argc+1];
  int i = 0;
  //运行argv[1]命令所需的参数保存在newArgv+1
  for(i=1;i<argc;i++){
    newArgv[i]=argv[i];
  }
  char buf[MAXARG+1];
  newArgv[i] = buf;
  char* p = buf;
  while(read(STDIN,p,sizeof(char))){
    if(*p=='\n'){
      *p = '\0';
      if(fork()==0){
        exec(argv[1],newArgv+1);
        exit(0);
      }
      else{
        wait((int*)0);
        p = newArgv[i];
      }
    }
    p++;
  }
  exit(0);
}