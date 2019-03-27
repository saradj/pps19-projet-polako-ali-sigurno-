/**
 * @file commands.c
 * @brief 
 *
 * @author Sara Djambazovska and Marouane Jaakik
 * @date March 2019
 */
#include "commands.h" // 
#include <stdio.h> // for size_t, FILE
#include <stdint.h> // for uint32_t
#include <string.h>
#include "error.h"

int program_init(program_t* program){
 program->nb_lines = 0;
program->allocated = sizeof(program->listing) ;
for_all_lines(line, program){line=0; }
size_t i;
for(i=0; i<100;++i)
program->listing [i]=0;
(void)memset(&(program->listing), 0, sizeof(program->listing));//???
return ERR_NONE; //???
}

int program_print(FILE* output, const program_t* program){
	
}

int program_shrink(program_t* program){
// validity  of argument??
M_REQUIRE_NON_NULL(program);
return 0;
}

int program_add_command(program_t* program, const command_t* command){

M_EXIT_IF((command->taille == DATA) && ((command->data_size != 1) || (command->data_size!= sizeof(word_t))), ERR_SIZE, "data can not have length different than 1 byte or word");
M_EXIT_IF((command->taille == INSTRUCTION) && (command->data_size!= sizeof(word_t)), ERR_SIZE, "Instructions must have length of a word");// should we use err size or err bad parameter??
M_EXIT_IF((command-> order == WRITE), ERR_BAD_PARAMETER, "cannot write only read commands");
// command can have ony one line, when we add command we increase nb of lines? we check if nb lines=100?
M_REQUIRE((program->nb_lines) < 100, ERR_MEM, "programm already contains 100 commands");
program->listing [program->nb_lines] = *command;
++(program->nb_lines);
return ERR_NONE;
} 

int program_read(const char* filename, program_t* program){
		
}
int read_command(const char* filename, program_t* program, size_t i){// read command i and put it in program listing
	
	FILE *fp;
	fp = fopen(filename, "r"); // read mode
	 if (fp == NULL)
   {
      perror("Error while opening the file.\n");
      exit(EXIT_FAILURE);
   }
   currChar = fgetc (fp);
    if (((!isspace(currChar) && 
                              currChar != EOF)) {
	if(currChar=='R')
	n_order = READ;
	else if(currChar=='W')
	n_order = WRITE;
	else
	M_EXIT(ERR_BAD_PARAMETER. "First caracther shoud be R or W")
	M_REQUIRE(isspace(fgetc(fp),ERR_BAD_PARAMETER, "sHOULD BE SPACE ")
							  
							  }
   
	command_t new_command ={.order=( (fgetc(fp) == R) ? READ, WRITE), .taille=  }
	
	program_add_command(&program, 
}


char* read_next(const char* filename){
	if()
}

