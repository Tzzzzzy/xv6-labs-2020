#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char* path)
{
	static char buf[DIRSIZ+1];
	char* p;

	for(p = path+strlen(path); p >= path && *p != '/'; p--)
		;
	p++;
	
	if(strlen(p) > DIRSIZ)
		return p;
	memmove(buf, p, strlen(p));
	//memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
	*(buf+strlen(p)) = '\0';
	return buf;
}

void
find(char* path, char *name)
{
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if((fd = open(path, 0)) < 0){
		fprintf(2, "find: cannot open %s\n", path);
		return;
	}

	if(fstat(fd,&st) < 0){
		fprintf(2, "find: cannnot stat %s\n", path);
		close(fd);
		return;
	}
	
	//int strcmp(const char*, const char*);
	//char* strcpy(char*, const char*);

	switch(st.type){
		case T_FILE:
			if(strcmp(name,fmtname(path)) == 0)
				printf("%s\n", path);
			break;
		case T_DIR:
			strcpy(buf, path);
			p = buf+strlen(buf);
			*p++ = '/';
			while(read(fd, &de, sizeof(de)) == sizeof(de)){
				if(strcmp(de.name, "." ) == 0 ||
				   strcmp(de.name, "..") == 0 ||
				   de.inum == 0)
					continue;
				memmove(p, de.name, DIRSIZ);

				p[DIRSIZ] = 0;
				//recursive
				find(buf, name);
			}
			break;
	}
	close(fd);
}

int
main(int argc, char* argv[])
{
	find(argv[1],argv[2]);
	exit(0);
}
