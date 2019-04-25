#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/' &&*p!='-'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

int checker(char* name,int mode){
  int len = strlen(name);
  if(len<4)
    return 1;
  if(name[0]=='c' && name[1]=='n' && name[3] == '-'){
    int val = name[2]-'0';
    if(val==mode){
      return 2;
    }
    else{
      return -1;
    }
  }else{
    return 3;
  }
}

void
ls(char *path,char* mode)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  int mod =0;
  if(strcmp(mode,"1")==0)
    mod = 1;
  if(strcmp(mode,"2")==0)
    mod = 2;
  if(strcmp(mode,"3")==0)
    mod = 3;

  printf(2,"\n\n\n  mode %d \n\n",mod );

  if((fd = open(path, 0)) < 0){
    printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf(1, "%s %d %d %d\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      int value = checker(de.name,mod);
      if(value<0)
        continue;

      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      printf(1, "%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  printf(1,"argc for ls is : %d \n",argc );
  if(argc < 2){
    ls(".",argv[0]);
    exit();
  }
  for(i=1; i<argc; i++)
    ls(argv[i],argv[0]);
  exit();
}
