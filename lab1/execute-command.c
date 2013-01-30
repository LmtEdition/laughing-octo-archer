// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdbool.h> //bool

#include <fcntl.h>
#include <unistd.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

// 1 indicates command failed
// 0 indicates command success
/*
bool exec_andor(command_t c) {
   
    //When parallezing, WAIT FOR EXIT VALUES

    if(c->type == AND_COMMAND) {

        //If the left side evaluates to true we must also evaluate the left side
        //Otherwise return false indicating that command failed

        return execute_command(c->u.command[0]) &&  execute_command(c->u.command[1]);   
  

    } else {
        // OR_COMMAND
        // If the left side is true return 0 (success immediately)
        // Otherwise also evaluate right side

        return execute_command(c->u.command[0]) || execute_command(c->u.command[1]);

    }
}
*/
/**
 *
 *  Execution for sequence command ;
 *  executes left and right command and returns 0 indicating that sequence was successfully executed
 *
 */
/*
bool exec_sequence(command_t c) {


    //execute left
    execute_command(c->u.command[0]);

    //execute right
    execute_command(c->u.command[1]);

    //essentially a place holder because postorder traversal executes left the right
    return true;
}

bool exec_pipe(command_t c){

}

bool exec_simple(command_t c) {
    //file io
    //if input redirection <
    int fd_i , fd_o;

    if(c->input) {

        if((fd_i = open(input,O_RDONLY)) == -1)) {

           error(1, 0, "Error opening input file: %s\n", c->input);

        } else {
            //closes fd and and performs redirection to specified file descriptor 
            //such that input is read from file
            if(dup2(fd_i,0) < 0) {
                error(1, 0, "Error using dup2 for input redirection");
            }
        }
    }

    //if output redirection
    if(c->output) {
        if((fd_o = open(output,O_WRONLY)) == -1) {
            error(1, 0, "Error opening output file: %s\n", c->output);
        } else {
             //duplicates output fd into stdout file descriptor such that output redirects into file  
             if(dup2(fd_o,1) < 0) {
                   error(1, 0, "Error using dup2 for output redirection");
             }
        }
    }

    //finally execute function
    if(execvp(c->u.word[0],c->u.word) == -1) {
        return false;
    }

    if(fd_i != -1) {
        close(fd_i);
    }

    if(fd_o != -1) {
        close(fd_o);
    }

    return true;

}

bool exec_subshell(command_t c) {
    //When parallezing WAIT FOR EXIT VALUE
    //return the value of the single child ; essentially a place holder
    return exec_command(c->subshell_command);
 }
*/

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
 */

bool
execute_command (command_t c, bool time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  
  error (1, 0, "command execution not yet implemented");*/

    if(c == NULL)
        return true;

    switch(c->type) {

        //binary commands
        case AND_COMMAND:

            //If the left side evaluates to true we must also evaluate the left side
            //Otherwise return false indicating that command failed

           return execute_command(c->u.command[0],time_travel) &&  execute_command(c->u.command[1],time_travel);  

        case OR_COMMAND:
            // If the left side is true return 0 (success immediately)
            // Otherwise also evaluate right side
            return execute_command(c->u.command[0],time_travel) || execute_command(c->u.command[1],time_travel);

        case SEQUENCE_COMMAND:
        {   //execute left
            execute_command(c->u.command[0],time_travel);

            //execute right
            execute_command(c->u.command[1],time_travel);

            //essentially a place holder because postorder traversal executes left the right
            return true;
        }
        case PIPE_COMMAND:
            
            //return exec_pipe(c);
            //TODO
            return true;

        //unary commands
        case SIMPLE_COMMAND:
        {    //file io
            //if input redirection <
            int fd_i , fd_o;

            if(c->input) {

                if((fd_i = open(c->input,O_RDONLY)) == -1) {

                   error(1, 0, "Error opening input file: %s\n", c->input);

                } else {
                    //closes fd and and performs redirection to specified file descriptor 
                    //such that input is read from file
                    if(dup2(fd_i,0) < 0) {
                        error(1, 0, "Error using dup2 for input redirection");
                    }
                }
            }

            //if output redirection
            if(c->output) {
                if((fd_o = open(c->output,O_WRONLY)) == -1) {
                    error(1, 0, "Error opening output file: %s\n", c->output);
                } else {
                     //duplicates output fd into stdout file descriptor such that output redirects into file  
                     if(dup2(fd_o,1) < 0) {
                           error(1, 0, "Error using dup2 for output redirection");
                     }
                }
            }

            //finally execute function
            if(execvp(c->u.word[0],c->u.word) == -1) {
                return false;
            }

            if(fd_i != -1) {
                close(fd_i);
            }
            
            if(fd_o != -1) {
                close(fd_o);
            }

            return true;
        }
        case SUBSHELL_COMMAND:
            return execute_command(c->u.subshell_command,time_travel);
        default:
              abort ();
    }

}
