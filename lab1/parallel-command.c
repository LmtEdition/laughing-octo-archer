#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "command.h"

#include "command-internals.h"
#include "alloc.h" //memory allocation

#include "parallel-command.h"

file_t generate_file(char* file_name,bool is_output) {

  file_t new_file = (file_t)checked_malloc(sizeof(file));
  new_file->is_output = is_output;
  new_file->file_name = file_name;
  
  return new_file;
}

void append_file(file_t** file_arr,file_t file,int* idx) {
//pass a file_t* by reference
//file_t is a file*

    if(*file_arr) {

    //char** indicates an array of strings
    *file_arr = (file_t*)checked_realloc(*file_arr,(*idx + 1) *  sizeof(file_t));

    (*file_arr)[*idx] = file;


    } else {
      *file_arr = (file_t*)checked_malloc(sizeof(file_t));
      (*file_arr)[*idx] = file;
    }

    (*idx)++;
}

file_t** append_folder(file_t*** file_system,int* idx) {
    
    if(*file_system) {

      *file_system = (file_t**)checked_realloc(*file_system,(*idx + 1) * sizeof(file_t*));

      (*file_system)[*idx] = NULL;


    } else { 
      //intialize first file buffer in file system
     
      *file_system = (file_t**)checked_malloc(sizeof(file_t*));
      (*file_system)[*idx] = NULL;

    }

    (*idx) = (*idx) + 1;

    //return pointer to the newly allocated folder
    return file_system[(*idx) -1];


}

void traverse_command(command_t c,file_t*** file_system,file_t** folder,int* folder_count) {
//traverse a top level command and add its files to its folder

  if(c == NULL) {
    return;
  }

  switch(c->type) {

  case AND_COMMAND:
  case SEQUENCE_COMMAND:
  case OR_COMMAND: 
  case PIPE_COMMAND:
    traverse_command(c->u.command[0],file_system,folder,folder_count);
    traverse_command(c->u.command[1],file_system,folder,folder_count);
    break;
  case SUBSHELL_COMMAND:
    traverse_command(c->u.subshell_command,file_system,folder,folder_count);
    break;
  case SIMPLE_COMMAND:
    //store input
    if(c->input) {

      append_file(folder,generate_file(c->input,false),folder_count);



    }
    //store output
    if(c->output){

      append_file(folder,generate_file(c->input,true),folder_count);

    }

    //assume all files in string are input
    while(c->u.word[0] != NULL) {//iterate over array of strings

      append_file(folder,generate_file(c->u.word[0],false),folder_count);

      (c->u.word)++;

    }

    break;

   default: 
    return;
  }

  return;
}



void get_command_files(command_t c,file_t*** file_system,int *length) {
//pass a 2d file_t array by reference
//create a folder
//traverse a top level commands recursively to get files

  if(c == NULL) {
    return;
  }

  int file_count = 0;
  file_t** folder = append_folder(file_system,length); //create a folder holding all files for current command

  traverse_command(c,file_system,folder,&file_count);
}
