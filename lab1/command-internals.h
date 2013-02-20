// UCLA CS 111 Lab 1 command internals

enum command_type
  {
    AND_COMMAND,         // A && B
    SEQUENCE_COMMAND,    // A ; B
    OR_COMMAND,          // A || B
    PIPE_COMMAND,        // A | B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )

    //these two are not command types but simplify conversion of infix to postfix
    LEFT_PAREN, // (
    RIGHT_PAREN // )
  };

// Data associated with a command.
struct command
{
  enum command_type type;

  // Exit status, or -1 if not known (e.g., because it has not exited yet).
  int status;

  // I/O redirections, or null if none.
  char *input;
  char *output;

  union
  {
    // for AND_COMMAND, SEQUENCE_COMMAND, OR_COMMAND, PIPE_COMMAND:
    struct command *command[2];

    // for SIMPLE_COMMAND:
    char **word;

    // for SUBSHELL_COMMAND:
    struct command *subshell_command;
  } u;

  // number of concurrent subprocesses including itself, for design problem
  int num_subproc; 
};

// holds an array of top level commands
struct command_stream
{
	command_t* cmds; // array of command_t
	int size; // size of array
	int idx; // index for read_command_stream
    int max_num_subproc; // the max number of sub processes among all commands
};

