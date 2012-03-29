#include "syscall.h"
void main()
{
	int fd;
	char *p="Hello World Open....",buf[25];
	fd=Open("file.txt");
	Write(p,20,fd);
	Close(fd);
	fd=Open("file.txt");
	Read(buf,20,fd);
	Print(buf);
	Close(fd);
	//Halt();	// Optional. Just print stats
}
