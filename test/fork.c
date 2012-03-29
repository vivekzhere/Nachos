#include "syscall.h"
void p()
{
  int i=0;
  while(i++ < 10)
  Print("Hi");
  Exit(0);
}
void func()
{
Print("****hi****\n");
  Fork(p);
  Print("###############Hey###############\n");
  Exit(0);
}
void main()
{
  int i=0;
  Print("Hello\n");
  Fork(func);
//  Fork(p);
 // Fork(func);
//   Join(i);
  Print("hello after fork\n");
  //Join(1);
  //while(i++<10000);
  Halt();
}
