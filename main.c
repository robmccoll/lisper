#include "lisper.h"
#include <string.h>
#include <stdio.h>
  
int
main(int argc, char *argv[])
{
  char * list = "(\"this is a list :-)\"    \"second item\" (x\"414243\"))";
  char * cur = list;
  int len = strlen(list);
  exp_t parsed;
  const char * err = parse_list(&parsed, &cur, &len);
  printf("err: %s\n", err);
  print_exp(&parsed, 0);
  return 0;
}
