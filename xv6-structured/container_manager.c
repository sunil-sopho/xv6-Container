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
	// fork test here added to container 1
	int forkVal = fork();
	if(forkVal == 0)
		join_container(id);
	forkVal = fork();
	if(forkVal == 0)
		join_container(id);
	forkVal = fork();
	if(forkVal == 0)
		join_container(id);


	// add to container 2 and 3
	forkVal = fork();
	if(forkVal == 0)
		join_container(id2);
	forkVal = fork();
	if(forkVal == 0)
		join_container(id3);


	// ps olny by one
	// create sys call to get  one pid for given container ID.
	// if (getpid() == pspid(containerId))

	exit();
}
