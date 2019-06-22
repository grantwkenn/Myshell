/****************************************************************
* Name        : Grant Kennedy                                               
* Class       : CSC 415                                        *
* Date        : October 2018                                               
* Description :  Writing a simple bash shell program          *
*                that will execute simple commands. The main   *
*                goal of the assignment is working with        *
*                fork, pipes and exec system calls.            *
****************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>

#define BUFFERSIZE 256
#define MYARGSIZE 6

int parseArgs(char** myargv, int* myargc, char *buffer, int* backgroundFlag);
void pwd();
void execute(char** myargv, int* myargc, int pipeIndex, int* backgroundFlag);


int main(int* argc, char** argv)
{
	int in = dup(0);
	int out = dup(1);
	
	int* myargc = malloc(sizeof(int*));
	char** myargv = malloc(sizeof(char*) * MYARGSIZE);
	char* buffer = malloc(BUFFERSIZE);
	*myargc = 0;
	int* bgFlag = malloc(sizeof(int*));
	
	while (1)
	{
		*bgFlag = 0;
		memset(buffer, '\0', BUFFERSIZE);
		pwd();
		fgets(buffer, BUFFERSIZE, stdin);
		int pipeIndex = parseArgs(myargv, myargc, buffer, bgFlag);
		execute(myargv, myargc, pipeIndex, bgFlag);
		dup2(in, 0);
		dup2(out, 1);
		for (int i = 0; i < MYARGSIZE; i++)
		{
			myargv[i] = '\0';
		}
		*myargc = 0;
	}
	return 0;
}


int parseArgs(char** myargv, int* myargc, char* buffer, int* backgroundFlag)
{
	char* cp;
	cp = strtok(buffer, " \n");
	int pipeIndex = 0;
	int i = 0;

	while (cp != NULL)
	{
		if ((strcmp(cp, ">")) == 0)
		{
			//NULL terminate argv, open the file
			int fd;
			cp = strtok(NULL, " \n");
			myargv[i] = '\0';
			fd = creat(cp, S_IRWXU);
			if (fd == -1)
				printf("ERROR");
			dup2(fd, 1);
			close(fd);
			return 0;
		}
		else if ((strcmp(cp, ">>")) == 0)
		{
			//NULL terminate argv, open the file
			int fd;
			cp = strtok(NULL, " \n");
			myargv[i] = '\0';
			fd = open(cp, O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
			if (fd == -1)
				printf("ERROR");
			dup2(fd, 1);
			close(fd);
			return 0;
		}
		else if ((strcmp(cp, "<")) == 0)
		{
			//NULL terminate argv, open the file
			int fd;
			cp = strtok(NULL, " \n");
			myargv[i] = '\0';
			fd = open(cp, O_RDONLY);
			if (fd == -1)
				printf("ERROR");
			dup2(fd, 0);
			return 0;
		}
		else if ((strcmp(cp, "|")) == 0)
		{
			pipeIndex = i;
			cp = strtok(NULL, " \n");
		}
		else if ((strcmp(cp, "&")) == 0)
		{
			*backgroundFlag = 1;
			return pipeIndex;
		}
		else
		{
			myargv[i] = cp;
			cp = strtok(NULL, " \n");
			*myargc = *myargc + 1;
			i++;
		}
	}
	return pipeIndex;
}

void execute(char** myargv, int* myargc, int pipeIndex, int* backgroundFlag)
{
	
	if (*myargv == NULL)
		return;

	if (strcmp(*myargv, "exit") == 0)
	{
		exit(0);
	}

	if (strcmp(*myargv, "cd") == 0)
	{
		if (*myargc >= 2)
			if (chdir(myargv[1]) < 0)
			{
				printf("cd: %s", myargv[1]);
				printf(": No such file or directory\n");
			}
	}

	else if (strcmp(*myargv, "pwd") == 0)
	{
		char dir[BUFFERSIZE];
		getcwd(dir, sizeof(dir));
		printf("%s\n", dir);
	}

	else if (pipeIndex > 0)
	{ //exec pipe command
		char** firstCommand = malloc(sizeof(char*) * MYARGSIZE);
		for (int i = 0; i < pipeIndex; i++)
		{
			firstCommand[i] = myargv[i];
		}

		//pipe
		int pd[2];
		if (pipe(pd) < 0)
		{
			fprintf(stderr, "Error");
			return;
		}

		//fork
		pid_t pid, pid2;
		if ((pid = fork()) < 0) //fork failed
		{
			fprintf(stderr, "Error");
			return;
		}
		if (pid == 0)
		{ //child process
			close(pd[0]);
			dup2(pd[1], 1);
			close(pd[1]);
			if ((execvp(firstCommand[0], firstCommand)) < 0)
			{
				exit(0);
			}
		}
		else
		{ //parent process	
			close(pd[1]);
			wait(NULL);

			if ((pid2 = fork()) < 0)
			{
				fprintf(stderr, "Error");
				return;
			}
			else if (pid2 == 0)
			{
				dup2(pd[0], 0);
				close(pd[0]);
				if ((execvp(myargv[pipeIndex], &myargv[pipeIndex])) == -1)
					exit(0);
			}
			else
			{
				wait(NULL);
			}
		}
	}

	else //NO PIPE
	{
		pid_t pid;
		pid = fork();
		if (pid == -1) //fork failed
		{
			fprintf(stderr, "Error");
			return;
		}
		if (pid == 0)
		{ //child process
			if (execvp(*myargv, myargv) == -1)
			{
				printf("%s: Command not found\n", *myargv);
				exit(0);
			}
		}
		if (pid > 0)
		{
			if (*backgroundFlag == 0)
			{
				wait(NULL);
			}
		}
	}
}

void pwd()
{
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;
	int len = strlen(homedir);
	char cwd[BUFFERSIZE];

	getcwd(cwd, sizeof(cwd));
	if (strncmp(homedir, cwd, len) == 0)
	{
		printf("myShell ~ ");
		printf("%s >> ", &cwd[len]);
	}
	else
		printf("myShell %s >> ", cwd);
}

