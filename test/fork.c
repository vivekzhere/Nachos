#include "syscall.h"
int i;
void p()
{
  while(i++ < 10)
  Print("Hi");
  Exit(0);
}
void func()
{
  Print("****hi****\n");
   //  Fork(p);
 i=Exec("./test/open");
  Print("###############Hey###############\n");
  Exit(0);
}
void main()
{
   Print("Hello\n");
  Fork(func);

 // Fork(func);
//   Join(i);
  Print("hello after fork\n");
  //Join(1);
  //while(i++<10000);
  Halt();
}
