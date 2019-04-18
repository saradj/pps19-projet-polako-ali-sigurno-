#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "commands.h"
#include "addr_mng.h"
#include "error.h"

//BEST PIECE OF CODE I'VE EVER WRITTEN SO FAR
//TODO: Won't compile, "Static declaration follows non static declaration"
static const size_t MAX_CHAR_NUMBER = 2;
static size_t handle_line_calls = 0;

static void printline(const command_t *command, FILE *stream)
{
	if (command->order == READ)
		fprintf(stream, "R ");
	else
		fprintf(stream, "W ");
	if (command->type == INSTRUCTION)
		fprintf(stream, "I ");

	else if (command->data_size == 4)
		fprintf(stream, "DW ");
	else
		fprintf(stream, "DB ");
	if (command->data_size == 1 && command->order == WRITE)
		fprintf(stream, "0x%02" PRIX32 " ", command->write_data);
	else if (command->order == WRITE)
		fprintf(stream, "0x%08" PRIX32 " ", command->write_data);
	fprintf(stream, "@0x%016" PRIX64 "\n", virt_addr_t_to_uint64_t(&(command->vaddr)));
}

static void handle_line_instruction(command_t *line)
{
	//In case of instruction read fields data size and write data are chosen to be 0
	line->data_size = 4;
	line->write_data = 0;
	line->type = INSTRUCTION;
}

static void handle_line_data_read(FILE *input, command_t *line, char size)
{

	size == 'W' ? (line->data_size = 4) : (line->data_size = 1);
	line->write_data = 0;
	line->type = DATA;
}

static void handle_line_data_write(FILE *input, command_t *line, char size)
{
	size == 'B' ? fscanf(input, " %" SCNx8, &(line->write_data)) : fscanf(input, " %" SCNx32, &(line->write_data));
	size == 'B' ? (line->data_size = 1) : (line->data_size = 4);
	line->type = DATA;
}

static command_t handle_line(FILE *input)
{
	debug_print("", NULL);
	command_t line;
	char accessType;
	char word_byte[MAX_CHAR_NUMBER];
	fscanf(input, " %c %2s", &accessType, word_byte);
	line.order = accessType == 'R' ? READ : WRITE;
	uint64_t vaddr = 0;
	if (line.order == READ)
	{
		word_byte[0] == 'I' ? handle_line_instruction(&line) : handle_line_data_read(input, &line, word_byte[1]);
	}
	else
	{
		handle_line_data_write(input, &line, word_byte[1]);
	}
	debug_print("Input stream at end: %d", feof(input));
	fscanf(input, " %*c%" SCNx64, &vaddr);
	debug_print("virt addr: 0x%08" PRIX32, vaddr);
	debug_print("Input stream at end: %d", feof(input));
	init_virt_addr64(&line.vaddr, vaddr);
	return line;
}

int program_read(const char *filename, program_t *program)
{
	M_REQUIRE_NON_NULL(filename);
	M_REQUIRE_NON_NULL(program);
	FILE *input;
	;
	debug_print("# lines at beginning of program_read: %d", program->nb_lines);
	fprintf(stderr, "\nHELp");
	program_init(program);
	debug_print("# lines at beginning of program_read: %d", program->nb_lines);
	input = fopen(filename, "r");
	M_REQUIRE_NON_NULL(input);

	debug_print("Right before while:", NULL);
	do
	{
		debug_print("Input stream at start: %d", feof(input));

		debug_print("Handle line calls %u", ++handle_line_calls, NULL);
		command_t lineCo = handle_line(input);
		size_t vaddr = virt_addr_t_to_uint64_t(&lineCo.vaddr);
		debug_print("Input stream at end: %d", feof(input));

		program_add_command(program, &lineCo);
	} while (!feof(input) && !ferror(input));

	debug_print("We entered %d times in the loop", handle_line_calls);
	fclose(input);
	program->nb_lines--;
	program_shrink(program);
	return ERR_NONE;
}
int program_init(program_t *program)
{
	debug_print("Entered program init", NULL);
	debug_print("Program freed", NULL);
	if ((program->listing = calloc(START_SIZE, sizeof(command_t))) != NULL)
	{
		program->allocated = START_SIZE * sizeof(command_t);
		program->nb_lines = 0;
		return ERR_NONE;
	}
	return ERR_MEM;
}

int program_print(FILE *output, const program_t *program)
{
	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(output);
	for_all_lines(line, program)
	{
		printline(line, output);
	}
	return ERR_NONE;
}

int program_shrink(program_t *program)
{
	M_REQUIRE_NON_NULL(program);
	if (program->listing != NULL && program->nb_lines > 0 && (program->listing = realloc(program->listing, sizeof(command_t) * program->nb_lines)) != NULL)
	{
		program->allocated = program->nb_lines * sizeof(command_t);
	}
	else
	{
		program_init(program);
	}
	return ERR_NONE;
}

int program_add_command(program_t *program, const command_t *command)
{
	debug_print("In program add command", NULL);
	M_REQUIRE_NON_NULL(program);
	M_REQUIRE_NON_NULL(command);
	bool wrongDataSize = (command->data_size != 1 && command->data_size != sizeof(word_t)) && command->type == DATA;
	bool wrongInstructionSize = command->data_size != sizeof(word_t) && command->type == INSTRUCTION;
	bool writtingInstruction = command->type == INSTRUCTION && command->order == WRITE;
	bool invalidAddr = ((command->vaddr).page_offset % (uint16_t)command->data_size) != 0;
	debug_print("About to test validity", NULL);

	if (wrongDataSize || wrongInstructionSize || writtingInstruction || invalidAddr)
	{
		debug_print("Some conditions are not satisfied", NULL);
		return ERR_BAD_PARAMETER;
	}
	debug_print("Passed error checks in program_add_command", NULL);
	if (program->nb_lines * sizeof(command_t) == program->allocated)
	{
		debug_print("program->nb_lines == program->allocated is true", NULL);
		program->allocated *= 2;
		if ((program->listing = realloc(program->listing, sizeof(command_t) * program->allocated)) == NULL)
		{
			return ERR_MEM;
		}
	}
	debug_print("Leaving program add command", NULL);
	debug_print("Number of lines = %d", program->nb_lines);
	program->listing[(program->nb_lines)++] = *command;
	debug_print("Assignment done", NULL);

	return ERR_NONE;
}

int program_free(program_t *program)
{
	debug_print("Entered free", NULL);

	if (program != NULL)
	{
		debug_print("Program not null", NULL);
		if (program->listing != NULL)
		{
			debug_print("Size of listing %ld", sizeof(program->listing));
			free(program->listing);
			debug_print("Successfully freed", NULL);
			program->listing = NULL;
		}
		program->nb_lines = 0;
		program->allocated = 0;
		return ERR_NONE;
	}
	debug_print("about to free listing", NULL);
	return ERR_MEM;
}
