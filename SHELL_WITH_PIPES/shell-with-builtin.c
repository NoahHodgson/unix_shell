//NOAH HODGSON April 11TH 2021
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <glob.h>
#include <sys/wait.h>
#include "sh.h"
#include <utmpx.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int pipeFunction(int pipe_val, char** argus, int arg_s);
void executeMe(char** arg, int arg_no, int bg, int already_piped, int no_clobber);
int redirectFunction(char** args, int nahclob);

void sig_handler(int sig)
{
  fprintf(stdout, "\n>> ");
  fflush(stdout);
}


typedef struct User{
   char* name;
   int caught;
   struct User* next;
};


//globals
struct User *head;
int userThread = 0;
pthread_t user;
pthread_mutex_t lock;


// function for the thread user that runs until full program is exited. 
// Watches to see if a user logs on and reports it
void *watchUsers(void* shouldBeNull){
   struct utmpx *up;
   while(1){
     setutxent();
     while((up = getutxent())){
        if(up->ut_type == USER_PROCESS){
 	  pthread_mutex_lock(&lock);
	  struct User *iter_user = head;
	  while(iter_user != NULL){
            if(strcmp(iter_user->name, up->ut_user)==0 && iter_user->caught != 1){
              printf("\n%s has logged on %s from %s\n", up->ut_user, up->ut_line, up ->ut_host);
              iter_user->caught = 1;
	    }
            iter_user = iter_user->next;
          }
	  pthread_mutex_unlock(&lock);
	}
     }
     sleep(20);
   }
   return NULL;
}


//main
int
main(int argc, char **argv, char **envp)
{
	char	buf[MAXLINE];
	char    *arg[MAXARGS];  // an array of tokens
	char    *ptr;
	char    *pref;
	char    *pdir = NULL;
        char    *pch;
	int flag = 0; //flag for checking if pref has been inputed
	pid_t	pid;
	int	status, i, arg_no;
        signal(SIGINT, sig_handler);
	signal(SIGTSTP, sig_handler);
	int noclobber = 0;
	
	head =  NULL;
	
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
			
		//background variable :)
		int bg = 0;
		int what_to_do = redirectFunction(arg, noclobber);
                     if(what_to_do == -1){ //no clobbing here
                       goto nextprompt;
                     }
                     else if(what_to_do == 1){//get rid of args after redirection symbols
                       int flag = 0;
                       int r = 0;
                       while(arg[r] != NULL){
                         if(strstr(arg[r], ">") || strstr(arg[r], "<")){
                           flag = 1;
                         }
                         if(flag){
                           arg[r] = NULL;
                           arg_no--;
                         }
                         r++;
                       }
                     } // end of separating between general running

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
		}else
		if(strcmp(arg[0], "noclobber")==0){
		  printf("Executing built-in [noclobber]\n]");
		  if(noclobber){
		    noclobber=1;
		    printf("%d\n", noclobber);
		  }
		  else{
		    noclobber=0;
		    printf("%d\n", noclobber);
		  }
		}
		else
		if(strcmp(arg[0], "watchuser")==0){
		  printf("Executing built-in [watchuser]\n");
		  if(arg[1] == NULL){
		    printf("ERROR: Not enough args\n");
		  }
		  else if(arg[2]==NULL){
		    pthread_mutex_lock(&lock);
	 	    int dupuser = 0;
		    struct User *new_user = (struct User*) malloc(sizeof(struct User));
		    new_user->name = (char*) malloc(sizeof(arg[1])+1);
		    strcpy(new_user->name, arg[1]);
		    new_user->next = NULL; 
		    if(head == NULL){
		      head = new_user;
		      printf("%s user added to watch list. \n", new_user->name);
		    }
		    else{
		       struct User* last;
                       last = head;
		       while(last->next != NULL && dupuser != 1){
			  if(strcmp(new_user->name, last->name)==0){
			    dupuser = 1;
			  }
		          last = last-> next;
		       }
		       //repeating code :{
		       if(strcmp(arg[1], last->name)==0){
                            dupuser = 1;
                       }

		       if(dupuser){
		          printf("ERROR: User already in list.\n");
		       }
		       else{
		          last->next = new_user;
			  printf("%s user added to watch list. \n", new_user->name);
		        } 
		      if(last != NULL){
		        free(last->name);
		        free(last);
		      }
		    }
		    //unlock me	
		    pthread_mutex_unlock(&lock);		    
		    if(userThread==0){
		      printf("Thread Start\n");
                      userThread=1;
                      if(pthread_create(&user, NULL, watchUsers, NULL) != 0){
                        printf("ERROR WITH PTHREAD: CRITICAL!\n");
                      }
		    }
		  }
		  else if (strcmp(arg[2], "off")==0){
		     pthread_mutex_lock(&lock);
		     struct User* looking;
                     struct User* prev;
		     looking = head;	
		     if(looking != NULL && strcmp(looking->name, arg[1])==0){
		       head = looking->next;
		       free(looking);
		       printf("USER DELETED!\n");
		     }
		     else{  
		       while(looking != NULL && strcmp(looking->name, arg[1])!=0){
		         prev = looking;
		         looking = looking->next;
		       }
		       if(looking == NULL){
		         printf("ERROR: USER NOT FOUND\n");
		       }
		       else{
		         prev->next = looking->next;
		         free(looking->name);
		         free(looking);
			 free(prev->name);
			 free(prev);
		         printf("USER DELETED!\n");
		       }
		     }
		     pthread_mutex_unlock(&lock);		     
		  }
		  else{printf("ERROR: Invalid Args\n");}
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
		  if(pdir)
		  	free(pdir);
		  if(flag)
		  	free(pref);
		  pthread_mutex_lock(&lock);
		  struct User* current = head;
		  struct User* next;
		  while(current != NULL){
		    next=current->next;
		    free(current->name);
		    free(current);
		    current = next;
		  }
		  if(head != NULL)
		  	free(head);
		  pthread_mutex_unlock(&lock);
		  pthread_cancel(user);
		  pthread_join(user, NULL);
		  exit(0);
		}
		//handling nonbuilt-in commands
		else { 
           	   executeMe(arg, arg_no, 0, 0, noclobber);  
		}
	        if(what_to_do){
		  int fid = open("/dev/tty", O_WRONLY);
		  close(1);
		  dup(fid);
		  close(fid);

		  fid = open("/dev/tty", O_RDONLY);
                  close(0);
                  dup(fid);
                  close(fid);
		}	
           nextprompt:
		//kill zombies
		while(waitpid((pid_t)(-1), 0, WNOHANG) > 0){}
		ptr=getcwd(NULL, 0);
		//how
		if(flag)
			fprintf(stdout, "%s[%s]>> ",pref, ptr);
		else
			 fprintf(stdout, "[%s]>> ",ptr);
		free(ptr);
		fflush(stdout);
	}
	printf("\nPlease use the built-in [exit] to exit this program.\n");
	sleep(1);
	goto nextprompt;
}

void executeMe(char** arg, int arg_no, int bg, int already_piped, int no_clob){
         int i;
         int status;
              if(already_piped != 1){
		//handling background issues
	 	if(strcmp(arg[arg_no-1], "&")==0){
                          bg=1;
                          arg[arg_no-1] = NULL;
                          arg_no--;
                        }
                   //checking to see if I should use IPC, then calling that function if that's
                   //the case. Function is near the top of the file
		     for(int l = 0; arg[l] != NULL; l++){
                      if(strcmp(arg[l],"|")==0){
                         pipeFunction(0, arg, arg_no);
			 return;
                      }
                      if(strcmp(arg[l],"|&")==0){
                        pipeFunction(1, arg, arg_no);
			return;
                      }
                     }
		   }
                  if ((pid = fork()) < 0) {
                        printf("fork error");
                  } else if (pid == 0) {                /* child */
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
                                   execve(execargs[0], execargs, NULL);
                                   printf("couldn't execute: %s", execargs[0]);
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
                          printf("Couldn't Execute [%s]\n", execargs[0]);
                        }
                  }
                  //parent
                  if(bg==1){
                     if((pid = waitpid(pid, &status, WNOHANG)) < 0)
                         printf("waitpid error");
                  }
                  else{
                      if((pid = waitpid(pid, &status, 0)) < 0)
                         printf("waitpid error");

                  }
/**
                  if (WIFEXITED(status)) S&R p. 239
                    printf("child terminates with (%d)\n", WEXITSTATUS(status));
**/
}

int redirectFunction(char** args, int nahclob){
  int flag = 0;
  char* destination = (char*) malloc(sizeof(char)*16);
  int i;
  int file_exists;
  for(i = 0; args[i] != NULL; i++){
    if(strcmp(args[i], ">")==0){
      strcpy(destination, args[i+1]);
      flag = 1;
      break;
    }    
    if(strcmp(args[i], ">&")==0){
      strcpy(destination, args[i+1]);
      flag = 2;
      break;
    }
    if(strcmp(args[i], ">>")==0){
      strcpy(destination, args[i+1]);
      flag = 3;
      break;
    }
    if(strcmp(args[i], ">>&")==0){
      strcpy(destination, args[i+1]);
      flag = 4;
      break;
    }
    if(strcmp(args[i], "<")==0){
      strcpy(destination, args[i+1]);
      flag = 5;
      break;
    }
  }
  //if there's no redirection
  if(flag == 0){
    free(destination);
    return 0;
  }
  //check if destination exist
  struct stat buffy;
  if(!stat(destination, &buffy)){
    file_exists = 1;
  }
  else{
    file_exists = 0;
  }
  //meat and potatos 
  if(flag == 1){
    if(nahclob && file_exists){
      printf("DON'T CLOB ME. CLOB IS OFF.\n");
      return -1;
    }
    else{//>
      int get_out = open(destination, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP); //stack overflow code here. if code is broken it's this line
      close(1);
      dup(get_out);
      close(get_out);
    }
  }
  else if(flag == 2){//>&
    if(nahclob && file_exists){
      printf("DON'T CLOB ME. CLOB IS OFF.\n");
      return -1;
    }
    else{
      int get_out_err = open(destination, O_WRONLY|O_CREAT|O_TRUNC, 1); //stack overflow code here. if code is broken it's this line
      close(1);
      dup(get_out_err);
      close(2);
      dup(get_out_err);
      close(get_out_err);
    }
  }
  else if(flag == 3){//>>
    if(nahclob && file_exists){
      printf("DON'T CLOB ME. CLOB IS OFF.\n");
      return -1;
    }
    else{
      int get_out_app = open(destination, O_WRONLY|O_CREAT|O_APPEND, 1); //stack overflow code here. if code is broken it's this line
      close(1);
      dup(get_out_app);
      close(get_out_app);
    }
  }
  else if(flag == 4){//>>&
    if(nahclob && file_exists){
      printf("DON'T CLOB ME. CLOB IS OFF.\n");
      return -1;
    }
    else{
      int get_out_err_app = open(destination, O_WRONLY|O_CREAT|O_APPEND, 1); //stack overflow code here. if code is broken it's this line
      close(1);
      dup(get_out_err_app);
      close(2);
      dup(get_out_err_app);
      close(get_out_err_app);     
    }

  }
  else if(flag == 5){//<
    if(file_exists == 0){
      printf("ERROR: File doesn't exist.\n");
      return -1;
    }
    else{
      int get_in = open(destination, O_RDONLY);
      close(0);
      dup(get_in);
      close(get_in);
    }
  }
  free(destination);
  return 1;
}

int pipeFunction(int pipe_val, char** argus, int arg_s){
  int left_args = 0; 
  int l_size = 0;
  int right_args = 0; 
  int pipe_flag = 0;
  int r_size = 0;
  char** left_of = (char**)malloc(sizeof(char*)*8);
  char** right_of = (char**)malloc(sizeof(char*)*8);;
  int my_pipe[2];
  //important variables above iteration variables below
  int i = 0;
  int j = 0;
  while(i < arg_s){
        if(!pipe_flag){
          if(strcmp(argus[i], "|")==0 || strcmp(argus[i], "|&")==0){
             pipe_flag=1;
	     left_of[i] = NULL;
          }
          else{
             left_of[i] = (char*)malloc(64); //change 64 later you dunce
             strcpy(left_of[i], argus[i]);
	     l_size++;
          }
        }
        else{
          right_of[j] = (char*)malloc(64); //change 64 later you dunce
          strcpy(right_of[j], argus[i]);
          j++;
	  r_size++;
        }
	i++;
  }
  right_of[j] = NULL;
  //redirection time
  pipe(my_pipe);
  close(0);
  dup(my_pipe[0]);
  close(my_pipe[0]);
  //standard error piping for |&
  if(pipe_val){
    close(2);
    dup(my_pipe[1]);
  }
  //back to regular scheduled piping
  close(1);
  dup(my_pipe[1]);
  close(my_pipe[1]);
  // HERE WILL GO A WAY TO RUN THE BEFORE PIPE ARGS
  executeMe(left_of, l_size, 0, 1, 0); 
  int get_out_back = open("/dev/tty",O_WRONLY); //this is from stack overflow. If pipes don't work check back here.
  close(1);
  dup(get_out_back);
  close(get_out_back);
  if(pipe_val){
  int get_err_back = open("/dev/tty",O_WRONLY);
  	close(2);
  	dup(get_err_back);
  	close(get_err_back);
  }
  //  HERE WILL GO A WAY TO RUN THE BEFORE PIPE ARGS
  executeMe(right_of, r_size, 0, 1, 0);
  int get_in_back = open("/dev/tty", O_RDONLY);
  close(0);
  dup(get_in_back);	
  close(get_in_back);
  //freeing for loop	
  for(j=0; j<r_size; j++){
    free(right_of[j]);
  }
  for(i=0; i<l_size; i++){
    free(left_of[i]);
  }
  free(left_of);
  free(right_of);
}
