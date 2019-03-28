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
//for_all_lines(line, program){line=0; }
size_t i;
for(i=0; i<100;++i)
program->listing [i]=0;
(void)memset(&(program->listing), 0, sizeof(program->listing));//???
return ERR_NONE; //???
}

int program_print(FILE* output, const program_t* program){
	
	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(output);
	for_all_lines(line, program){
	 fprintf (output,(line.order==READ) ? "R ": "W ");
	 fprintf (output,(line.taille==INSTRUCTION) ? "I ": (line.data_size==1) ? "DB ": "DW ");
	if( line.order==WRITE)
		fprintf (output, line.data_size==1? : "0x%02" PRIX32, line.write_data : "0x%08" PRIX32, line.write_data );
	fprintf(output, " @");
	print_virtual_address(&output, &(line.vaddr));
	fprintf(output,"\n");
}
return ERR_NONE;	
}

int program_shrink(program_t* program){
// validity  of argument??
M_REQUIRE_NON_NULL(program);
return 0;
}

int program_add_command(program_t* program, const command_t* command){

M_EXIT_IF((command->taille == DATA) && ((command->data_size != 1) || (command->data_size!= sizeof(word_t))), ERR_SIZE, "data can not have length different than 1 byte or word");
M_EXIT_IF((command->taille == INSTRUCTION) && (command->data_size!= sizeof(word_t)), ERR_SIZE, "Instructions must have length of a word");//should we use err size or err bad parameter??
M_EXIT_IF((command-> order == WRITE), ERR_BAD_PARAMETER, "cannot write only read commands");
//command can have ony one line, when we add command we increase nb of lines? we check if nb lines=100?
M_REQUIRE((program->nb_lines) < 100, ERR_MEM, "programm already contains 100 commands");
program->listing [program->nb_lines] = *command;
++(program->nb_lines);
return ERR_NONE;

} 

int program_read(const char* filename, program_t* program){
	program_init(&program);
	FILE *fp;
	fp = fopen(filename, "r"); // read mode
	M_REQUIRE_NON_NULL(fp);
	//char[35] str;	
	// while (fgets(str, 35, fp) != NULL){
		
//	 if(fill_command(str,&command)!= ERR_NONE)
	// return ERR_BAD_PARAMETER;
	 //if(program_add_command(&program,&command)!= ERR_NONE)
	  //return ERR_BAD_PARAMETER; 
	   command_t command;
		while((int k=fill_command(&fp,&command))!=EOF)
		if((k!=ERR_NONE)||(program_add_command(&program,&command)!= ERR_NONE))
	
	  return ERR_BAD_PARAMETER;
	  
	 }
	fclose(fp);
	return ERR_NONE; 
}


int fill_command(FILE* fp, command_t* command){

size_t state=0;
size_t i=0;
switch(state){
case 0: char c=fgetc(fp);
if(c=='R')
		command->order= READ;
		else if(c=='W')
		command->order = WRITE;
		else
		M_EXIT(ERR_BAD_PARAMETER, "The command must start with R or W");
		M_REQUIRE(isspace(fgetc(fp), ERR_BAD_PARAMETER, "W or R must be followed by space");
		++state;
case 1: switch(fgetc(fp){
	case 'I': command->taille=INSTRUCTION;
			command->data_size=sizeof(word_t);
			state=3;
			M_REQUIRE(isspace(fgetc(fp), ERR_BAD_PARAMETER, "I must be followed by space");
			
			break;
	case 'D': command->taille=DATA;
	char c=fgetc(fp)
			M_REQUIRE((c=='W')||(c =='B'), ERR_BAD_PARAMETER, "Must specify data size word or byte");
			command->data_size= (c=='W') ? sizeof(word_t) : 1;
			state =(command->order==READ) ? 3 : 2; 
			M_REQUIRE(isspace(fgetc(fp), ERR_BAD_PARAMETER, "DW or DB must be followed by space");
			break;
	default : EXIT_FAILURE;
case 2: // checking if size of writedata is correct is done in add comand
//strcpy(string, string+i+1);
	if(command->data_size==1)
	const size_t size_of_wrdata = command->data_size==1 ? 4:10;
	char [size_of_wrdata] str;
	for(int i=0;i <size_of_wrdata; ++i)
	str[i]=fgetc(fp);
	command->write_data = (int) strtol(str, NULL,0); // string to long
	M_REQUIRE(isspace(fgetc(fp), ERR_BAD_PARAMETER, "wrdata  must be followed by space");
	state++;
case 3:// virt addr
	M_REQUIRE(fgetc(fp)=='@', ERR_ADDR, "virtadd  must start with @");
	char [18] vrt;
	for(int i=0;i <18; ++i)
	vrt[i]=fgetc(fp);
	init_virt_addr64(&(command->vaddr),(int) strtol(vrt, NULL, 0));	
}
M_EXIT_IF_TRAILING(fp);
return ERR_NONE;

}
}

uint32_t
int require_valid(program_t* program){
	for_all_lines(line, program){
	M_REQUIRE( ((line.data_size ==( (line.taille == DATA) ? ( 1 || sizeof(word_t)) : sizeof(word_t))), ERR_SIZE, "data can not have length different than 1 byte or word");
M_REQUIRE((line.taille == INSTRUCTION) && (line.data_size== sizeof(word_t)), ERR_SIZE, "Instructions must have length of a word");//should we use err size or err bad parameter??
M_EXIT_IF((line.order == WRITE), ERR_BAD_PARAMETER, "cannot write only read commands");
if(line.taille==DATA)
if(line.data_size==1)
M_REQUIRE( (write_data & (pow(2,9)-1)== write_data), ERR_SIZE, "wrdata should be ony 1 byte if data size =1");
else
M_REQUIRE( (write_data & (pow(2,25)-1)== write_data), ERR_SIZE, "wrdata should be ony 4 bytes if data size =1");
else if(line.taille==INSTRUCTION)
M_REQUIRE((write_data & (pow(2,25)-1)== write_data), ERR_SIZE, "wrdata should be ony 4 bytes if data size =1");

}
}
