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
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include "error.h"
#include "addr_mng.h"
#include "addr.h"

int k = VIRT_ADDR_RES ;

int fill_command(FILE* fp, command_t* command);

int program_init(program_t* program){
	program->listing=(command_t*)calloc(10, sizeof(command_t));
	M_REQUIRE_NON_NULL((program->listing));
program->nb_lines = 0;
program->allocated = 10;
return ERR_NONE; //???
} 

int program_print(FILE* output, const program_t* program){
	
	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(output);
	for_all_lines(line, program){
	 fprintf (output,(line->order==READ) ? "R ": "W ");
	 fprintf (output,(line->taille==INSTRUCTION) ? "I ": (line->data_size==1) ? "DB ": "DW ");
	if( line->order==WRITE){
	//fprintf (output, line->data_size == 1 ? ("0x%02" PRIX32, line->write_data) : ("0x%08" PRIX32, line->write_data ));
	if(line->data_size == 1)
	fprintf (output, "0x%02" PRIX32, line->write_data );
	else
	fprintf (output, "0x%04" PRIX32, line->write_data );}
	fprintf(output, " @");
	uint64_t v= virt_addr_t_to_virtual_page_number(&(line->vaddr))<<12 |(line->vaddr).page_offset;
	fprintf (output, "0x%016" PRIX64, virt_addr_t_to_virtual_page_number(&(line->vaddr))<<12  );
	fprintf (output, "0x%016" PRIX64, v );
	//print_virtual_address(output, &(line->vaddr));
	fprintf(output,"\n");
	
}
return ERR_NONE;	
}

int program_shrink(program_t* program){
M_REQUIRE_NON_NULL(program);
M_REQUIRE_NON_NULL(program->listing);
program->allocated = program->nb_lines == 0 ? 10 : program->nb_lines;
program->listing= (command_t*)realloc(program->listing, program->allocated * sizeof(command_t));

return ERR_NONE;
}

int program_add_command(program_t* program, const command_t* command){
	//fprintf(stderr,"err in prog \n");
M_REQUIRE_NON_NULL(program);
M_REQUIRE_NON_NULL(command);
M_REQUIRE_NON_NULL(program->listing);
M_EXIT_IF((command->taille == DATA) && ((command->data_size != 1) && (command->data_size!= sizeof(word_t))), ERR_SIZE, "data can not have length different than 1 byte or word");

M_EXIT_IF((command->taille == INSTRUCTION) && (command->data_size!= sizeof(word_t)), ERR_SIZE, "Instructions must have length of a word");//should we use err size or err bad parameter??

M_EXIT_IF((command->taille == INSTRUCTION) &&(command->order != READ), ERR_BAD_PARAMETER, "cannot write only read commands");

M_EXIT_IF((program->allocated) > 100,ERR_SIZE, "Can only allocate 100 programs");

(program->nb_lines);

if(program->nb_lines>=program->allocated){
	
	program->allocated =program->allocated * 2;
program->listing= (command_t*)realloc(program->listing, program->allocated * sizeof(command_t));
fprintf(stderr, "here");
memset(program->listing + 10, 0, 10*sizeof(command_t));
fprintf(stderr, "after");

}
M_EXIT_IF((program->allocated) > 100,ERR_SIZE, "Can only allocate 100 programs");
M_REQUIRE((program->nb_lines) < 100, ERR_MEM, "programm already contains 100 commands");
program->listing [program->nb_lines] = *command;
++(program->nb_lines);
return ERR_NONE;

} 

int program_read(const char* filename, program_t* program){
	program_init(program);
	
	FILE *fp;
	fp = fopen(filename, "r"); // read mode
	
	M_REQUIRE_NON_NULL(fp);
		
	   command_t command;// should we initiaise it??
	   int k;
		while((k = fill_command(fp,&command))!=EOF){
		if((k!=ERR_NONE)){
			
			fprintf(stderr,"err in fill \n");
			return k;}
		else{ 
			k=program_add_command(program,&command);
			if(k!=ERR_NONE){
	fprintf(stderr,"err in prog \n");
	  return k;
  }}
 }
	fclose(fp);
	return ERR_NONE; 
}



int fill_command(FILE* fp, command_t* command){

char c;

while (isspace(c=fgetc(fp))){};
if(c==EOF)
	return EOF;
	
fprintf(stderr, "charr == %c", c);

if(c=='R'){
		command->order= READ;}
		else if(c=='W'){
		
		command->order = WRITE;}
		else{
		M_EXIT(ERR_BAD_PARAMETER, "The command must start with R or W");}
		
		M_REQUIRE(isspace(fgetc(fp)), ERR_BAD_PARAMETER, "W or R must be followed by space");
		

 switch(fgetc(fp)){
	case 'I': {
				command->taille=INSTRUCTION;
				command->data_size=sizeof(word_t);
				M_REQUIRE(isspace(fgetc(fp)), ERR_BAD_PARAMETER, "I must be followed by space");
				break;
				}
	case 'D':{ 
		command->taille=DATA;
		c=fgetc(fp);
		M_REQUIRE((c=='W')||(c =='B'), ERR_BAD_PARAMETER, "Must specify data size word or byte");
		command->data_size= (c=='W') ? sizeof(word_t) : 1;
		if(command->order == WRITE){
			uint64_t writeData;
			fscanf(fp,"%lx" SCNx64 ,&writeData);
			command->write_data = writeData;
		}
		
		M_REQUIRE(isspace(fgetc(fp)), ERR_BAD_PARAMETER, "WRITEDATA must be followed by space");
	}	
  }


	while (isspace(c=fgetc(fp))){};
	M_REQUIRE(c=='@', ERR_ADDR, "virtadd  must start with @");
	uint64_t vaddr;
	fscanf(fp,"%lx" SCNx64 ,&vaddr);
	init_virt_addr64(&(command->vaddr),vaddr);	
	fprintf(stderr, " virt = %lx \n", vaddr);
	
return ERR_NONE;
}



int program_free(program_t* program){
	
	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(program->listing);
	free(program->listing);
	program->listing = NULL;
	program->nb_lines=0;
	program->allocated=0;
	program = NULL;
	return ERR_NONE;
}
