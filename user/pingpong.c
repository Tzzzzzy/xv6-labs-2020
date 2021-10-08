#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int 
main(int argc, char *argv[])
{
	int p1[2];
	int p2[2];
	pipe(p1);
	pipe(p2);

 	char cbuf[1];
	char pbuf[1];
	if(fork() == 0){
		close(p1[1]);
		read(p1[0],cbuf,1);
		fprintf(1,"%d: received ping\n",getpid());
		//fprintf(1,"child rcv %c\n",cbuf[0]);
		close(p1[0]);
		close(p2[0]);
		write(p2[1],"c",1);
		close(p2[1]);		
	}else{
		close(p1[0]);
		write(p1[1],"p",1);
		close(p1[1]);
		wait(0);
		close(p2[1]);
		read(p2[0],pbuf,1);
		fprintf(1,"%d: received pong\n",getpid());
		//fprintf(1,"parent rcv %c\n",pbuf[0]);
		exit(0);
	}
	exit(0);
}
	
