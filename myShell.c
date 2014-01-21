//*******************************************
// Andrew Walters
// Matthew Thorp
// CSC 1600: Operating Systems
// Shell Project
// December 2013
//*******************************************

#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/types.h>
#include<errno.h>
#include<syslog.h>
#include<signal.h>
#include<sys/wait.h>

#define STRLENGTH 200
#define TOKLENGTH 50
#define TOKNUM 10

int readstr(char str[STRLENGTH]);
int parse(char str[STRLENGTH], char tok[TOKNUM][TOKLENGTH]);
int redirect(char tok[TOKNUM][TOKLENGTH], int stdinCopy, int stdoutCopy);
int background(char tok[TOKNUM][TOKLENGTH], int num);
void sigtstp_handler();

//PID of suspended and child process
pid_t childpid, child_id; 

int main(int argc, char *argv[])
{
	//variable declarations and initializations
	char str[STRLENGTH];			//holds input string
	char tok[TOKNUM][TOKLENGTH];	//holds parsed tokens
	char *args[TOKNUM];				//holds pointers to strings in tok
	int i;							//loop counter
	int num;						//number of arguments
	int redir = -1;					//tracks redirection
	sigset_t set;					//to hold signal	
	int check;
	
	//make copies of I/O
	int stdinCopy = dup(0);
	int stdoutCopy = dup(1);

	//infinite loop - exits when user types "exit"
	while(1)
	{
		signal(SIGINT,SIG_IGN);	
		signal(SIGTSTP,sigtstp_handler);

		//print prompt
		printf("my-shell$: ");
		
		//read input and store result in str
		readstr(str);
	
		//if the input is "exit" then exit the shell
		if(strcmp(str,"exit")==0)
		{
			return 0;
		}
		//if the input is "resume" resume the suspended process
		else if(strcmp(str,"resume")==0)
		{
			kill(childpid,SIGCONT);
			
		}
		//otherwise parse the input
		else
		{
			//parse input and return the number of tokens
			num = parse(str, tok);
		}
		
		//check for run in background
		if(strcmp(tok[num-1],"&")==0)
		{
			background(tok,num-1);
		}
		//otherwise execute normally
		else
		{
			//check for redirection
			redir = redirect(tok,stdinCopy,stdoutCopy);
		
			// if there was redirection, reduce num by 2
			if(redir != -1)
			{
				num = num - 2;
			}

			//set arguments
			for (i=0;i<TOKNUM;i++)
			{
				if(i<num)
					args[i]=tok[i];
			
				else
					args[i]=NULL;
			}		
	
			child_id = fork();
			int child_status;
			
			//in child process
			if(child_id == 0)
			{
				childpid = getpid(); 
				signal(SIGINT,SIG_DFL);
				signal(SIGTSTP,SIG_DFL);
				execvp(args[0],args);
				exit(0);
			}
			//parent process
			else
			{
				
				waitpid(child_id,&child_status,WUNTRACED);
				
			}

			//restore I/O
			switch(redir)
			{
			case -1: 	break;
			case 0:		close(0); dup2(stdinCopy,0);break;
			case 1:		close(1); dup2(stdoutCopy,1);break;
			default:	break;
			}
		}//end else - not a background process
	}//end while
	return 0;

}//end main


//******************************************************
// read input from stdin into string
//******************************************************
int readstr(char str[STRLENGTH])
{
	int i = 0;
	char temp;
	do
	{	
		str[i] = fgetc(stdin);
		//if redirect command comes in, increment i,get input, and check 
		//next character. If next character is a space
		if (str[i]=='>'||str[i]=='<')
		{
			i++;
			str[i]=fgetc(stdin);
			
			//if the character following redirection is not a space
			if(str[i]!= ' ')
			{
				//insert a space
				temp = str[i];
				str[i]= ' ';
				i++;
				str[i]=temp;
			}
		}
		
		i++;

	}
	while(str[i-1] != '\n' && i < STRLENGTH);
	str[i-1] = '\0';
	return 0;
}//end read


//******************************************************
// store each token from the input
//******************************************************
int parse(char str[STRLENGTH], char tok[TOKNUM][TOKLENGTH])
{
	char *tokptr;
	int i;
	int num = 0;
	char * deliminators = {" "};	

	tokptr = strtok(str,deliminators);
	for(i = 0; i < TOKNUM; i++)
	{
		if(tokptr != NULL)
		{
			//store token
			strcpy(tok[i], tokptr);
			
			//assign new token
			tokptr = strtok(NULL,deliminators);
	
		}//end if
		else
		{
			tok[i][0] = '\0';

			//set num to the index of the first token equal to NULL
			if(num == 0)
				num = i;

		}//end else
	}
	return num;
}//end parse

//************************************************************
// check for redirection and open new I/O
// return -1 for no redirection, 0 for input redirection, 1 for output redirection
//************************************************************
int redirect(char tok[TOKNUM][TOKLENGTH], int stdinCopy, int stdoutCopy)
{
	//variable declarations and initialization
	int i;
	
	for(i = 0; i < TOKNUM; i++)
	{
		//output redirection
		if(strcmp(tok[i],">")==0)
		{
			close(1); //closes output file descriptor
			if(open(tok[++i],O_WRONLY | O_CREAT, 0666)==1)
			{
				//remove redirection token and file name token from tok
				tok[i][0] = '\0';
				tok[--i][0] = '\0';
			}
			//open failed
			else
			{
				//restore stdout
				dup2(stdoutCopy,1);
			}
			return 1;
		}//end output redirection
		
		//input redirection
		else if(strcmp(tok[i],"<")==0)
		{
			close(0); //closes input file descriptor
			if(open(tok[++i],O_RDONLY | O_CREAT, 0666)==0)
			{
				//remove redirection token and file name token from tok
				tok[i][0] = '\0';
				tok[--i][0] = '\0';
			}
			//open failed
			else
			{
				//restore stdout
				dup2(stdinCopy,0);
			}
			return 0;
		}//end input redirection
		
	}//end for
	
	return -1;
}


//****************************************************************************
// Run a process in the background
//****************************************************************************
int background(char tok[TOKNUM][TOKLENGTH],int num)
{
	//declare variables
	int i = 0;
	pid_t pid, sid;
	char *args[TOKNUM];				//holds pointers to strings in tok
        
	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		return 0;
	}
	
        /* If we got a good PID, then
           we can exit the parent process.
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }
		*/
	
	//check if in child process
	//attemptwithoutchildif(pid == 0)
	//attemptwithoutchild{

		/* Change the file mode mask */
		umask(0);
                
		/* Open any logs here */        
                
		/* Create a new SID for the child process */
		sid = setsid();
		if (sid < 0)
		{
			/* Log the failure */
			return -1;
		}
        

        
		/* Change the current working directory
		if ((chdir("/")) < 0)
		{
			// Log the failure
			exit(EXIT_FAILURE);
		}
		*/
        
		/* Close out the standard file descriptors */
		close(0);
		close(1);
		close(2);
        
		/* Daemon-specific initialization goes here */
        
		/* The Big Loop */
		while (1)
		{
			//set arguments
			for (i=0;i<TOKNUM;i++)
			{
				if(i<num)
					args[i]=tok[i];
			
				else
					args[i]=NULL;
			}

			//exec call
			execvp(args[0],args);
			return -1;
		}//end while
	//}attemptwithoutchildend if - in child process
	
	return 0;
}


//**********************************************************
//Signal Handler for Cntl Z
//**********************************************************
void sigtstp_handler()
{
	signal(SIGTSTP,SIG_IGN);
	kill(childpid,SIGTSTP);
}
