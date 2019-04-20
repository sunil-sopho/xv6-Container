// Container  user file

# include "types.h"
# include "user.h"
# include "date.h"
int
main ( int argc , char * argv [])
{

	printf(1,"\n\n Started Container Manager \n");

	int id = create_container();
	int id2 = create_container();
	int id3 = create_container();

	printf(1,"Id : %d\n",id );
	printf(1,"Id : %d\n",id2 );
	printf(1,"Id : %d\n",id3 );


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
	printf(1, "In Process :: %d belong to container :: %d \n",pidCurProc,procContainer);
	// ps olny by one
	// create sys call to get  one pid for given container ID.
	// if (getpid() == pspid(containerId))
	if(parentPid != pidCurProc){

		//------------------ PROCESS ISOLATION ---------------------
		if(proc_container_num(pidCurProc) == 1){
			ps();
		}
	}else{

		/* - - - - - - - - - - - - - - - - SCHEDULER TEST - - - - - - - - - - - - - - - - - - - - - - - - */
		// scheduler_log_on ();
		// scheduler_log_off ();
	}


	exit();
}
