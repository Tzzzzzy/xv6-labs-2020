#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
primes(int* buf, int n)
{	
	int p[2];
	pipe(p);
	int pid = fork();
	if(pid == 0){
		memset(buf,0,100);
		close(p[1]);
		int i = 0;
		while(read(p[0],buf+(i++),1)){
		}
		close(p[0]);
		primes(buf,buf[0]);
		exit(0);
	}else{
		close(p[0]);
		int i = 0;
		if(buf[0] != 0)
			fprintf(1,"prime %d\n",buf[0]);
		else
			exit(0);
		while(buf[i] != 0){
			if(buf[i]%n != 0)
				write(p[1],buf+i,1);
			i++;
		}
		close(p[1]);
		wait(0);
		exit(0);
	}
}

int
main()
{
        int buf[100];
        for(int i = 0; i <= 33; i++)
                buf[i] = i+2;
        int n = 2;
	primes(buf,n);
        exit(0);
}

