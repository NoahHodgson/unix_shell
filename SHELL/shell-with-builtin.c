//NOAH HODGSON MARCH 14TH 2021
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <glob.h>
#include <sys/wait.h>
#include "sh.h"

void sig_handler(int sig)
{
  fprintf(stdout, "\n>> ");
  fflush(stdout);
}
  
int
main(int argc, char **argv, char **envp)
{
	char	buf[MAXLINE];
	char    *arg[MAXARGS];  // an array of tokens
	char    *ptr;
	char    *pref;
	char    *pdir;
        char    *pch;
	int flag = 0; //flag for checking if pref has been inputed
	pid_t	pid;
	int	status, i, arg_no;
        signal(SIGINT, sig_handler);
	signal(SIGTSTP, sig_handler);

	ptr=getcwd(NULL,0);
	fprintf(stdout, "[%s]>> ",ptr);	/* print prompt (printf requires %% to print %) */
	free(ptr);
	fflush(stdout);
	while (fgets(buf, MAXLINE, stdin) != NULL) {
		if (strlen(buf) == 1 && buf[strlen(buf) - 1] == '\n')
		  goto nextprompt;  // "empty" command line

		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = 0; /* replace newline with null */
		// parse command line into tokens (stored in buf)
		arg_no = 0;
                pch = strtok(buf, " ");
                while (pch != NULL && arg_no < MAXARGS)
                {
		  arg[arg_no] = pch;
		  arg_no++;
                  pch = strtok (NULL, " ");
                }
		arg[arg_no] = (char *) NULL;

		if (arg[0] == NULL)  // "blank" command line
		  goto nextprompt;

		/* print tokens
		for (i = 0; i < arg_no; i++)
		  printf("arg[%d] = %s\n", i, arg[i]);
                */

                if (strcmp(arg[0], "pwd") == 0) { // built-in command pwd 
		  printf("Executing built-in [pwd]\n");
	          ptr = getcwd(NULL, 0);
                  printf("%s\n", ptr);
                  free(ptr);
	        }
		else
		if(strcmp(arg[0],"pid")==0){
		  printf("Executing built-in [pid]\n");//built-in command pid
		  pid=getpid();
		  printf("Process ID is %d\n",pid);
		}
		else
	 	if(strcmp(arg[0], "kill")==0){ //built-in command kill
	 	  printf("Executing built-in [kill]\n");
		  if(arg[1]==NULL){
		    printf("ERROR: Not enough args.\n"); // you need to identify something to kill
		  }
		  else if(arg[2]==NULL){
		    int pid = atoi(arg[1]); // atoi will turn our string to an int
		    if(pid == 0){
		      printf("ERROR: Invalid process id\n");
		    }
		    else{
		      if(kill(pid, SIGTERM)==-1){ //-1 is the error code for c's kill function
		        printf("ERROR: Could not kill\n");
		      }
		    }  
		  }
		  else if(arg[3]==NULL){
		    int pid = atoi(arg[2]);
		    arg[1][0] = ' '; //getting rid of the dash
		    int sig = atoi(arg[1]);
		    if(pid == 0){
		      printf("ERROR: Invalid process id\n");
		    }
		    else if(sig == 0){
		      printf("ERROR: Invalid signal\n");
		    }
		    else{
		      if(kill(pid, sig)==-1){
		        printf("ERROR: Could not kill\n");
		      }
		    }
		  }
		  else{
		    printf("ERROR: Too many arguments");
		  }	  
		}
		else
                if (strcmp(arg[0], "which") == 0) { // built-in command which
		  struct pathelement *p, *tmp;
                  char *cmd;
                    
		  printf("Executing built-in [which]\n");

		  if (arg[1] == NULL) {  // "empty" which
		    printf("which: Too few arguments.\n");
		    goto nextprompt;
                  }

		  p = get_path();
           /***/
		  tmp = p;
		  while (tmp) {      // print list of paths
		    printf("path [%s]\n", tmp->element);
		    tmp = tmp->next;
                  }
           /***/

                  cmd = which(arg[1], p);
                  if (cmd) {
		    printf("%s\n", cmd);
                    free(cmd);
                  }
		  else               // argument not found
		    printf("%s: Command not found\n", arg[1]);

		  while (p) {   // free list of path values
		     tmp = p;
		     p = p->next;
		     free(tmp->element);
		     free(tmp);
            	  }
	        }
		else
		if(strcmp(arg[0],"printenv")==0){ //built-in printenv
		  printf("Executing built-in [printenv]");
		  if(arg[1] == NULL){ //prints the whole enviroment
		    for (char **env = envp; *env != 0; env++){
    		      char *thisEnv = *env;
                      printf("%s\n", thisEnv);    
  		    }
		  }  
		  else if(arg[1]!=NULL && arg[2] == NULL){
		    char* thisEnv = getenv(arg[1]); //prints based on argument passed
		    if(thisEnv)
			    printf("%s\n", thisEnv);
		    free(thisEnv);
		  }
		  else{
		    printf("ERROR: Too Many Arguments.\n");
		  }  
		}
		else
		if(strcmp(arg[0],"setenv")==0){ //built-in set-env
		  printf("Executing built in [setenv].\n");
		  if(arg[1]==NULL){
		    for (char **env = envp; *env != 0; env++){
                      char *thisEnv = *env;
                      printf("%s\n", thisEnv);
                    }
		  }
		  else if(arg[1]!=NULL && arg[2]==NULL){
		  	//may have to change something here for HOME to work
		    setenv(arg[1],"",1); //path to empty string
		  }
		  else if(arg[2]!=NULL && arg[3]==NULL){
		    setenv(arg[1], arg[2],1); //setting enviroment of arg1 to arg2
		  }
		  else{
		    printf("ERROR: Too Many Arguments.\n");
		  }
		}
		else
		if(strcmp(arg[0],"list")==0){
                  ptr=getcwd(NULL,0);
		  DIR *dir;
                  struct dirent *cdir;
		  if(arg[1]==NULL){
		    dir = opendir(ptr);
		    while((cdir = readdir(dir)) != NULL){
		      printf("  %s\n", cdir->d_name);//printing everything in the current directory
		    }//make sure we free everything out
		    free(ptr);
		    free(dir);
		    free(cdir);
		  }
		  else{
		    int i = 1;
		    while(arg[i] != NULL){ //so we can do this for multiple arguments
		      if((dir=opendir(arg[i]))==0){ //0 = directory failed to open
		        perror("ERROR: ");
			break;
		      }
		      printf("%s:\n", arg[i]);
		      while(cdir=readdir(dir)){
		        printf("   %s\n", cdir->d_name);
		      } 
		      printf("\n");
		      free(dir);
		      free(cdir);
		      i++;
		    }
		  }
		}
	        else
		if(strcmp(arg[0], "where")==0){ //similar to which but more
		    struct pathelement *p, *tmp; //might need more
		    char *cmd;

		    printf("Executing built-in [where]\n");
		    if(arg[1] == NULL){ //empty where
		  	printf("where: Too few arguments.\n");
			goto nextprompt;
		    }
 		    p = get_path();
           /***/
                    tmp = p;
                    while (tmp) {      // print list of paths
                      printf("path [%s]\n", tmp->element);
                      tmp = tmp->next;
                    }
           /***/

                    cmd = where(arg[1], p);
                    if (cmd) {
                      printf("%s\n", cmd);
                      free(cmd);
                    } 
                  else               // argument not found
                    printf("%s: Command not found\n", arg[1]);
		}
		else
		if(strcmp(arg[0],"prompt")==0){//built-in promt
		  printf("Executing built-in [prompt]\n");
		  if(arg[1]==NULL){
		    printf("   input a prompt:\n");
		    scanf("%s",ptr);
		    pref = malloc(sizeof(char)*strlen(ptr));
		    strcpy(pref, ptr);
		    free(ptr);
		    flag = 1;
		  }
		  else{
		    pref = malloc(sizeof(char)*strlen(arg[1]));
		    strcpy(pref, arg[1]); //places pref in the front of the console
		    flag = 1;
		  }
		}
		else
		if(strcmp(arg[0], "cd")==0){ //built-in cd
		  printf("Executing built-in [cd]\n");
		  if(arg[1] == NULL){ //meaning we go home
		    ptr = getcwd(NULL, 0);
                    pdir=malloc(strlen(ptr)*sizeof(char));
                    strcpy(pdir, ptr);
		    printf("CD'ing into HOME\n");
		    chdir(getenv("HOME"));
		    free(ptr);
		  }
		  else if(strcmp(arg[1],"-")==0){ //meaning we go back. Doesn't work if called twice in a row
		    ptr=getcwd(NULL,0);
                    char*prev = malloc(strlen(ptr)*sizeof(char));
		    strcpy(prev,ptr);
		    chdir(pdir);
		    pdir=prev;
		    free(prev);
		    free(ptr);
		  }
		  else{	            
		    ptr = getcwd(NULL, 0); //cd to this directory
                    pdir=malloc(strlen(ptr)*sizeof(char));
		    strcpy(pdir, ptr);
		    chdir(arg[1]);
		    free(ptr);
		  }
		}	
		else
		if(strcmp(arg[0], "exit") == 0){ //exit and free what is leftover
		  printf("Executing built-in [exit].\n");
		  free(pdir);
		  if(flag)
		  	free(pref);
		  exit(0);
		}
		else {  // external command
		  if ((pid = fork()) < 0) {
			printf("fork error");
		  } else if (pid == 0) {		/* child */
			                // an array of aguments for execve()
			char    *execargs[MAXARGS]; 
		        glob_t  paths;
                        int     csource; 
			int j;
			char    **p;

			execargs[0] = malloc(sizeof(char) *strlen(arg[0])+1);
			strcpy(execargs[0], arg[0]);
			if(strstr(execargs[0], "./") || strstr(execargs[0], "/") || strstr(execargs[0], "../")){
				j=1;
		        	for (i = 1; i < arg_no; i++){ // check arguments
			  	  if (strchr(arg[i], '*') != NULL) { // wildcard!
			    	    csource = glob(arg[i], 0, NULL, &paths);
                            		if (csource == 0) {
                              			for (p = paths.gl_pathv; *p != NULL; ++p) {
                                			execargs[j] = malloc(strlen(*p)+1);
							strcpy(execargs[j], *p);
							j++;
                              			}	
                           
                              			globfree(&paths);
                            		}
				  }
				  else{
				    execargs[j] = malloc(strlen(*p)+1);
                                    strcpy(execargs[j], arg[i]);
				    j++; 
				  }
				}	  
                        	execargs[j] = NULL;

				i = 0;
                        	for (i = 0; i < j; i++){
			  	   printf("exec arg [%s]\n", execargs[i]);	
				   execve(execargs[0], execargs, NULL);
				   printf("couldn't execute: %s", buf);
				   exit(127);
				}	
			   }
			else{
			  struct pathelement *path = get_path();
			  int k = 1;
			  for(i=1; i<arg_no; i++){
				if (strchr(arg[i], '*') != NULL) { // wildcard!
				    printf("\n WILDCARD DETECTED \n");
                                    csource = glob(arg[i], 0, NULL, &paths);
                                        if (csource == 0) {
                                                for (p = paths.gl_pathv; *p != NULL; ++p) {
                                                        execargs[k] = malloc(strlen(*p)+1);
                                                        strcpy(execargs[k], *p);
                                                        k++;
                                                }

                                                globfree(&paths);
                                        }
                                }

				else{  
			      	   execargs[k] = malloc(sizeof(char)*(strlen(arg[i])+1));
				   strcpy(execargs[k],arg[i]);
				   k++;
				}
			  }
			  execargs[k] = NULL;
			  execve(which(execargs[0], path), execargs, NULL);
			}
		  }	
		  /* parent */
		  if ((pid = waitpid(pid, &status, 0)) < 0)
			printf("waitpid error");
/**
                  if (WIFEXITED(status)) S&R p. 239 
                    printf("child terminates with (%d)\n", WEXITSTATUS(status));
**/
		  }      
           nextprompt:
		ptr=getcwd(NULL, 0);
		if(flag)
			fprintf(stdout, "%s[%s]>> ",pref, ptr);
		else
			 fprintf(stdout, "[%s]>> ",ptr);
		free(ptr);
		fflush(stdout);
	}
	printf("\nPlease use the built-in [exit] to exit this program.\n");
	goto nextprompt;
}
