// Container  user file

# include "types.h"
# include "user.h"
# include "date.h"

#define O_CREATE  0x200
#define O_RDWR    0x002


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

int
main ( int argc , char * argv [])
{

	printf(1,"\n\n Started Container Manager \n");

	int id = create_container();

	// create mkdir
	// char* arg[] = {" " ,"container_1"};
	// int pid = fork();
	// if(pid==0)
	// 	exec("mkdir",arg);
	// else
	// 	wait();

	int id2 = create_container();
	// char *arg2[] = {" " ,"container_2"};
	// pid = fork();
	// if(pid==0)
	// 	exec("mkdir",arg2);
	// else
	// 	wait();


	int id3 = create_container();
	// char *arg3[] = {" " ,"container_3"};
	// pid = fork();
	// if(pid==0)
	// 	exec("mkdir",arg3);
	// else
	// 	wait();
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
			wait();
			break;
		}	
	}

	// add to container 2 and 3
	if(parentPid == getpid()){
		forkVal = fork();
		if(forkVal == 0){
			// printf(1,"pid for container 2 :: %d\n",forkVal );
			join_container(id2);
			wait();

		}
	}
	if(parentPid == getpid()){
		forkVal = fork();
		if(forkVal == 0){
			join_container(id3);
			wait();

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
		// leave_container();
		// char* s = "file_"+pidCurProc;
		char s[20];
		fileNameWithPid(pidCurProc,s);
		open(s,O_CREATE | O_RDWR);
		// if(fd >= 0) {
  //       printf(1, "ok: create file succeed\n");
	 //    } else {
	 //        printf(1, "error: create backup file failed\n");
	 //        exit();
	 //    }

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

		printf("start ls\n");
		char *argv_c[] = {"0"};
		int pid = fork();
		if(pid==0)
			exec("ls_new",argv_c);
		else 
			wait();

		printf("end ls\n");

		// exit();
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

		wait();

		// char *argv_c[] = {"0"};
		// exec("ls_new",argv_c);
		// befor leaving this section  on memory logs
		// memory_log_on();

		// for(;;)
		// 	if(check_memory_log(0))
		// 		break;

	}


	if(parentPid == pidCurProc){

		// wait();
		// printf(1,"deleting files here \n");
		// // leave_container();
		// destroy_container(id);
		// pid = fork();
		// if(pid==0)
		// 	exec("rm",arg);
		// else
		// 	wait();

		// destroy_container(id2);
		// pid = fork();
		// if(pid==0)
		// 	exec("rm",arg2);
		// else
		// 	wait();

		// destroy_container(id3);
		// pid = fork();
		// if(pid==0)
		// 	exec("rm",arg3);
		// else
		// 	wait();
		
	}

	exit();
}
