#ifndef PARALLEL_H
#define PARALLEL_H

#include <stdbool.h>

typedef struct {

  bool is_output;
  char* file_name;

} file;

typedef file* file_t;

file_t generate_file(char* file_name,bool is_output);

void append_file(file_t** file_arr,file_t file,int* idx);
file_t** append_folder(file_t*** file_system,int* idx);


void traverse_command(command_t c,file_t*** file_system,file_t** folder,int* folder_count);

void get_command_files(command_t c,file_t*** file_system,int *length);

void build_file_system(command_stream_t c_stream,file_t*** file_system,int* folder_count);

void free_dep_graph_and_wait_queue(bool ***dep_graph, int size, int **wait_queue);

void clean_file_system(file_t*** file_system,int* folder_count);


bool **create_dep_graph(file_t ***file_system, int *size, int **cmd_dep_counts);

#endif
