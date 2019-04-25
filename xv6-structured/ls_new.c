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
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
ls(char *path,char *mode)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  
  // if(strcmp(mode,"0")==0)
  //   path = ".";
  // if(strcmp(mode,"1")==0)
  //   path = "container_1";
  // if(strcmp(mode,"2")==0)
  //   path = "container_2";
  // if(strcmp(mode,"3")==0)
  //   path = "container_3";


  printf(1,"container ls by id : %d\n",proc_container(getpid()) );

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
      memmove(p, de.name, DIRSIZ);
      if(strcmp(de.name,"container_1")==0)
        continue;
      if(strcmp(de.name,"container_2")==0)
        continue;
      if(strcmp(de.name,"container_3")==0)
        continue;
      // printf(1,"-------- %s  -----------\n",de.name );
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      printf(1, "%s %d %d %d %d\n", fmtname(buf), st.type, st.ino, st.size,st.containerID);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  printf(1,"argc for ls is : %d  %s\n",argc,argv[0] );
  if(argc < 2){
    ls(".",argv[0]);
    exit();
  }
  for(i=1; i<argc; i++)
    ls(argv[i],argv[0]);
  exit();
}
