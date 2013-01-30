// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdbool.h> //bool

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

// 1 indicates command failed
// 0 indicates command success

bool exec_andor(command_t c) {
   
    //When parallezing, WAIT FOR EXIT VALUES

    if(c->type == AND_COMMAND) {

        //If the left side evaluates to true we must also evaluate the left side
        //Otherwise return false indicating that command failed

        return execute_command(c->command[0]) &&  execute_command(c->command[1]);   
  

    } else {
        // OR_COMMAND
        // If the left side is true return 0 (success immediately)
        // Otherwise also evaluate right side

        return execute_command(c->command[0]) || execute_command(c->command[1]);

    }
}

/**
 *
 *  Execution for sequence command ;
 *  executes left and right command and returns 0 indicating that sequence was successfully executed
 *
 */

bool exec_sequence(command_t) {


    //execute left
    execute_command(c->command[0]);

    //execute right
    execute_command(c->command[1]);

    //essentially a place holder because postorder traversal executes left the right
    return true;
}

bool exec_pipe(command_t c){

}

bool exec_simple(command_t c) {
    Includes redirect handling 
    //file io
    //if input redirection <
    if(c->input)
        int fd = open(pathname,...);
        read(fd,...);
        close(fd);
    //if output redirection
    if(c->output)
        int fd = open(pathname,....);
        write(fd,...);
        close(fd);

}

bool exec_subshell(command_t c) {
    //When parallezing WAIT FOR EXIT VALUE
    //return the value of the single child ; essentially a place holder
    return exec_command(c->subshell_command);
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
 */

bool
execute_command (command_t c, bool time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  
  error (1, 0, "command execution not yet implemented");*/
        
    //PSEUDOCODE    
    if(command_t == NULL)
        return true;

    switch(c->type) {

    //binary commands
    case AND_COMMAND:
    case OR_COMMAND:
        return execute_andor(c);
    case SEQUENCE_COMMAND:
        return exec_sequence(c);
  
    case PIPE_COMMAND:
        return exec_pipe(c);

    //unary commands
    case SIMPLE_COMMAND:
        return exec_simple(c);
    case SUBSHELL_COMMAND:
        return exec_subshell(c);
    default:
          abort ();
    }

}
