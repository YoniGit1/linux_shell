#include <stdio.h> 			/*Yhonatan Srur*/
#include <string.h>			/*ID : 203868351*/
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <linux/limits.h>
#include <signal.h>
#include<fcntl.h>

#define MAX_CMD 510

/*functions*/
void chandir(char*);
void printPromp();
char** putArgInArry(char *,int);
int numWords(char*);
void freeArr(char**);
void runCmd (char** ,int);
void sig_handler(int);
void doPipe (char **,int,int);
void toFile (char**,int,int);
char** arrToArr(char**,int,int);
int checkDirection(char**,int*,int*);
void cath_sint (int);
/*Global val*/
char cwd[1024];
int cmdCounter=0;
int cmdLength=0;

int main() {
    char x[MAX_CMD];
    char y[MAX_CMD];
    signal(SIGINT,cath_sint);
    while (1){
		  int isPipe=0;
      int isRedirection=0;
  		printPromp();					// print the shell line
  		fgets(x, MAX_CMD, stdin);
  		int j = strlen(x) - 1;
  		if(j==0)						//no input
  			continue;
  		if (x[j] == '\n')			 // remove the '/n'
  			x[j] = '\0';
  		strcpy(y, x);				 // copy the string before it change

  		int counter= numWords(x);	 			// number of word in the 'x' strin
  		char **arg = putArgInArry(y,counter);		 // the arry has now the command(arg[0])and arguments

      int place=checkDirection(arg,&isRedirection,&isPipe); // check if there is '|'(pipe) or Redirection in the command
                                                          // the function return the place in the arry (of the Redirection)

  		if(isPipe==1){
        doPipe(arg,place,counter);  // do pipe command
        freeArr(arg);
        continue;
      }
      if(isRedirection==1){
        toFile(arg,place,counter);     // do Redirection command
        freeArr(arg);
        continue;
      }
      if(strcmp(arg[0],"cd")==0 )			 //the 'cd' command
  		{
  			chandir(arg[1]);   				 // go to change dir function
  			freeArr(arg);					/*free the malloc arg arry*/
  			continue;
  		}
  		else if(strcmp(arg[0],"done")==0) 		//the 'done' command
  		{
  			freeArr(arg);					/*free the malloc arg arry*/
  			break;
  		}
  		else{
  			runCmd(arg,counter);				/*implement the command */
  			freeArr(arg); 				/*free the malloc arg arry*/
  		}

    }

	printf("Num of cmd: %d\nCmd Length: %d\nBye !\n",cmdCounter,cmdLength);
    return 0;

}

void cath_sint (int sig_num){
  signal(SIGINT,cath_sint);
}

/*Print the propt line in shell*/
void printPromp(){
    struct passwd *pwd;
    pwd = getpwuid(geteuid());
	if (getcwd(cwd, sizeof(cwd)) != NULL)
        printf("%s@%s>", pwd->pw_name, cwd);
    else
        perror("getcwd() error");
}

/*chnage dirctory command*/
void chandir(char*arg){
    if(arg== NULL)
		return;
    char* ret;
    char cwd [1024];
    if(strcmp(arg,"..")==0){
        if (getcwd(cwd, sizeof(cwd)) != NULL){
            ret= strrchr(cwd,'/');
            int size=strlen(cwd)-strlen(ret);
            cwd[size]='\0';
            chdir(cwd);
        }
    }
    else
        chdir(arg);
}

/*return the numbers of words in the string*/
int numWords(char* x){
		char *ptr = strtok(x, " ,");
		int counter = 0;
		while (ptr != NULL) {
			counter++;
			ptr = strtok(NULL, " ,");
		}
	return counter;
}

/*implement regular command */
void runCmd (char** arg,int lastWord){
		signal(SIGCHLD,sig_handler);
	    pid_t p = fork();
        if(p<0)
        {
            perror("ERR");
			      exit(1);
        }
        int ex=0;
        if (p == 0)
		        {
            ex =execvp(arg[0], arg);
            if(ex< 0)
            {
                printf("%s: command not found!\n",arg[0]);
				        exit(1);

            }
			      exit(1);
        }
        else /*not the son*/
    		   {
    			if(strcmp(arg[lastWord-1],"&")!=0)
               		pause();
    			cmdLength+=strlen(arg[0]);
    			cmdCounter++;
    		  }
}

void sig_handler(int sig){
  while(waitpid(-1,NULL,WNOHANG) > 0);
}

/*get string and number of words in the string, and put them in arry*/
char** putArgInArry(char * y,int counter){

	char** 	arg = (char **) malloc(sizeof(char *) * (counter + 1));
        if (arg == NULL)
        {
            printf("ERR\n");
            exit(1);
        }
		char *ptr2 = strtok(y, " ,");

		int i = 0;
    while (ptr2 != NULL) {
        arg[i] = (char *) malloc(strlen(ptr2));
		assert(arg[i] != NULL);
        strcpy(arg[i], ptr2);
        ptr2 = strtok(NULL, " ,\"");
        i++;
    }
    arg[i] = NULL;
	return arg;
}

/*this func implement command with '|' (with pipe) */
void doPipe(char ** arg, int place,int size){
	int fds[2];
	pid_t left,right;
	char** leftArg = arrToArr(arg,0,place);
	char** rightArg = arrToArr(arg,place+1,size);
	int i=0;
  if(pipe(fds)<0){
    printf("ERR pipe\n");
    exit(1);
  }
  left=fork();
  if(left<0){
    perror("ERR fork");
    exit(1);
  }
	if( left ==0)
	{
		dup2(fds[1],STDOUT_FILENO);
		close(fds[0]);
		close(fds[1]);
		execvp(leftArg[0],leftArg);

	}
  right=fork();
  if(right<0){
    perror("ERR fork");
    exit(1);
  }
	else if( right ==0)
	{
		dup2(fds[0],STDIN_FILENO);
		close(fds[0]);
		close(fds[1]);
    int isRedirection=0,isPipe=0;
    int r=checkDirection(rightArg,&isRedirection,&isPipe); // 'r' is the place of the redirection in the 'arg'array
    if(isRedirection==1){
      toFile(rightArg,r,((size+1)-place));
      exit(0);
    }
		else
      if(execvp(rightArg[0],rightArg)<0){
        printf("%s: command not found\n",rightArg[0]);
        exit(1);
      }
	}
	else
	{
		close(fds[0]);
		close(fds[1]);
		wait(NULL);
		wait(NULL);
    cmdLength+=strlen(leftArg[0]);
    cmdCounter++;
	}
	freeArr(leftArg);
	freeArr(rightArg);
}

/*this func implement command with Redirection */
void toFile(char** arg,int place , int size ){

  pid_t pid;
  int fd;
	char** leftArg = arrToArr(arg,0,place);
  pid=fork();
  if(pid==0){
    if(strcmp(arg[place],">")==0){
    fd = open(arg[place+1],O_WRONLY|O_CREAT|O_TRUNC,S_IRWXU);
    if(fd==-1){perror("ERR");exit(1);}
    dup2(fd,STDOUT_FILENO);
    }
    else if (strcmp(arg[place],">>")==0) {
      fd = open(arg[place+1], O_WRONLY | O_CREAT | O_APPEND,S_IRWXU);
      if(fd==-1){perror("ERR");exit(1);}
      dup2(fd,STDOUT_FILENO);
    }
    else if(strcmp(arg[place],"<")==0){
      fd = open(arg[place+1], O_RDONLY,S_IRWXU);
      if(fd==-1){perror("ERR");exit(1);}
      dup2(fd,STDIN_FILENO);
    }
    else if(strcmp(arg[place],"2>")==0) {
      fd = open(arg[place+1], O_WRONLY | O_CREAT | O_TRUNC,S_IRWXU);
      if(fd==-1){perror("ERR");exit(1);}
      dup2(fd,2);
    }
    if(execvp(leftArg[0],leftArg)<0){
      printf("%s: command not found\n",leftArg[0]);
      exit(1);
    }
  }
  else{
    wait(NULL);
    cmdLength+=strlen(leftArg[0]);
    cmdCounter++;
  }
  freeArr(leftArg);
}

/*copy the recived array from stat to stop to new arry and return him*/
char** arrToArr(char** arg,int start , int stop){
	char** newArg = (char**) malloc(sizeof(char*)*((stop+1)-start));
	if (newArg == NULL){
            printf("ERR\n");exit(1);
        }
	int i=start;
	int j=0;
	while(start<stop)
	{
		newArg[j]=(char*)malloc(strlen(arg[start]));
    if(newArg[j]==NULL){
      printf("ERR\n");exit(1);
    }
		strcpy(newArg[j], arg[start]);
		start++;
		j++;
	}
	newArg[j]=NULL;
	return newArg;
}

/*check if the command has '|' '>' '>>' '<' '2>' */
int checkDirection(char** arg,int * isRedirection,int* isPipe){
  int e=1;
  while(arg[e] != NULL){
    if (strcmp(arg[e],"|") ==0){
      (*isPipe)=1;
      break;
    }
    if((strcmp(arg[e],">"))==0||(strcmp(arg[e],">>"))==0||(strcmp(arg[e],"<"))==0||(strcmp(arg[e],"2>"))==0){
      (*isRedirection)=1;
      break;
    }
    e++;
  }
  return e;
}

/*free the argumant array*/
void freeArr(char** arg){
	int i=0;
	while(arg[i]!=NULL){
		free (arg[i]);
		i++;
	}
	free (arg[i]);
	free (arg);
}
