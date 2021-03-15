#include "get_path.h"
char *where(char *command, struct pathelement *p)
{
  char cmd[64], *ch;
  int  found;
  char* nl = "\n";

  found = 0;
  while (p) {
    sprintf(cmd, "%s/%s", p->element, command);
    if (access(cmd, X_OK) == 0) {
      found = 1;
    }	
    if (found) {
      ch = malloc(strlen(cmd)+1);
      strcat(ch, cmd);   
      found=0;
    }
    p = p->next;
  }
 //not part of our code
  if (ch==0){
    return (char *) NULL;
  }
  else{
    return ch;	  
  }
}
