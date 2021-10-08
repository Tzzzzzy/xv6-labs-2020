#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

char* args[MAXARG];

int
main(int argc, char* argv[])
{
	char* args[MAXARG];
	for(int i = 0; i < MAXARG; i++)
		//warning: don't use this form: args[i] = "",or they all point
		//to the same place!
		args[i] = (char*)(malloc(32));
	int i;
	for(i = 1; i < argc; i++){
		args[i] = argv[i];
	}	
	char buf;
	int idx = 0;
	while(read(0,&buf,1) > 0){
		if(buf == '\n'){
			i++;
			idx = 0;
		}
		else if(buf == ' '){
			i++;
			idx = 0;
		}else{
			args[i][idx++] = buf;
		}
	}
	
	char* p[i-1];
	for(int j = 0; j < i-1; j++){
		p[j] = args[j+1];
	}
	p[i-1] = 0;
	if(fork() == 0){
		exec(args[1],p);
	}
	else
		wait(0);
	exit(0);
}
