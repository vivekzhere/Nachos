#include "syscall.h"
void main()
{
	int fd;
	Print("Hello World\n");
	fd=Exec("./test/open");
	Print("Hello World after\n");
	Halt();	// Optional. Just print stats
}
