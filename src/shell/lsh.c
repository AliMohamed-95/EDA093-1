/* 
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file 
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive: 
      $> tar cvf lab1.tgz lab1/
 *
 * All the best 
 */


#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"

#include <sys/stat.h> 
#include <fcntl.h>

/*
 * Function declarations
 */

void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);
int ExecuteSimpleCommand(Command *cmd);

/* When non-zero, this global means the user is done using this program. */
int done = 0;

/*
 * Name: main
 *
 * Description: Gets the ball rolling...
 *
 */
int main(void)
{
  Command cmd;
  int n;

  while (!done) {

    char *line;
    line = readline("> ");

    if (!line) {
      /* Encountered EOF at top level */
      done = 1;
    }
    else {
      /*
       * Remove leading and trailing whitespace from the line
       * Then, if there is anything left, add it to the history list
       * and execute it.
       */
      stripwhite(line);

      if(*line) {
        add_history(line);
        /* execute it */
        n = parse(line, &cmd);
		  ExecuteSimpleCommand(&cmd);
        PrintCommand(n, &cmd);
      }
    }
    
    if(line) {
      free(line);
    }

	/* Waits for any zombie processses and cleans them up */
	waitpid(-1, NULL, WNOHANG);
  }
  return 0;
}

/*
 * Name: ExecutePipedCommand
 *
 * Description: Executes a command and pipes the output to the next command in the pgm.
 *
 */
int ExecutePipedCommand(Command *cmd, struct c *pgm) {
	int pid;
	int fd[2];
	int ret;

	if(pipe(fd) == -1) {
		fprintf(stderr, "Pipe failed.\n");
		return 1;
	}

	pid = fork();

	if(pid < 0) {
		fprintf(stderr, "Fork Failed. ExecutePipedCommand.\n");
		return 1;
	} else if(pid == 0) { /* Child code */
		/* Redirect standard output and either keep recurring or execute command if last in chain */
		close(fd[0]);
		dup2(fd[1], 1);
		pgm = pgm->next;
		if(pgm->next != NULL) {
			ExecutePipedCommand(cmd, pgm);
		} else {
			/* Redirecting standard input */
			if(cmd->rstdin != NULL) {
				int infd = open(cmd->rstdin, 0);
				dup2(infd, 0);
			}
			execvp(pgm->pgmlist[0], pgm->pgmlist);
		}
	} else { /* Parent code */
		close(fd[1]);
		dup2(fd[0], 0);
		execvp(pgm->pgmlist[0], pgm->pgmlist);
	}
}

/*
 * Name: ExecuteSimpleCommand
 *
 * Description: Executes a command
 *
 */
int ExecuteSimpleCommand(Command *cmd) {
	int pid;
	struct c *pgm = cmd->pgm;

	pid = fork();

	if(pid < 0) {
		fprintf(stderr, "Fork Failed. ExecuteSimpleCommand.\n");
		return 1;
	} else if(pid == 0) {
		/* Redirect standard output and error as needed */
		if(cmd->rstdout != NULL) {
			int fd = open(cmd->rstdout, O_CREAT|O_RDWR, S_IRWXU);
			dup2(fd, 1);
		}
		if(cmd->rstderr != NULL) {
			int fd = open(cmd->rstderr, O_CREAT|O_RDWR, S_IRWXU);
			dup2(fd, 2);
		}
		if(pgm->next != NULL) {
			/* If we have pipes, recursivly execute them instead */
			ExecutePipedCommand(cmd, pgm);
		} else {
			/* Redirect standard input */
			if(cmd->rstdin != NULL) {
				int fd = open(cmd->rstdin, 0);
				dup2(fd, 0);
			}
			/* Execute the requested command, execvp handles the path */
			execvp(pgm->pgmlist[0], pgm->pgmlist);
		}
	} else {
		/* If it is a background process, don't wait */
		if(cmd->bakground == 0) {
			waitpid(pid, NULL, 0);
		}
		return 0;
	}
}

/*
 * Provided functions
 */

/*
 * Name: PrintCommand
 *
 * Description: Prints a Command structure as returned by parse on stdout.
 *
 */
void
PrintCommand (int n, Command *cmd)
{
  printf("Parse returned %d:\n", n);
  printf("   stdin : %s\n", cmd->rstdin  ? cmd->rstdin  : "<none>" );
  printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>" );
  printf("   bg    : %s\n", cmd->bakground ? "yes" : "no");
  PrintPgm(cmd->pgm);
}

/*
 * Name: PrintPgm
 *
 * Description: Prints a list of Pgm:s
 *
 */
void
PrintPgm (Pgm *p)
{
  if (p == NULL) {
    return;
  }
  else {
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    PrintPgm(p->next);
    printf("    [");
    while (*pl) {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}

/*
 * Name: stripwhite
 *
 * Description: Strip whitespace from the start and end of STRING.
 */
void
stripwhite (char *string)
{
  register int i = 0;

  while (isspace( string[i] )) {
    i++;
  }
  
  if (i) {
    strcpy (string, string + i);
  }

  i = strlen( string ) - 1;
  while (i> 0 && isspace (string[i])) {
    i--;
  }

  string [++i] = '\0';
}
