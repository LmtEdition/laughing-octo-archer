// UCLA CS 111 Lab 1 command execution
#include "command.h"
#include "command-internals.h"
#include "alloc.h" //checked_malloc

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
      free(*w);
			while (*++w)
				free(*w);
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

                  free_command(c); // delete childes copy
					if (command_status(c->u.command[0]) == 0) {
						_exit(0);
                      }
					else {
						_exit(1);
                      }
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
          free_command(c);

					_exit(1);
				// parent process
				} else { 
					int status;
					waitpid(childpid, &status, 0);		

					if (fd_i != -1)
					close(fd_i);
					if (fd_o != -1)
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
execute_command(command_stream_t c_stream, bool time_travel, int N)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  
  error (1, 0, "command execution not yet implemented");*/

    command_t cmd;
    int last_command_status;
    file_t** file_system = NULL;
    int folder_count = 0;
    int total_subproc = 0; // keep track of total number of subprocesses

    if (time_travel) {
        if (N < c_stream->max_num_subproc) {
            fprintf(stderr, "Error: number of subprocesses specified is too low\n");
            exit(1);
        }



      //first build file system
      build_file_system(c_stream, &file_system, &folder_count);

      //build dependency graph
      int *wait_queue = NULL;
      bool **dep_graph = create_dep_graph(&file_system, &folder_count, &wait_queue);
      clean_file_system(&file_system, &folder_count);

      // create an array for holding pid corresponding to commands
      pid_t *command_pids = (pid_t *)checked_malloc(sizeof(pid_t) * folder_count);
      int i;
      for (i = 0; i < folder_count; i++) 
        command_pids[i] = 0; 

      //execute commands in parallel
      int commands_not_finished = folder_count;

      pid_t pid;

      while (commands_not_finished > 0) {
        // fork child processes for runnable commands
        for (i = 0; i < folder_count; i++) {

          if ( (wait_queue[i] == 0) && (c_stream->cmds[i]->status == 2)
                 && (total_subproc + c_stream->cmds[i]->num_subproc <= N) ) {
              total_subproc += c_stream->cmds[i]->num_subproc;
              printf("Executing top level command type: %d, Total number of subprocesses: %d\n", c_stream->cmds[i]->type, total_subproc);

            pid = fork();
            if (pid == 0) {

              // child process executes command
              exec_command(c_stream->cmds[i]);
              free_dep_graph_and_wait_queue(&dep_graph, folder_count, &wait_queue);
              //deallocated copied memory
              int x, child_status;
              child_status = c_stream->cmds[i]->status; 
              for (x = 0; x < c_stream->size; x++)
                free_command(c_stream->cmds[x]);
              free(c_stream->cmds);
              free(c_stream);
              free(command_pids);
              _exit(child_status);

            } else if (pid > 0) {

              // parent process set the status to unknown == still running
              c_stream->cmds[i]->status = -1;
              commands_not_finished--;
              command_pids[i] = pid;

            } else {
              perror("Error!");
              exit(1);
            }
          }
        }

        // wait for all children to finish, not good for parallelization
        int status;
        while ( (pid = wait(&status)) > 0) {
          last_command_status = status;

          // update status and wait queue
          i = -1;
          int j;
          for (j = 0; j < folder_count; j++) {
              if (command_pids[j] == pid) {
                i = j;
                continue;
              }

              if ( (wait_queue[j] == 0) && (c_stream->cmds[j]->status == 2)
                     && (total_subproc + c_stream->cmds[j]->num_subproc <= N) ) {
                  total_subproc += c_stream->cmds[j]->num_subproc;
                  printf("Executing top level command type: %d, Total number of subprocesses: %d\n", c_stream->cmds[j]->type, total_subproc);

                pid = fork();
                if (pid == 0) {

                  // child process executes command
                  exec_command(c_stream->cmds[j]);
                  free_dep_graph_and_wait_queue(&dep_graph, folder_count, &wait_queue);
                  //deallocated copied memory
                  int x, child_status;
                  child_status = c_stream->cmds[j]->status; 
                  for (x = 0; x < c_stream->size; x++)
                    free_command(c_stream->cmds[x]);
                  free(c_stream->cmds);
                  free(c_stream);
                  free(command_pids);
                  _exit(child_status);

                } else if (pid > 0) {

                  // parent process set the status to unknown == still running
                  c_stream->cmds[j]->status = -1;
                  commands_not_finished--;
                  command_pids[j] = pid;

                } else {
                  perror("Error!");
                  exit(1);
                }
              }
          }

          if (i != -1) {
            if (c_stream->cmds[i]->status == -1) {
              c_stream->cmds[i]->status = 0;
              //Decrement total subprocesses
              total_subproc -= c_stream->cmds[i]->num_subproc;
              printf("Command finished type: %d, Total number of subprocesses: %d\n", c_stream->cmds[i]->type, total_subproc);

              //decrement all dependency counts, cmd j depends on cmd i that just finished
              for (j = 0; j < folder_count; j++) {

                if (dep_graph[j][i]) {

                  if (wait_queue[j] == 0) {
                    fprintf(stderr,"Wait Queue[%d] is already empty!",j);
                  }
                  wait_queue[j]--;
                  // execute command if runnable
                  if ( (wait_queue[j] == 0) && (c_stream->cmds[j]->status == 2) && (total_subproc + c_stream->cmds[j]->num_subproc <= N) ) {
                      total_subproc += c_stream->cmds[j]->num_subproc;
                      printf("Executing top level command type: %d, Total number of subprocesses: %d\n", c_stream->cmds[i]->type, total_subproc);

                    pid = fork();
                    if (pid == 0) {

                      // child process executes command
                      exec_command(c_stream->cmds[j]);
                      free_dep_graph_and_wait_queue(&dep_graph, folder_count, &wait_queue);
                      //deallocated copied memory
                      int x, child_status;
                      child_status = c_stream->cmds[j]->status; 
                      for (x = 0; x < c_stream->size; x++)
                        free_command(c_stream->cmds[x]);
                      free(c_stream->cmds);
                      free(c_stream);
                      free(command_pids);
                      _exit(child_status);

                    } else if (pid > 0) {

                      // parent process set the status to unknown == still running
                      c_stream->cmds[j]->status = -1;
                      commands_not_finished--;
                      command_pids[j] = pid;

                    } else {
                      perror("Error!");
                      exit(1);
                    }

                  }
                }
              }
            }
          }
        }
      }
      // free file system and dependency graph and wait queue and command pid array
      free_dep_graph_and_wait_queue(&dep_graph, folder_count, &wait_queue);
      free(command_pids);

    } else {

      while ((cmd = read_command_stream (c_stream))) {
        exec_command (cmd);
        last_command_status = command_status(cmd);
      }
    }
    
    int x;
    for (x = 0; x < c_stream->size; x++)
      free_command(c_stream->cmds[x]);

    return last_command_status;
}

