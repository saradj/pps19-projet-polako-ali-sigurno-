/**
 * @file commands.c
 * @brief 
 *
 * @author Sara Djambazovska and Marouane Jaakik
 * @date March 2019
 */
#include "commands.h" //
#include <stdio.h>	// for size_t, FILE
#include <stdint.h>   // for uint32_t
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include "error.h"
#include "addr_mng.h"
#include "addr.h"

#define INIT_NBLINES 10
#define BITS_IN_BYTE 8
#define MASK_WORD ((uint32_t)-1)
int fill_command(FILE *fp, command_t *command);

int program_init(program_t *program)
{ //initialising the program

	M_REQUIRE_NON_NULL(program);
	program->listing = (command_t *)calloc(INIT_NBLINES, sizeof(command_t)); //using callc to allocate and initialise to 0 an array of 10 commands
	if (program->listing == NULL)
	{
		program->allocated = 0;
		return ERR_MEM;
	}
	program->nb_lines = 0;
	program->allocated = INIT_NBLINES * sizeof(command_t);
	return ERR_NONE;
}

int program_print(FILE *output, const program_t *program)
{ // printing the program

	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(program->listing);
	M_REQUIRE_NON_NULL(output);
	for_all_lines(line, program)
	{														  // looping trough the lines
		fprintf(output, (line->order == READ) ? "R " : "W "); //check for a read
		fprintf(output, (line->type == INSTRUCTION) ? "I " : (line->data_size == 1) ? "DB " : "DW ");
		if (line->order == WRITE)
		{ // checking if we need write data
			if (line->data_size == 1)
				fprintf(output, "0x%02" PRIX32, line->write_data);
			else
				fprintf(output, "0x%08" PRIX32, line->write_data);
		}
		fprintf(output, " @");
		uint64_t vaddr_num = virt_addr_t_to_virtual_page_number(&(line->vaddr)) << PAGE_OFFSET | (line->vaddr).page_offset;
		fprintf(output, "0x%016" PRIX64, vaddr_num); // printing the virtual address
		fprintf(output, "\n");
	}
	return ERR_NONE;
}

int program_shrink(program_t *program)
{ // fitting the size of a program

	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(program->listing);
	size_t old_allocated = program->allocated;										// saving the old allocated value
	program->allocated = program->nb_lines == 0 ? INIT_NBLINES : program->nb_lines; // allocating only a needed number of lines
	command_t *listing = NULL;
	if ((listing = (command_t *)realloc(program->listing, program->allocated * sizeof(command_t)), program->allocated * sizeof(command_t)) == NULL)
	{
		program->allocated = old_allocated;
		return ERR_MEM; // IF REALLOC FAILS
	}
	program->listing = listing;
	return ERR_NONE;
}

int program_add_command(program_t *program, const command_t *command)
{
	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(command);
	M_REQUIRE_NON_NULL(program->listing);

	M_EXIT_IF((command->type == DATA) && ((command->data_size != 1) && (command->data_size != sizeof(word_t))), ERR_SIZE, "data can not have length different than 1 byte or word, length is %u", command->data_size);
	M_EXIT_IF((command->type == INSTRUCTION) && (command->data_size != sizeof(word_t)), ERR_SIZE, "Instructions must have length of a word, but size is %z", command->data_size); //should we use err size or err bad parameter??
	M_EXIT_IF((command->type == INSTRUCTION) && (command->order != READ), ERR_BAD_PARAMETER, "cannot write only %s commands", "read");
	M_EXIT_IF((command->order == WRITE) && (command->type == DATA) && ((command->data_size == 1) && (command->write_data >> BITS_IN_BYTE != 0)), ERR_BAD_PARAMETER, "wite data is not good size %d", command->write_data);
	//M_EXIT_IF((command->order == READ) && (command->write_data != 0), ERR_BAD_PARAMETER, "wite data is not good size %d", command->write_data);// gives an error !

	size_t old_allocated = program->allocated; // saving the old allocated value to be able to initialise the new allocated memory part
	if (program->nb_lines * sizeof(command_t) >= program->allocated)
	{
		program->allocated = program->allocated * 2; // doubling  the allocated size
		command_t *listing = NULL;
		M_EXIT_IF_NULL(listing = (command_t *)realloc(program->listing, program->allocated), program->allocated);
		program->listing = listing;
		memset(program->listing + (old_allocated / sizeof(command_t)), 0, old_allocated); // initialiing the uninitialised parts of memory
	}
	M_REQUIRE((program->nb_lines) < 100, ERR_MEM, "programm already contains %u commands", program->nb_lines);
	program->listing[program->nb_lines] = *command; //adding the command
	++(program->nb_lines);
	return ERR_NONE;
}

int program_read(const char *filename, program_t *program)
{

	program_init(program);
	FILE *fp;
	fp = fopen(filename, "r"); // read mode
	M_REQUIRE_NON_NULL(fp);
	command_t command;
	int k;
	while ((k = fill_command(fp, &command)) != EOF)
	{

		if ((k != ERR_NONE))
		{
			fclose(fp);
			return k;
		}

		if ((k = program_add_command(program, &command)) != ERR_NONE)
		{
			fclose(fp);
			return k;
		}
	}
	program_shrink(program);
	fclose(fp);
	return ERR_NONE;
}

int fill_command(FILE *fp, command_t *command)
{
	int n = 0;
	char c;
	while (isspace(c = fgetc(fp)))
	{
	}; // skipping the spaces in the begining or end of last line
	if (c == EOF)
		return EOF; // if we have reached End of File
	if (c == 'R')
		command->order = READ;
	else if (c == 'W')
	{
		command->order = WRITE;
	}
	else
	{
		M_EXIT(ERR_BAD_PARAMETER, "The command must start with R or W, starts with %c", c);
	}

	M_REQUIRE(isspace(c = fgetc(fp)), ERR_BAD_PARAMETER, "W or R must be followed by space but is followed by %c", c);

	while (isspace(c = fgetc(fp)))
	{
	}; // might have more spaces in between
	switch (c)
	{
	case 'I':
	{
		command->type = INSTRUCTION;
		command->data_size = sizeof(word_t);
		M_REQUIRE(isspace(c = fgetc(fp)), ERR_BAD_PARAMETER, "I must be followed by space, but is followed by %c", c);
		break;
	}
	case 'D':
	{
		command->type = DATA;
		c = fgetc(fp);
		M_REQUIRE((c == 'W') || (c == 'B'), ERR_BAD_PARAMETER, "Must specify data size word or byte character read is %c", c);
		command->data_size = (c == 'W') ? sizeof(word_t) : 1;
		if (command->order == WRITE)
		{
			uint32_t writeData;
			M_REQUIRE(n = fscanf(fp, "%x" SCNx32, &writeData) == 1, ERR_BAD_PARAMETER, "number of 32 bits read is %d", n); // getting the write data
			command->write_data = writeData;
		}
		M_REQUIRE(isspace(c = fgetc(fp)), ERR_BAD_PARAMETER, "WRITEDATA must be followed by space but is followed by %c", c);
		break;
	}
	default:
	{
		M_EXIT_ERR(ERR_BAD_PARAMETER, "Must specify a type instruction I or data D, but character is %c", c);
	}
	}
	while (isspace(c = fgetc(fp)))
	{
	}; // might have more spaces in between
	M_REQUIRE(c == '@', ERR_ADDR, "virtadd  must start with @, first character is %c", c);
	uint64_t vaddr = 0;

	M_REQUIRE(n = fscanf(fp, "%lx" SCNx64, &vaddr) == 1, ERR_BAD_PARAMETER, "number of 64 bits read is %d", n);

	return init_virt_addr64(&(command->vaddr), vaddr); // initialising the virtual address of the command
}

int program_free(program_t *program)
{ // free the memory allocated by the program
	M_REQUIRE_NON_NULL(program);
	free(program->listing);
	program->listing = NULL;
	program->nb_lines = 0;
	program->allocated = 0;
	program = NULL;
	return ERR_NONE;
}
