// Container  user file

# include "types.h"
# include "user.h"
# include "date.h"

#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_CREATE  0x200

char buf[512];

void
cat(int fd)
{
  int n;

  while((n = read(fd, buf, sizeof(buf))) > 0) {
    if (write(1, buf, n) != n) {
      printf(1, "cat: write error\n");
      exit();
    }
  }
  if(n < 0){
    printf(1, "cat: read error\n");
    exit();
  }
}

void fileNameWithPid(int pid,char ar[]){
	// char ar[20];
	ar[0] = (char)'f';
	ar[1] = (char)'i';
	ar[2] = (char)'l';
	ar[3] = (char)'e';
	ar[4] = (char)'_';
	int size = 0,temp = pid,itr=0,val=0;
	for(;;){
		if(pid==0)
			break;
		pid /= 10;
		size++;
	}
	pid = temp;
	for(itr=0;itr<size;itr++){
		val = pid%10;
		pid /= 10;
		ar[4+size-itr] = (char)('0'+val);
	}

	// return ar;
}

void containerWithId(int id,char ar[]){
	if(id==0){
		ar[0] = ' ';
		ar[1] = '.';
		ar[2] = '\0';
	}else if(id>0){
		ar[0] = 'c';
		ar[1] = 'o';
		ar[2] = 'n';
		ar[3] = 't';
		ar[4] = 'a';
		ar[5] = 'i';
		ar[6] = 'n';
		ar[7] = 'e';
		ar[8] = 'r';
		ar[9] = '_';
	}

	if(id>0){
		if(id==1)
			ar[10] = '1';
		else if(id==2)
			ar[10] = '2';
		else 
			ar[10] = '3';
		ar[11] = '\0';
	}
}

void catstring(char ar[],int pid){
	int size = 0,temp = pid,itr=0,val=0;
	for(;;){
		if(pid==0)
			break;
		pid /= 10;
		size++;
	}
	pid = temp;
	int shift= strlen(ar);
	for(itr=0;itr<size;itr++){
		val = pid%10;
		pid /= 10;
		ar[shift+size-itr] = (char)('0'+val);
	}
}

int
main ( int argc , char * argv [])
{

	printf(1,"\n\n Started Container Manager \n");

	int id = create_container();
	int id2 = create_container();
	int id3 = create_container();

	// printf(1,"Id : %d\n",id );
	// printf(1,"Id : %d\n",id2 );
	// printf(1,"Id : %d\n",id3 );

	// exit();
	int parentPid = getpid();
	printf(1,"Parets PID: %d \n",parentPid);
	// fork test here added to container 1
	int itr =0,forkVal;
	
	for(itr=0;itr<3;itr++){
		forkVal = fork();
		if(forkVal == 0){
			join_container(id);
			break;
		}	
	}

	// add to container 2 and 3
	if(parentPid == getpid()){
		forkVal = fork();
		if(forkVal == 0){
			// printf(1,"pid for container 2 :: %d\n",forkVal );
			join_container(id2);
		}
	}
	if(parentPid == getpid()){
		forkVal = fork();
		if(forkVal == 0){
			join_container(id3);
		}
	}


	int pidCurProc = getpid();
	int procContainer = proc_container(pidCurProc);
	// printf(1, "In Process :: %d belong to container :: %d \n",pidCurProc,procContainer);
	// ps olny by one
	// create sys call to get  one pid for given container ID.
	// if (getpid() == pspid(containerId))
	if(parentPid != pidCurProc){

		//------------------ PROCESS ISOLATION ---------------------
		if(proc_container_num(pidCurProc) == 3 && procContainer == 1){
			ps();
		}
		else if(proc_container_num(pidCurProc) == 1 && procContainer != 1){
			ps();
		}

		// // BARRIER TILL ALL process join container
		// while(containerProcessNum(id)!= 3 || containerProcessNum(id2)!=1 || containerProcessNum(id3)!=1){

		// }
		// for(;;){

		// }
		for(;;){
			if(check_schedule_log(1))
				break;
		}
		for(;;){
			if(check_schedule_log(0))
				break;
		}

		printf(1,"pid of process %d of Container %d \n",pidCurProc,procContainer);
		// for(;;)
		// 	if(check_memory_log(1)==1)
		// 		break;
		// exit();

		for(;;)
			if(check_memory_log(1)==1)
				break;
			// else
				// printf(1,"pid is stuck\n",getpid() );
			// else
				// printf(1,"memory_log_on : %d \n",check_memory_log(1) );
		// leave_container();
		// char* s = "file_"+pidCurProc;
		char s[20];
		fileNameWithPid(pidCurProc,s);
		int fd =open(s,O_CREATE | O_RDWR);
		if(fd >= 0) {
  	      printf(1, "ok: create file succeed\n");
	    } else {
	        printf(1, "error: create backup file failed\n");
	        exit();
	    }

	    close(fd);

	    file_creation_log(procContainer);

	    for(;;){
	    	if(check_file_creation(procContainer)==3 && procContainer == 1){
	    		break;
	    	}
	    	if(check_file_creation(procContainer)==1 && procContainer != 1){
	    		break;
	    	}
	    }

		// printf(1,"file created with pid %s \n",s);
		// char s[20];
		// containerWithId(procContainer,s);
		// if(procContainer==1){
		// 	char *argv_c[] = {"1"};
		// 	exec("ls_new",argv_c);
		// }
		// else if(procContainer==2){
		// 	char *argv_c[] = {"2"};
		// 	exec("ls_new",argv_c);
		// }
		// else if(procContainer==3){
		// 	char *argv_c[] = {"3"};
		// 	exec("ls_new",argv_c);
		// }
		// void *m = container_malloc()
		// for(itr=0;itr<5;itr++)
		// 	wait();

		// printf(1,"start ls  %d \n",getpid());
		// int pid = fork();
		// if(pid==0){
		// 	char *argv_c[] = {"0"," fdj"};

		// 	printf(1,"ls not running here %d \n",getpid() );
		// 	exec("ls",argv_c);
		// 	printf(1,"ls ----- here %d \n",getpid() );
			
		// }
		// else {
		// 	printf(1,"fork pid : %d\n",pid );
		// 	wait();
		// }

		// printf(1,"end ls\n");

		// exit();
		// create("file_"+pidCurProc);

			for(;;)
			if(check_memory_log(1)==2)
				break;

		if(proc_container_num(pidCurProc) == 3 && procContainer == 1){
			// printf(1,"---------------------------------------------\n");
			// int pid = fork();
			// if(pid==0){
			// 	char *argv_c[] = { " ","." };
			// 	exec("ls",argv_c);
			// }else{
			// 	wait();
			// }
			// char s[20];
			// fileNameWithPid(pidCurProc,s);
			// struct writer t;
		    // t.name = ;
		    // t.number = 1;

			fd =open("my_file",O_CREATE | O_RDWR);
			if(fd >= 0) {
	  	      printf(1, "ok: create file succeed\n");
		    } else {
		        printf(1, "error: create backup file failed\n");
		        exit();
		    }

		    char t[20] = "Modified by : ";
		    catstring(t,pidCurProc);


		    int size = sizeof(t);
		    if(write(fd,t, size) != size){
		        printf(1, "error: write to backup file failed\n");
		        exit();
		    }

		    close(fd);
		    printf(1, "write ok\n");

		    memory_log_on();


		}else if(proc_container_num(pidCurProc) == 1 && procContainer != 1){
			// int pid = fork();
			// if(pid==0){
			// 	char *argv_c[] = { " ","." };
			// 	exec("ls",argv_c);
			// }else{
			// 	wait();
			// }
			fd =open("my_file",O_CREATE | O_RDWR);
			if(fd >= 0) {
	  	      printf(1, "ok: create file succeed\n");
		    } else {
		        printf(1, "error: create backup file failed\n");
		        exit();
		    }

			char t[20] = "Modified by : ";
		    catstring(t,pidCurProc);


		    int size = sizeof(t);
		    if(write(fd,t, size) != size){
		        printf(1, "error: write to backup file failed\n");
		        exit();
		    }
		    close(fd);

		    memory_log_on();
		    printf(1, "write ok\n");
		}

		// leave_container();
		for(;;){
			if(check_memory_log(1)==5)
				break;
		}



	    // see it works
	    fd = open("my_file",O_RDWR);
	    cat(fd);
	    close(fd);
		// void *m = container_malloc()
		// exit();
		leave_container();
		// create("file_"+pidCurProc);
		

	}else{

		/* - - - - - - - - - - - - - - - - SCHEDULER TEST - - - - - - - - - - - - - - - - - - - - - - - - */
		while(containerProcessNum(id)!= 3 || containerProcessNum(id2)!=1 || containerProcessNum(id3)!=1){

		}
		printf(1,"proc counts :: %d  :  %d  :  %d \n",containerProcessNum(id),containerProcessNum(id2),containerProcessNum(id3));
		scheduler_log_on ();

		// barrier for scheduler log
		for(;;){
			if(check_schedule_log(0))
				break;
		}

		// int itr=0;
		for(itr=0;itr<5;itr++)
			wait();

		int pid = fork();
		if(pid==0){
			char *argv_c[] = { " ","." };
			exec("ls",argv_c);
		}else{
			wait();
		}
		// befor leaving this section  on memory logs
		memory_log_on();


		pid = fork();
		if(pid==0){
			char *argv_c[] = { "1" };
			exec("ls_new",argv_c);
		}else{
			wait();
		}

		pid = fork();
		if(pid==0){
			char *argv_c[] = { "2" };
			exec("ls_new",argv_c);
		}else{
			wait();
		}

		pid = fork();
		if(pid==0){
			char *argv_c[] = { "3" };
			exec("ls_new",argv_c);
		}else{
			wait();
		}

		memory_log_on();

		
		for(itr=0;itr<5;itr++)
			wait();

		// leave_container();
		// for(;;){
		// 	if(check_memory_log(1)==5)
		// 		break;
		// }


	}


	if(parentPid == pidCurProc){
		// leave_container();
		for(;;){
			if(isAllEnded()){
				break;
			}
		}

		destroy_container(id);
		destroy_container(id2);
		destroy_container(id3);
	}

	exit();
}
