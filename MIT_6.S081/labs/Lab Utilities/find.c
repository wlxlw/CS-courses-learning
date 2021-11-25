#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


void dfs(char*path,char*filename){
  struct dirent de;
  struct stat st;
  char buf[512],*p;
  int fd;
  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }


  strcpy(buf, path);//path拷贝到buff
  p = buf+strlen(buf);
  *p++ = '/';//在目录字符串末尾加/


  while(read(fd, &de, sizeof(de)) == sizeof(de)){//
    if(de.inum == 0)//dirent的inum为0表示这一项没有使用直接跳过
      continue;
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;

    //buf指向的字符串是完整的路径

    if(stat(buf, &st) < 0){
      printf("ls: cannot stat %s\n", buf);
      continue;
    }
    if(!strcmp(de.name,".") || !strcmp(de.name,".."))//如果文件是.或者.. 则跳过
      continue;

    if(st.type==T_DIR){//如果是目录继续往里找
      dfs(buf,filename);
    }
    else if(st.type==T_FILE && !strcmp(de.name,filename)){
      printf("%s\n",buf);
    }
  }
}


int
main(int argc, char *argv[])
{

  if(argc < 3){
    exit(1);
  }
  dfs(argv[1],argv[2]);
  exit(0);
}