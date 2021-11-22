#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
my_atoi(const char *s, int *flg)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  //fprintf(1,"s = %d\n",*s);
  if(*s == '\0') //如果字符串s中有除了数字之外的字符，则报错
    *flg = 1;
  else
    *flg = 0;
  return n;
}

int main(int argc, char *argv[])
{
  if(argc<=1){
    fprintf(2,"error: need time arg\n");
    exit(1);
  }
  int flg = 1;
  int time = my_atoi(argv[1],&flg);
  //fprintf(1,"time %d, flg %d\n",time,flg);
  if(flg==0){
    fprintf(2,"error: need numeric arg\n");
    exit(1);
  }
  sleep(time);
  exit(0);
}