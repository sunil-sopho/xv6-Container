#include "types.h"
#include "stat.h"
#include "user.h"

#define MaxProc 8

int table[3][MaxProc];
// for each column: row 0 is the child's pid, row 1 indicates busy\idle, row 2 

void convert(int partialsum,char* msg){
	int loc =0;
	int val = partialsum%10;
	partialsum /= 10;
	msg[loc] = (char)val;
	loc++;
	while(partialsum > 0){
		val = partialsum%10;
		partialsum /= 10;
		msg[loc] = (char)val;
		loc++;
	}
	msg[loc]= '\0';
}

int deconvert(char* msg){
	int sum =0;
	int loc=0;
	int siz=1;
	while(msg[loc]!='\0'){
		sum += (int)msg[loc]*(siz);
		siz*=10;
		loc++;
	}
	return sum;
}

int
main(int argc, char *argv[])
{
	if(argc< 3){
		printf(1,"Need type and input filename\n");
		exit();
	}
	char *filename;
	filename=argv[2];
	int type = atoi(argv[1]);
	printf(1,"Type is %d and filename is %s\n",type, filename);

	int tot_sum = 0;	
	float variance = 0.0;

	int size=1000;
	short arr[size];
	char c;
	int fd = open(filename, 0);
	for(int i=0; i<size; i++){
		read(fd, &c, 1);
		arr[i]=c-'0';
		read(fd, &c, 1);
	}	
  	close(fd);
  	// this is to supress warning
  	printf(1,"first elem %d\n", arr[0]);
  
  	//----FILL THE CODE HERE for unicast sum and multicast variance
  	// int par =0;
  	// for(int i=0;i<333;i++)
  	// 	par += arr[i];
  	// printf(1,"sum of par:%d\n",par );
  	int i,numChild=7;
  	int parent = getpid();
	for (i = 0; i < numChild; i++){
			table[0][i] = fork();
			if (table[0][i] == 0) { // child code
				// while(1)
				printf(1,"%d %d\n",getpid(),i);;
				// 	sigpause();
				int partialsum=0;
				int start = i*(size/numChild);
				int end;
				if(i==numChild-1)
					end =size;
				else
					end = (i+1)*(size/numChild);
				// int startold= start;
				for(;start<end;start++)
					partialsum += arr[start];
				char msg[8];
				convert(partialsum,msg);
				// sprintf(msg, "%d", partialsum);

				// printf(1,"partial sum sending:%d  id:%d from :%d to:%d\n",partialsum,i+1 ,startold,end);
				convert(partialsum,msg);
				send(getpid(),parent,msg);
				// printf(1, "ERROR\n");
				if(type==0)
					exit();
				else{
					// recv(msg);
					// int value = deconvert(msg);
					// start = i*(size/numChild);
					// int parti = 0;
					// for(;start<end;start++)
					// 	parti += (arr[start]-value)*(arr[start]-value);
					// convert(parti,msg);
					// send(getpid(),parent,msg);
					// exit();
				}
			
			}
			else { // father code
				// printf(1,"%d  : : %d : : %d\n",parent,table[0][i],getpid());
				table[1][i] = 0; // mark child as idle

			}
	}
	if(getpid()==parent){
		int count=0;
		char msg[8];
		while(count<numChild){
			// printf(1," trying to get msg %d\n",count+1);
			recv(msg);
			// printf(1," got msg %d\n",count+1);
			int value = deconvert(msg);
			tot_sum += value;
			// printf(1,"partial sum recieving:%d  id:%d\n",value,count+1 );
			count++;
			// printf(1,"count increased next iter\n");
		}
		if(type==1){
			// int avg = tot_sum/1000;
		}

	}

  	//------------------

  	// if(type==0 && getpid() == parent){ //unicast sum
  	if(type==0 ){ //unicast sum
		printf(1,"Sum of array for file %s is %d\n", filename,tot_sum);
	}
	else if(type==1){ //mulicast variance
		printf(1,"Variance of array for file %s is %d\n", filename,(int)variance);
	}
	exit();
}
