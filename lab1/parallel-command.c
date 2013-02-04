#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //strcmp
#include <stdlib.h> //free

#include "command.h"


#include "command-internals.h"
#include "alloc.h" //memory allocation

#include "parallel-command.h"

file_t generate_file(char* file_name, bool is_output) {

  file_t new_file = (file_t)checked_malloc(sizeof(file));
  new_file->is_output = is_output;
  new_file->file_name = file_name;
  
  return new_file;
}

void append_file(file_t** folder, file_t file, int* idx) {
//pass a file_t* by reference
//file_t is a file*

    if (*folder) {
		//char** indicates an array of strings
		*folder = (file_t*)checked_realloc(*folder,(*idx + 1) *  sizeof(file_t));
    } else {
		*folder = (file_t*)checked_malloc(sizeof(file_t));
    }

	(*folder)[*idx] = file;
    (*idx)++;
}

// folder refers to the array of file structs that belong in a top-level command
file_t** append_folder(file_t*** file_system, int* idx) {
    
    if (*file_system) {

      *file_system = (file_t**)checked_realloc(*file_system,(*idx + 1) * sizeof(file_t*));

    } else { 
      //intialize first file buffer in file system
     
      *file_system = (file_t**)checked_malloc(sizeof(file_t*));

    }

    (*file_system)[*idx] = NULL;
    (*idx) = (*idx) + 1;

    //return pointer to the newly allocated folder
    return &( (*file_system)[(*idx)-1] );
	//return file_system[(*idx)-1];
}

void traverse_command(command_t c, file_t*** file_system, file_t** folder, int* file_count) {
//traverse a top level command and add its files to its folder

  if(c == NULL) {
    return;
  }

  switch(c->type) {

  case AND_COMMAND:
  case SEQUENCE_COMMAND:
  case OR_COMMAND: 
  case PIPE_COMMAND:
    traverse_command(c->u.command[0],file_system,folder,file_count);
    traverse_command(c->u.command[1],file_system,folder,file_count);
    break;
  case SUBSHELL_COMMAND:
    traverse_command(c->u.subshell_command, file_system, folder,file_count);
    break;
  case SIMPLE_COMMAND:
    //store input
    if (c->input) {
      append_file(folder, generate_file(c->input,false), file_count);
    }
    //store output
    if (c->output){
      append_file(folder, generate_file(c->output,true), file_count);
    }

    //assume all files in string are input
	char **w = c->u.word;
    while (*w) {//iterate over array of strings
      append_file(folder, generate_file(*w, false), file_count);
      w++;
    }

    break;

   default: 
    return;
  }

  return;
}

void get_command_files(command_t c, file_t*** file_system, int *idx) {
//pass a 2d file_t array by reference
//create a folder
//traverse a top level commands recursively to get files
//idx holds the size of the file_system array

  if(c == NULL) {
    return;
  }

  int file_count = 0;
  file_t** folder = append_folder(file_system,idx); //create a folder holding all files for current command

  traverse_command(c,file_system,folder,&file_count);
  // printf("%s",((*folder)[0])->file_name);
  //append null file indicating end of folder
  append_file(folder,NULL,&file_count);

}

void build_file_system(command_stream_t c_stream,file_t*** file_system,int* folder_count) {
    command_t cmd;
    while((cmd = read_command_stream(c_stream))) {
      get_command_files(cmd,file_system, folder_count);
    }

    /*int i;
    for(i = 0; i < *folder_count; i++) {
      printf("Folder %d:\n",i);
      int j;
      file_t* folder = (*file_system)[i];
      file_t f;
      
      for(j = 0; (f = folder[j]);j++){
        printf("\tFile %d: %s\n",j,f->file_name);
      }
    }*/
}

// free the dependency graph
void free_dep_graph_and_wait_queue(bool ***dep_graph, int size, int **wait_queue) {
  // free wait_queue
  free(*wait_queue);

  int i;
  for (i = 0; i < size; i++) {
    free((*dep_graph)[i]);
  }
  free(*dep_graph);
}

void clean_file_system(file_t*** file_system,int* folder_count){

    
    int i;
    int j;
    for(i = 0; i < *folder_count; i++) {
      //free all folders
      file_t f;
      for (j = 0; ( f = (*file_system)[i][j] ); j++)
        free(f);
      // NULL file at the end of each folder but it was malloced so need to free it
      free(f);
      free((*file_system)[i]);
    }

    //free file system
    free((*file_system));

}

// Create a dependency graph of size sizexsize where size is the number of top level commands
// dep_graph[row][col] means that the rowth command depends on the colth command
bool **create_dep_graph(file_t ***file_system, int *size, int **wait_queue) {
	// allocate memory for dependency count array
	*wait_queue = (int *)checked_malloc(sizeof(int) * (*size)); 

	// allocate memory for size x size 2D dependency array
	bool **dep_graph  = (bool **)checked_malloc(sizeof(bool *) * (*size));
	int i;
	for (i = 0; i < *size; i++) {
		dep_graph[i] = (bool *)checked_malloc(sizeof(bool) * (*size));
		int j;
		for (j = 0; j < *size; j++) 
			dep_graph[i][j] = false;
		// init to 0 dependency counts for each command
		(*wait_queue)[i] = 0;
	}


	// create dependency graph by 
	int cmd_row;
	for (cmd_row = 0; cmd_row < *size; cmd_row++) {
		// current folder/file
		file_t *folder = (*file_system)[cmd_row];
		file_t f;
		int file_idx;
		for (file_idx = 0; (f = folder[file_idx]); file_idx++) {

			int prev_cmd_row;
			for (prev_cmd_row = 0; prev_cmd_row < cmd_row; prev_cmd_row++) {
				// compare current folder/file to folder/files from previous commands
				file_t *prev_folder = (*file_system)[prev_cmd_row];
				file_t prev_f;
				int prev_file_idx;
				for (prev_file_idx = 0; (prev_f = prev_folder[prev_file_idx]); prev_file_idx++) {
					// current command depends on prev command
					//printf("Cur: %s is_output:%d Prev: %s is_output:%d\n", f->file_name, f->is_output, prev_f->file_name, prev_f->is_output);
					if (strcmp(f->file_name, prev_f->file_name) == 0 && (f->is_output || prev_f->is_output)) {
						dep_graph[cmd_row][prev_cmd_row] = true;
						(*wait_queue)[cmd_row]++;
					}
				}
			}
		}
	}

	/*// print dependency graph and dependency counts
	
	int x;
	for (x = 0; x < *size; x++) {
		int y;
		printf("Command %d: ", x);
		for (y = 0; y < *size; y++) {
			if (dep_graph[x][y])
				printf("cmd%d, ", y);
		}
		printf("\n");
	}
	printf("\n");

	// matrix
	printf("cmd");
	for (x = 0; x < *size; x++) 
		printf(" %d ", x);
	printf("\n   ");
	for (x = 0; x < *size; x++)
		printf(" - ");

	printf("\n");
	for (x = 0; x < *size; x++) {
		int y;
		printf(" %d:", x);
		for (y = 0; y < *size; y++) {
			//if (dep_graph[x][y])
				printf(" %d ", dep_graph[x][y]);
			//else
				//printf(" 0 ");
		}
		printf("\n");
	}
	printf("\n");
	
	for (x = 0; x < *size; x++) {
		printf("Command %d depends on %d commands.\n", x, (*wait_queue)[x]);
	}
  */
	return dep_graph;
}
