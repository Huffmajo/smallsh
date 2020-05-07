/***********************************************************
 * Program: smallsh.c
 * Author: Joel Huffman
 * Last updated: 3/3/2019
 * Sources: https://oregonstate.instructure.com/courses/1706555/pages/block-3
 * https://oregonstate.instructure.com/courses/1706555/pages/3-dot-3-advanced-user-input-with-getline
 * https://www.geeksforgeeks.org/dup-dup2-linux-system-call/
 * https://www.geeksforgeeks.org/signals-c-language/
 ***********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_CHARS 2048
#define MAX_ARGS 512
#define _GNU_SOURCE

struct flag
{
	int background;
	int redirectIn;
	int reInFile;
	int redirectOut;
	int reOutFile;
	int emptyOrComment;
	int numArgs;
};

// used to track flags used in current operation
struct flag flag;

// used to track last processed exit status and terminating signal
int lastStatus = -1;
int isStatus;

// limits if processes can be run in background
int backgroundDisable = 0;

// used to queue up background processes and run in order
int bgProcesses[256];
int currBgProcess = 0;

/***********************************************************
 * Function: ExpandPid(string)
 * Expands any $$ substring in string to the process id. 
 * Returns that char*.
 ***********************************************************/
char* ExpandPid(char* string)
{
	int pid = getpid();

	// search for each instance of $$	
	while (strstr(string, "$$"))
	{
		// replace $$ with pid
		sprintf(strstr(string, "$$"), "%d", pid);
	}
	
	return string;
}

/***********************************************************
 * Function: GetUserInput()
 * Prompts user for command line arguements and gets entire 
 * string submitted by user. Returns pointer to said string.
 ***********************************************************/
char* GetUserInput()
{
	char* userInput = NULL;
	size_t bufferSize = 0;

	// prompt user with :
	printf(": ");
	fflush(stdout);

	// clear error if interrupted mid-read
	if (getline(&userInput, &bufferSize, stdin) == -1)
	{
		clearerr(stdin);
	}
	
	//expand $$ to pid
	userInput = ExpandPid(userInput);

	return userInput;
}

/***********************************************************
 * Function: ReadInput(userInput)
 * Takes string input userInput and separates and stores
 * arguements into string array. Checks for redirect in and 
 * out chars while separating and marks flags appropriately.
 * returns a pointer to string array of arguments.
 ***********************************************************/
char** ReadInput(char* userInput)
{
	// maximum amount of arguments is 512
	char** args = malloc(sizeof(char*) * MAX_ARGS);

	// split string by any spacing characters
	char* token = strtok(userInput, " \t\r\n");

	// read and separate all arguments in string
	while (token != NULL)
	{
		// if >, set redirectOut flag
		if (strcmp(token, ">") == 0)
		{
			flag.redirectOut = 1;
			// next arg is file to write to
			flag.reOutFile = flag.numArgs;
		}

		// if <, set redirectIn flag
		else if (strcmp(token, "<") == 0)
		{
			flag.redirectIn = 1;
			// next arg is file to read from
			flag.reInFile = flag.numArgs;
		}

		// otherwise it's a command, argument or '&'
		else
		{			
			args[flag.numArgs] = token;
			flag.numArgs++;
		}
		token = strtok(NULL, " \t\r\n");
	}
	// mark end of args array
	args[flag.numArgs] = NULL;
	return args;
}

/***********************************************************
 * Function: ClearFlags()
 * Reset all flags to false (0). No parameters or return 
 * value. 
 ***********************************************************/
void ClearFlags ()
{
	flag.background = 0;
	flag.redirectIn = 0;
	flag.reInFile = 0;
	flag.redirectOut = 0;
	flag.reOutFile = 0;
	flag.emptyOrComment = 0;
	flag.numArgs = 0;
}

/***********************************************************
 * Function: CheckFlags(args)
 * Check command line arguments for remaining flags. Update 
 * flags struct accordingly. Return nothing.
 ***********************************************************/
void CheckFlags (char** args)
{
	// check if blank line or entered as comment
	if (args[0] == NULL || strncmp(args[0], "#", 1) == 0)
	{
		flag.emptyOrComment = 1;
	}

	// check if background process character is last argument
	else if (strcmp(args[flag.numArgs - 1], "&") == 0)
	{
		flag.background = 1;

		// remove & from args and decrement numArgs
		args[flag.numArgs - 1] = NULL;
		flag.numArgs--;
	}
	else
	{
		// do nothing
	}
}

/***********************************************************
 * Function: BuiltInFunctions(args)
 * Runs one of the built-in commands based on command line 
 * user input in args. Can run "exit", "cd" and "status". 
 * Returns nothing.
 ***********************************************************/
void BuiltInFunctions(char** args)
{
	// check if built-in exit function is called, & is accepted but ignored
	if (strcmp(args[0], "exit") == 0 && (args[1] == NULL || strcmp(args[1], "&") == 0))
	{
		// kill any background processes
		int i;
		for (i = 0; i < currBgProcess; i++)
		{
			kill(bgProcesses[i], SIGKILL);
		}

		// exit shell
		exit(0);
	}
	// check if built-in cd function is called
	else if (strcmp(args[0], "cd") == 0)
	{
		// if only arg is "cd" go to home dir
		if (flag.numArgs == 1)
		{
			chdir(getenv("HOME"));
		}

		// try to change dir to given arguement
		else if (chdir(args[1]) == 0)
		{
			// directory successfully changes
		}

		// if not, print error message for user
		else
		{
			printf("%s is not a valid directory\n", args[1]);
			fflush(stdout);
		}
	}
	// otherwise must be status function 
	else if (strcmp(args[0], "status") == 0 && args[1] == NULL)
	{
		// check if no prior applicable processes have run
		if (lastStatus == -1)
		{
			printf("exit value 0\n");
			fflush(stdout);
		} 

		// if last applicable process completed, print exit status
		else if (isStatus == 1)
		{
			printf("exit value %d\n", lastStatus);
			fflush(stdout);
		}

		// if last applicable process was terminated by signal, print terminating signal
		else if (isStatus == 0)
		{
			printf("terminated by signal %d\n", lastStatus);
			fflush(stdout);
		}
		else
		{
			printf("Something has gone wrong\n");
			fflush(stdout);
		}
	}
}

/***********************************************************
 * Function: NonBuiltInFunctions(args, sigint, sigtstp)
 * Runs any non
 ***********************************************************/
void NonBuiltInFunctions(char** args, struct sigaction sigint, struct sigaction sigtstp)
{

	pid_t pid;
	int status;
	int result;
	int sourceFD;
	int targetFD;

	pid = fork();
	// error with fork
	if (pid == -1)
	{
		perror("Fork error\n");
		exit(1);
	}

	// child process
	else if (pid == 0)
	{
		// sigint can halt foreground child processes
		if (flag.background == 0)
		{
			sigint.sa_handler = SIG_DFL;
			sigaction(SIGINT, &sigint, NULL);
		}

		// sigtstp signals will be ignored by all child processes
		sigtstp.sa_handler = SIG_IGN;
		sigaction(SIGTSTP, &sigtstp, NULL);

		// redirect stdin if needed
		if (flag.redirectIn == 1)
		{
			// if file is readable, read from it
			sourceFD = open(args[flag.reInFile], O_RDONLY);
			if (sourceFD != -1)
			{
				// if redirection fails, alert user
				result = dup2(sourceFD, 0);
				if (result == -1)
				{
					perror("Error with stdin redirection\n");
					exit(1);
				}
			}
		
			// otherwise alert user that file is unreadable
			else
			{
				perror("Cannot open input file\n");
				exit(1);
			}
		}
	
		// if background and has no input redirect, send from /dev/null
		else if (flag.background == 1 && backgroundDisable == 0)
		{
			// if source is readable, read from it
			sourceFD = open("/dev/null", O_RDONLY);
			if (sourceFD != -1)
			{
				// if redirection fails, alert user
				result = dup2(sourceFD, 0);
				if (result == -1)
				{
					perror("Error opening /dev/null");
					exit(1);	
				}
			}
		}
			
		// redirect stdout if needed
		if (flag.redirectOut == 1)
		{
			// if file doesn't exist, create it. If it does exists, write to it
			targetFD = open(args[flag.reOutFile], O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (targetFD == -1)
			{
				perror("Cannot open output file\n");
				exit(1);
			}
				
			// if redirection fails, alert user
			else
			{
				result = dup2(targetFD, 1);
				if (result == -1)
				{
					perror("Error with stdout redirection\n");
					exit(1);
				}
			}
		}

		// if background and has no output redirect, send to /dev/null
		else if (flag.background == 1 && backgroundDisable == 0)
		{
			// if source is writeable,  from it
			targetFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (targetFD != -1)
			{
				// if redirection fails, alert user
				result = dup2(targetFD, 1);
				if (result == -1)
				{
					perror("Error opening /dev/null");
					exit(1);	
				}
			}
		}
		
		// execute function
		if (execvp(args[0], args) == -1)
		{
			perror("Execution statement failed\n");
			exit(1);
		}
		exit(0);
	}

	// parent process
	else
	{			
		// run in background if it's flagged AND not disabled
		if (flag.background == 1 && backgroundDisable == 0)
		{
			// store pids to ensure they get killed later
			bgProcesses[currBgProcess] = pid;
			currBgProcess++;

			waitpid(pid, &status, WNOHANG);
			printf("background pid is %d\n", pid);
			fflush(stdout);			
		}
	
		// otherwise run in foreground
		else
		{
			waitpid(pid, &status, 0);
			
			// if process completed normally, store exit status
			if (WIFEXITED(status) != 0)
			{
				lastStatus = WEXITSTATUS(status);
				isStatus = 1;
			}

			// otherwise, store terminating signal
			if (WIFSIGNALED(status) != 0)
			{
				lastStatus = WTERMSIG(status);
				isStatus = 0;

				// also print out exit status
				printf("terminated by signal %d\n", lastStatus);
				fflush(stdout);
			}
		}
	}

	// listen for background processes completing 
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		printf("Background pid %d is done: ", pid);

		// if processes completed normally, show exit status
		if (WIFEXITED(status) != 0)
		{
			printf("exit value %d\n", status);
			fflush(stdout);				
		}

		// otherwise, show terminating signal
		else
		{
			printf("terminated by signal %d\n", status);
			fflush(stdout);
		}
	}
}


/***********************************************************
 * Function: SwitchBackgroundMode(signo)
 * Toggles on or off whether input processes can be run in 
 * the background with '&'. Returns nothing.
 ***********************************************************/
void SwitchBackgroundMode(int signo)
{
	char* message = "\nNew processes can no longer be run in background\n";

	// disable if enabled
	if (backgroundDisable == 0)
	{
		// new background processes now disabled
		backgroundDisable = 1;
		// can't use printf due to reentrancy issue
		write(STDOUT_FILENO, message, 50);
	}

	// enable if disabled
	else
	{
		// new background processes now re-enabled
		backgroundDisable = 0;
		message = "\nNew processes can now be run in the background\n";
		write(STDOUT_FILENO, message, 48);
	}
}

int main ()
{
	// setup signal handler for CTRL-C
	struct sigaction SIGINT_action = {0};

	// don't run default terminate
	SIGINT_action.sa_handler = SIG_IGN;
	SIGINT_action.sa_flags = SA_RESTART;

	// setup signal handler for CTRL-Z
	struct sigaction SIGTSTP_action = {0};

	// use function instead of default stop 
	SIGTSTP_action.sa_handler = SwitchBackgroundMode;
	SIGTSTP_action.sa_flags = SA_RESTART;

	// link signals to use created handlers
	sigaction(SIGINT, &SIGINT_action, NULL);
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	//run program until exit is entered
	while(1)
	{
		// check if 
		//ProcessEnder();

		// get input from command line
		char* userInput = GetUserInput();

		// set all flags to false
		ClearFlags();

		// read user input word by word and set flags
		char** args = ReadInput(userInput);
		
		// check for remaining flags
		CheckFlags(args);

		// check if input is not a comment or empty
		if (flag.emptyOrComment == 0)
		{
			// check if input is a built-in function
			if ((strcmp(args[0], "exit") == 0) || (strcmp(args[0], "cd") == 0) || (strcmp(args[0], "status") == 0))
			{
				BuiltInFunctions(args);
			}
			
			// otherwise run it as a non-built-in function
			else
			{
				NonBuiltInFunctions(args, SIGINT_action, SIGTSTP_action);
			}
		}
	}
	return 0;
}
