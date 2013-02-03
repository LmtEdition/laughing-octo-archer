// UCLA CS 111 Lab 1 command execution
#include "command.h"
#include "command-internals.h"

#include <sys/types.h>
#include <sys/wait.h>  //waitpid

#include <error.h>
#include <errno.h>  //errno
#include <string.h> //strerror
#include <unistd.h> //pipe,
#include <stdbool.h> //bool
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include "parallel-command.h"

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

void free_command(command_t c) {
	if (c == NULL)
		return;
	switch (c->type)
	{
		case AND_COMMAND:
		case SEQUENCE_COMMAND:
		case OR_COMMAND:
		case PIPE_COMMAND:
		{
			free_command(c->u.command[0]);
			free_command(c->u.command[1]);
			break;
		}
		case SIMPLE_COMMAND:
		{
			if (c->input)
				free(c->input);
			else if (c->output)
				free(c->output);
			char **w = c->u.word;
			while (*w)
				free(*w++);
			free(c->u.word);
			break;	
		}
		case SUBSHELL_COMMAND:
			free_command(c->u.subshell_command);
			break;
		default:
			fprintf(stderr, "Command has wrong type in free_command");
	}
	free(c);
}


int command_status (command_t c)
{
  return c->status;
}

/**
 *  
 *   PostOrder Traversal (DFS) because highest to lowest precedence from bottom up
 *   execute commands on cur (after looking at left, then right)
 *   Look at command type to check for binary operators
 *   
 *   Exit status:
 *   	0 means success
 *   	1 means fail
 *   	-1 means unknown
 */
void exec_simple_command(command_t c, int* fd_i, int* fd_o, bool exec) {
	if (c == NULL)
		return;
	if(c->input) {
		if ((*fd_i = open(c->input, O_RDONLY)) == -1) {
			fprintf(stderr, "%s\n", strerror(errno));
			c->status = 1;
			return;
		   //error(1, 0, "Error opening input file: %s\n", c->input);
		} else {
			//closes fd and performs redirection to specified file descriptor 
			//such that input is read from file
			if (dup2(*fd_i, 0) < 0) {
				//error(1, 0, "Error using dup2 for input redirection");
				fprintf(stderr, "%s\n", strerror(errno));
				c->status = 1;
				return;
			}
		}
	}

	//if output redirection
	if (c->output) {
		if ((*fd_o = open(c->output, O_RDWR | O_CREAT, 0666)) == -1) {
			//error(1, 0, "Error opening output file: %s\n", c->output);
			fprintf(stderr, "%s\n", strerror(errno));
			c->status = 1;
			return;
		} else {
			 //duplicates output fd into stdout file descriptor such that output redirects into file  
			 if(dup2(*fd_o,1) < 0) {
				   //error(1, 0, "Error using dup2 for output redirection");
				fprintf(stderr, "%s\n", strerror(errno));
				c->status = 1;
				return;
			 }
		}
	}
	if (exec)
		execvp(c->u.word[1], c->u.word + 1);
	else 
		execvp(c->u.word[0], c->u.word);
	fprintf(stderr, "%s\n", strerror(errno));
}

void
exec_command(command_t c) {
    if(c == NULL)
		return;
    switch(c->type) {
        //binary commands
        case AND_COMMAND:
            //If the left side evaluates to true we must also evaluate the left side 
			////Otherwise return false indicating that command failed 
			exec_command(c->u.command[0]);
			if (command_status(c->u.command[0]) == 0) {
				exec_command(c->u.command[1]);  
				c->status = command_status(c->u.command[1]);
			}
			else
				c->status = command_status(c->u.command[0]);
			break;

        case OR_COMMAND:
            // If the left side is true return 0 (success immediately)
            // Otherwise also evaluate right side
			exec_command(c->u.command[0]);
			if (command_status(c->u.command[0]) == 1) {
				exec_command(c->u.command[1]);  
				c->status = command_status(c->u.command[1]);
			}
			else
				c->status = command_status(c->u.command[0]);
			break;

        case SEQUENCE_COMMAND:
            //execute left
            exec_command(c->u.command[0]);

            //execute right
            exec_command(c->u.command[1]);
			c->status = command_status(c->u.command[0]) || command_status(c->u.command[1]);
			break;
        
        case PIPE_COMMAND:
		{		
			int fd[2];
			pid_t childpid;

			pipe(fd);

			if ((childpid = fork()) == -1) {
				c->status = 1;
				break;
			}

			if (childpid == 0) {
				// child process closes read/input side of pipe
				close(fd[0]);
            
				// redirect write/output side of pipe stdout
				dup2(fd[1], 1);
				exec_command(c->u.command[0]);
				close(fd[1]);

				if (command_status(c->u.command[0]) == 0)
					_exit(0);
				else
					_exit(1);
			} else {
				// parent process closes write/output side of pipe
				close(fd[1]);

				// redirect read/input side of pipe to stdin
				dup2(fd[0], 0);

				exec_command(c->u.command[1]);
				close(fd[0]);
			
				int status;
		 	    waitpid(childpid, &status, 0);		

				// execute failed
				if (WEXITSTATUS(status) == 1 || command_status(c->u.command[1]) != 0)
					c->status = 1;	
				else 
					c->status = 0;
			}
			break;
		}	
        //unary commands
        case SIMPLE_COMMAND:
		{
            //file io
            //if input redirection <
            int fd_i = -1, fd_o = -1;

			// if exec keyword is found, then do not fork a child process
			if (strcmp(c->u.word[0], "exec") == 0) 
				exec_simple_command(c, &fd_i, &fd_o, true);

					
            //finally execute function, must fork or execvp exits process
			pid_t childpid;
			if ((childpid = fork()) == -1) {
				fprintf(stderr, "%s\n", strerror(errno));
				c->status = 1;			
				break;
			}

			// child process
			if (childpid == 0) {
				exec_simple_command(c, &fd_i, &fd_o, false);
				_exit(1);
			// parent process
			} else { 
				int status;
		 	    waitpid(childpid, &status, 0);		

				if (fd_i != 1)
					close(fd_i);
				if (fd_o != 1)
					close(fd_o);

				// execvp failed and child process exited correctly
				if (WEXITSTATUS(status) == 1) {
					//printf("false\n");
					c->status = 1;
				} else {
					//printf("true\n");
					c->status = 0;
				}
			}
			break;	
        } 
        case SUBSHELL_COMMAND:
            exec_command(c->u.subshell_command);
			c->status = command_status(c->u.subshell_command);
			break;
		default:
			c->status = -1;
    }
}

int
execute_command (command_stream_t c_stream, bool time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  
  error (1, 0, "command execution not yet implemented");*/

    command_t cmd;
    int last_command_status;
    file_t** file_system = NULL;
    int folder_count = 0;

    if(time_travel) {

    //first build file system
    	build_file_system(c_stream,&file_system, &folder_count);

    //build dependency graph

    //execute commands in parallel


    } else {

    	while ((cmd = read_command_stream (c_stream)))
		    {
		    
		    	  exec_command (cmd);
		    	  last_command_status = command_status(cmd);
		    }
    }


	return last_command_status;
}
