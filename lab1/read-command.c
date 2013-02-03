// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h" //memory allocation
#include <stdio.h> //EOF, ungetc
#include <stdlib.h> //free
#include <string.h> //string functions
#include <error.h>
#include <ctype.h> //isalnum, isspace
#include <stdbool.h> //bool

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

bool
is_simple_char(char byte)
{
  if (isalnum(byte) || byte == '!' || byte == '%' || byte == '+' || 
      byte == ',' || byte == '-' || byte == '.' || byte == '/' ||
      byte == ':' || byte == '@' || byte == '^' || byte == '_')
    return true;
  return false;
}

command_t
create_command(enum command_type type)
{
  command_t new_cmd = (command_t)checked_malloc(sizeof(struct command));
  new_cmd->type = type;
  
  new_cmd->status = -1;

  new_cmd->input = NULL;
  new_cmd->output = NULL;

  return new_cmd;
}

void 
print_error(int line_number, char *msg)
{
  //fprintf(stderr, "%d: syntax error\n", line_number);
  error(1, 0, "%d: %s\n", line_number, msg);
}

// create_simple_command with strtok
command_t
create_simple_command(char *cmd_string, char *input, char *output)
{
  command_t simple_cmd = create_command(SIMPLE_COMMAND);
  simple_cmd->input = input;
  simple_cmd->output = output;
 
  size_t byte_size = (size_t) sizeof(char *) * 8;

  // end of word array is NULL string since print-command.c checks for NULL as end of array
  simple_cmd->u.word = (char **)checked_malloc(byte_size);

  // words are substrings delimited by spaces
  char *token = strtok(cmd_string, " ");
  int word_idx = 0;

  // Populate simple command with words
  while (token != NULL)
  {
    if ((word_idx) * sizeof(char *) == byte_size) 
      simple_cmd->u.word = checked_grow_alloc(simple_cmd->u.word, &byte_size);

    simple_cmd->u.word[word_idx] = (char *)checked_malloc( sizeof(char) * (strlen(token)+1) );//+1 for NULL byte at end of C string
    strcpy(simple_cmd->u.word[word_idx], token);
    word_idx++;
     
    token = strtok(NULL, " ");
  } 

  if (word_idx == 0)
  {
    free(simple_cmd->u.word);
    simple_cmd->u.word = NULL;
  }
  else
    simple_cmd->u.word = (char **)checked_realloc(simple_cmd->u.word, sizeof(char *) * (word_idx+1)); //+1 for \0

  // last element of word array must be NULL
  simple_cmd->u.word[word_idx] = NULL;
  
  return simple_cmd;
}

char *
get_next_word(int (*get_next_byte) (void *), void *filestream)
{
  size_t word_size = (size_t) (sizeof(char) * 16);

  char *word = (char *)checked_malloc(word_size);

  int word_idx = 0;
  char cur_byte;
  while( (cur_byte = get_next_byte(filestream)) != EOF)
  {
    if ( is_simple_char(cur_byte))
    {
      // need to make sure enough room for NULL byte
      if ((word_idx + 1) * sizeof(char) == word_size) 
        word = (char *) checked_grow_alloc(word, &word_size); 
      word[word_idx] = cur_byte;
      word_idx++;
    }
    else if (!isspace(cur_byte) || cur_byte == '\n')
    {
      ungetc(cur_byte, filestream);
      break;
    }
  }

  if (word_idx == 0)
  {
    free(word);
    return NULL;
  }
  else
    word = (char *)checked_realloc(word, sizeof(char) *( word_idx + 1));
    
  word[word_idx] = '\0';
  return word;
}

command_t
get_simple_command(int (*get_next_byte) (void *), void *filestream, int line_number)
{
  size_t buf_size = (size_t) (sizeof(char) * 30); //arbitrary, guessing that most simple won't be this big

  char *simple_cmd_buf = (char *)checked_malloc(buf_size);
  char *input_buf = NULL; 
  char *output_buf = NULL;

  int buf_idx = 0;
  
  bool input = false;
  bool output = false;

  char cur_byte;
  while( (cur_byte = get_next_byte(filestream)) != EOF)
  {
    // get simple command w/o redirect
    if (!input && !output && ( is_simple_char(cur_byte) || ( isspace(cur_byte) && cur_byte != '\n') ) )
    {
      // need to make sure enough room for NULL byte
      if ((buf_idx + 1) * sizeof(char) == buf_size) 
        simple_cmd_buf = (char *) checked_grow_alloc(simple_cmd_buf, &buf_size); 
      // all whitespace besides \n is treated as space
      if (isspace(cur_byte))
        cur_byte = ' '; 

      simple_cmd_buf[buf_idx] = cur_byte;
      buf_idx++; 
    }
    else if (!input && cur_byte == '<')
    {
      if (output)
      {
        print_error(line_number, "invalid redirection of this type: a > b < c");
        return NULL;
      }
      input = true;
      input_buf = get_next_word(get_next_byte, filestream);
      if (!input_buf)
      {
        print_error(line_number, "invalid redirection <"); 
        return NULL;
      }
    }
    else if (!output && cur_byte == '>')
    {
      output = true;
      output_buf = get_next_word(get_next_byte, filestream);
      if (!output_buf)
      {
        print_error(line_number, "invalid redirection >"); 
        return NULL;
      }
    }
    else 
    {
      ungetc(cur_byte, filestream);
      break;
    }
       
  }
  // remove whitespaces at end of simple commands and put back ONE whitespace if any were removed
  int i;
  for (i = buf_idx-1; i >= 0; i--)
	  if (simple_cmd_buf[i] != ' ')
		  break;
		  
  // if a whitespace wasn't removed
  if (i != buf_idx-1)
	  ungetc(' ', filestream);

  buf_idx = i + 1;
  simple_cmd_buf[buf_idx] = '\0'; // C string end with NULL byte


  char *free_buf = simple_cmd_buf; // strtok will modify argument string, so keep pointer to free memory

  command_t simple_cmd = create_simple_command(simple_cmd_buf, input_buf, output_buf);
  
  free(free_buf);

  return simple_cmd;
}

bool
is_binary_op(enum command_type type)
{
  return type == AND_COMMAND || type == OR_COMMAND || type == SEQUENCE_COMMAND ||
      type == PIPE_COMMAND;
}

bool
continue_after_newline(enum command_type prev_type, int num_sub_shells)
{
  // always continue if inside subshell or after && || |
  if (num_sub_shells > 0 || ( is_binary_op(prev_type) && prev_type != ';' ))
    return true;
  return false;
}

bool
is_valid_type(enum command_type prev_type, enum command_type cur_type, int num_sub_shells)
{
  // SIMPLE_COMMANDS can follow any other commands

  if (cur_type == RIGHT_PAREN && num_sub_shells <= 0)
    return false;

  else if (is_binary_op(cur_type) || cur_type == RIGHT_PAREN)
    return prev_type == RIGHT_PAREN || prev_type == SIMPLE_COMMAND;

  else if (cur_type == LEFT_PAREN)
    return prev_type != SIMPLE_COMMAND && prev_type != RIGHT_PAREN;

  return false;
}

void add_command_to_infix(command_t **infix_arr, int *infix_idx,
     size_t *infix_size, enum command_type type, enum command_type *prev_type)
{
   if (*infix_idx * sizeof(command_t) == *infix_size)
     *infix_arr = (command_t *)checked_grow_alloc(*infix_arr, infix_size); 

   (*infix_arr)[*infix_idx] = create_command(type);
   (*infix_idx)++;

   *prev_type = type; 
}

void check_end_command_error(int num_sub_shells, enum command_type prev_type,
    int line_number)
{
  //CHECK LAST COMMAND IS VALID BEFORE ENDING
  if (num_sub_shells > 0)
    print_error(line_number, "invalid () matching");
  else if (prev_type != SIMPLE_COMMAND && prev_type != RIGHT_PAREN)
    print_error(line_number, "invalid command ending");
}

command_t *
get_infix_commands(int (*get_next_byte) (void *), void *filestream, int *line_number, int *infix_len)
{
  // NEED TO CHECK IF STARTING A NEW COMMAND STREAM, can't check prev_type in comparisons
  enum command_type prev_type;
  bool get_more_cmds = true; //keeps track of building the current complete command
  int num_sub_shells = 0;

  size_t infix_size = (size_t) sizeof(command_t) * 16;
  command_t *infix_arr = (command_t *)checked_malloc(infix_size);
  int infix_idx = 0;

  int cur_byte;
  bool prev_byte_is_space = true;

  while (get_more_cmds)
  {
    cur_byte = get_next_byte(filestream);

    // only spaces, simple char, (, ), # can be at beginning of line
    if (infix_idx == 0)
    {
      if (cur_byte == EOF)
        break;
      if (!isspace(cur_byte) && !is_simple_char(cur_byte) && 
        cur_byte != '(' && cur_byte != '#')
        print_error(*line_number, "invalid character at beginning of line");
    }

    // NEWLINE
    if (cur_byte == '\n')
    {
      (*line_number)++;
      prev_byte_is_space = true;
      if (infix_idx == 0)
        continue; 

      get_more_cmds = continue_after_newline(prev_type, num_sub_shells);

      if (num_sub_shells > 0 && (prev_type == SIMPLE_COMMAND || prev_type == RIGHT_PAREN))
      {
        //Add sequence command
        add_command_to_infix(&infix_arr, &infix_idx, &infix_size, SEQUENCE_COMMAND, &prev_type); 
      }
      else if (num_sub_shells == 0 && prev_type == ';')
      {
        // remove previous sequence command
        if (infix_idx > 0)
          infix_idx--;
        if (infix_arr[infix_idx]->type == SEQUENCE_COMMAND)
        {
          free(infix_arr[infix_idx]);
          if (infix_idx > 0)
            prev_type = infix_arr[infix_idx-1]->type;
        }
      }

      // CHECK THAT LAST COMMAND IS VALID BEFORE ENDING 
      if (!get_more_cmds && prev_type != SIMPLE_COMMAND && prev_type != RIGHT_PAREN) 
        print_error(*line_number, "invalid command ending");
    }
    // SEQUENCE COMMAND
    else if (cur_byte == ';')
    {
      // create sequence command
      if (is_valid_type(prev_type, SEQUENCE_COMMAND, num_sub_shells))
        add_command_to_infix(&infix_arr, &infix_idx, &infix_size, SEQUENCE_COMMAND, &prev_type); 
      else
        print_error(*line_number, "invalid SEQUENCE command ;");
    }
    // LEFT PAREN
    else if (cur_byte == '(')
    {
      //create LEFT_PAREN command
      if (infix_idx == 0)
        add_command_to_infix(&infix_arr, &infix_idx, &infix_size, LEFT_PAREN, &prev_type); 
      else if (is_valid_type(prev_type, LEFT_PAREN, num_sub_shells))
        add_command_to_infix(&infix_arr, &infix_idx, &infix_size, LEFT_PAREN, &prev_type); 
      else
        print_error(*line_number, "invalid open parenthesis (");

      num_sub_shells++;
    }
    // RIGHT PAREN
    else if (cur_byte == ')')
    {
      if (is_valid_type(prev_type, RIGHT_PAREN, num_sub_shells))
        add_command_to_infix(&infix_arr, &infix_idx, &infix_size, RIGHT_PAREN, &prev_type); 
      else
        print_error(*line_number, "invalid closed parenthesis )");

      if (num_sub_shells <= 0)
        print_error(*line_number, ") has no matching open parenthesis (");

      num_sub_shells--;
    }
    // SIMPLE COMMAND
    else if (is_simple_char(cur_byte))
    {
      // simple commands can follow any other command, no need to check
      ungetc(cur_byte, filestream);

      if (infix_idx * sizeof(command_t) == infix_size)
        infix_arr = (command_t *)checked_grow_alloc(infix_arr, &infix_size); 

      infix_arr[infix_idx] = get_simple_command(get_next_byte, filestream, *line_number);
      infix_idx++;

      prev_type = SIMPLE_COMMAND;

      /* //print for debugging
      char **w = infix_arr[infix_idx-1]->u.word;
      printf("%s", *w);
      while(*++w)
        printf(" %s", *w); 
      
      if (infix_arr[infix_idx-1]->input)
        printf("<%s", infix_arr[infix_idx-1]->input);
      if (infix_arr[infix_idx-1]->output)
        printf(">%s", infix_arr[infix_idx-1]->output);
      printf("\n");
      */
    }
    else if (cur_byte == '&')
    {
      int next_byte = get_next_byte(filestream);

      // create AND command
      if (next_byte == '&')
      {
        if (is_valid_type(prev_type, AND_COMMAND, num_sub_shells))
          add_command_to_infix(&infix_arr, &infix_idx, &infix_size, AND_COMMAND, &prev_type); 
        else
          print_error(*line_number, "invalid AND command");
      }
      else
        print_error(*line_number, "invalid command: single &");
    }
    else if (cur_byte == '|')
    {
      int next_byte = get_next_byte(filestream);

      // create OR command
      if (next_byte == '|')
      {
        if (is_valid_type(prev_type, OR_COMMAND, num_sub_shells))
          add_command_to_infix(&infix_arr, &infix_idx, &infix_size, OR_COMMAND, &prev_type); 
        else
          print_error(*line_number, "invalid OR command");
      }
      // create PIPE command
      else
      {
        ungetc(next_byte, filestream);
        if (is_valid_type(prev_type, PIPE_COMMAND, num_sub_shells))
          add_command_to_infix(&infix_arr, &infix_idx, &infix_size, PIPE_COMMAND, &prev_type); 
        else
          print_error(*line_number, "invalid PIPE command");
      }
    }
    // COMMENT
    else if (cur_byte == '#')
    {
      if (prev_byte_is_space)
      {
        do
        {
          cur_byte = get_next_byte(filestream);
        } while (cur_byte != EOF && cur_byte != '\n');

        if (cur_byte == EOF)
        { 
          get_more_cmds = false;

          //CHECK LAST COMMAND IS VALID BEFORE ENDING
          check_end_command_error(num_sub_shells, prev_type, *line_number);
        }
        else
          (*line_number)++;
      }
      else
        print_error(*line_number, "invalid # after a non-whitespace character");
    }
    else if (cur_byte == EOF)
    {
      get_more_cmds = false;

      //CHECK LAST COMMAND IS VALID BEFORE ENDING
      check_end_command_error(num_sub_shells, prev_type, *line_number);
    }
    // WHITESPACE
    else if (isspace(cur_byte)) 
    {
      prev_byte_is_space = true;
      continue;
    }
    else 
    {
      print_error(*line_number, "invalid character");
    }

    if (!isspace(cur_byte))
      prev_byte_is_space = false;
  }

  if (infix_idx == 0)
  {
    free(infix_arr);
    infix_arr = NULL;
  }
  else
    infix_arr = (command_t *)checked_realloc(infix_arr, sizeof(command_t) * infix_idx);

  *infix_len = infix_idx;
  return infix_arr;
}


/*==== START STACK IMPLEMENTATION ====*/

typedef struct node {

   command_t cmd_t;
   struct node* link;

} STACK_NODE;

typedef struct {

   STACK_NODE* top;

} STACK;

void push (STACK* p, command_t cmd_t) {
    STACK_NODE* pnew = (STACK_NODE*)checked_malloc(sizeof (STACK_NODE));

    pnew->cmd_t = cmd_t;
    pnew->link = p->top;
    p->top = pnew;   
}

bool pop (STACK* p, command_t* cmd_t) {

  STACK_NODE* dlt;

  if(p->top) {
    *cmd_t = p->top->cmd_t;//by reference to change address that cmd_t is referring to
    dlt = p->top;
    p->top = (p->top)->link;
    free(dlt);

    return true;

  } else {

    return false;

  }
}


/*==== END STACK IMPLEMENTATION ====*/

void append_to_postfix(command_t** postfix,command_t cmd_t,int* idx) {
//command_t is a command*

    if(*postfix) {

    *postfix = (command_t*)checked_realloc(*postfix,(*idx + 1) *  sizeof(command_t));

    (*postfix)[*idx] = cmd_t;


    } else {
      *postfix = (command_t*)checked_malloc(sizeof(cmd_t));
      (*postfix)[*idx] = cmd_t;
    }

    (*idx)++;
}

/**
 *
 * Builds an array of command_t in postfix given an array of command_t infix
 * for example c && d | e yields cde|&&
 *
 */

command_t* create_postfix(command_t* arr,int *length) {

/*
    0 AND_COMMAND,         // A && B
    1 SEQUENCE_COMMAND,    // A ; B
    2 OR_COMMAND,          // A || B
    3 PIPE_COMMAND,        // A | B
    4 SIMPLE_COMMAND,      // a simple command
    5 SUBSHELL_COMMAND,    // ( A )

    6 LEFT_PAREN, // (
    7 RIGHT_PAREN // )
*/

/*
  Precedence Rules

  1. | PIPE_COMMAND
  2. && or || AND_COMMAND or OR_COMMAND
  3. ; SEQUENCE_COMMAND

  SIMPLE_COMMAND IS NOT CONSIDERED AN OPERATOR


*/

 int precedence [4] = 
 {
   2 //AND_COMMAND
  ,1 //SEQUENCE_COMMAND
  ,2 //OR_COMMAND
  ,3 //PIPE_COMMAND
 };



  STACK* stack = (STACK *)checked_malloc(sizeof(STACK)); //stack of operators

  stack->top = NULL;

  command_t* postfix;
  postfix = NULL;
  int postfix_length = 0;

  

  int i;
  int len = *length;
  bool is_operator;
  command_t  item;

  for(i = 0; i < len; i++) {

    item = arr[i];

    is_operator = item->type == AND_COMMAND || item->type == SEQUENCE_COMMAND || item->type == OR_COMMAND || item->type == PIPE_COMMAND;

    if(item->type == SIMPLE_COMMAND) {
      
      //APPEND TO POSTFIX
      append_to_postfix(&postfix,item,&postfix_length);

    }

    else if(item->type == LEFT_PAREN) {
      push(stack,item);
    }

    //If its an operator and stack is empty push operator onto stack
    else if(is_operator && stack->top == NULL) {
      push(stack,item);
    }

    //If its an operator and stack is NOT EMPTY
    else if(is_operator && stack->top != NULL) {

      //Pop off all operators with greater or equal precedence off the stack and append them to the postfix 
      //Stop when you reach an operator with lower precedence or a (

      command_t  p_cmd_t;
      while( (stack->top && stack->top->cmd_t->type != LEFT_PAREN && precedence[stack->top->cmd_t->type] >= precedence[item->type] ) ) {
        //note that a RIGHT_PARENT will never be pushed on the stack
        pop(stack,&p_cmd_t);
        append_to_postfix(&postfix,p_cmd_t,&postfix_length);

      }


      //Push the new operator on the stack
      push(stack,item);
    }

    else if(item->type == RIGHT_PAREN) {
 
      //Pop operators off the stack and append them onto the postfix until you pop a matching (
      command_t p_cmd_t;
      while(pop(stack, &p_cmd_t) && p_cmd_t->type != LEFT_PAREN ) {

        append_to_postfix(&postfix,p_cmd_t,&postfix_length);

      }
      // free LEFT_PAREN and RIGHT_PAREN because they are pseudo commands
      free(item);
      free(p_cmd_t);

      //Then append a () to postfix which is used for building SUBSHELL COMMAND
      command_t subsh_cmd_t = create_command(SUBSHELL_COMMAND);

      append_to_postfix(&postfix,subsh_cmd_t,&postfix_length);
      (*length)--;
    }

  }//end for loop


  command_t remaining_command_t;
   //pop remaining commands
  while(pop(stack, &remaining_command_t)) {
     
      //Append remaining operators to postfix
      append_to_postfix(&postfix,remaining_command_t,&postfix_length);

  }

  //stack is now empty so free stack pointer
  free(stack);
  
  return postfix;

}

/**
 * generates a binary tree representing a complete command_t
 *
 *  @param command_t* postfix (array of command_t)
 *  @return command_t root of the tree
 *  Algorithm is equivalent to postfix evaluation algorithm
 *
 */

 command_t create_binary_tree(command_t* postfix,int length) {
 /*
    0 AND_COMMAND,         // A && B
    1 SEQUENCE_COMMAND,    // A ; B
    2 OR_COMMAND,          // A || B
    3 PIPE_COMMAND,        // A | B
    4 SIMPLE_COMMAND,      // a simple command
    5 SUBSHELL_COMMAND,    // ( A )

    6 LEFT_PAREN, // (
    7 RIGHT_PAREN // )
*/
  
    STACK* cmd_stack = (STACK*)checked_malloc(sizeof(STACK)); //stack of commands

    cmd_stack->top = NULL; //indicates end of linked list

    int i;

    command_t item
         , left_cmd
         , right_cmd;
         
    bool is_operator;

    for(i = 0; i < length; i++) {


      item = postfix[i];

      is_operator = item->type == AND_COMMAND || item->type == SEQUENCE_COMMAND || item->type == OR_COMMAND || item->type == PIPE_COMMAND;

      if(item->type == SIMPLE_COMMAND) { 

        push(cmd_stack,item);

      } else if(is_operator) {
        //pop 2 commands of stack
        //store as left and right as nodes of binary command item

        //THE ONLY CASE IN WHICH A right node is NULL is for a  terminating; (SEQUENCE COMMAND) 
        item->u.command[1] = (pop(cmd_stack,&right_cmd))? right_cmd : NULL;

        pop(cmd_stack,&left_cmd);
        item->u.command[0] = left_cmd;
        
        //push resulting item back onto stack
        push(cmd_stack,item);

      } else { //item type must be SUBSHELL COMMAND
              //store reference to whole command_t
        command_t subshell_command;

        pop(cmd_stack,&subshell_command);

        //item->subshell command is a pointer to a command so it must store an address
        item->u.subshell_command = subshell_command;

        //push result back onto stack
        push(cmd_stack,item);

      }

    }//end for loop

    //the final stack top is the root of the binary tree
    
    //top of command_stack is dynamically allocated; this the the vlaue that will be returned
    command_t cmd_t = cmd_stack->top->cmd_t;

     //stack is no longer needed free stack pointer
     free(cmd_stack->top);
     free(cmd_stack);   
     free(postfix);   

     return cmd_t;

 }

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *filestream)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */

  command_stream_t stream = (command_stream_t) checked_malloc(sizeof(struct command_stream)); 

  size_t byte_size = (size_t) sizeof(command_t) * 16;
	stream->cmds = (command_t*) checked_malloc(byte_size);
	stream->size = 0;

  int line_number = 1;
  command_t *infix_arr;
  command_t *postfix;
  int infix_len = 0;
  while ( (infix_arr = get_infix_commands(get_next_byte, filestream, &line_number, &infix_len)) )
  {
    if ((stream->size) * sizeof(command_t) == byte_size) 
      stream->cmds = checked_grow_alloc(stream->cmds, &byte_size);

    postfix = create_postfix(infix_arr, &infix_len);
    stream->cmds[stream->size] = create_binary_tree(postfix, infix_len);
		stream->size++;

    free(infix_arr);
  }

	if (stream->size * sizeof(command_t) < byte_size)
   	stream->cmds = (command_t*)checked_realloc(stream->cmds, stream->size * sizeof(command_t));

  return stream;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */

	if (s) {
		if (s->idx < s->size)
			return s->cmds[s->idx++];
	}
  return NULL;
}
