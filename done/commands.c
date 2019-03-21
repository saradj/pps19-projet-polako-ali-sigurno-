/**
 * @file commands.c
 * @brief 
 *
 * @author Sara Djambazovska and Marouane Jaakik
 * @date March 2019
 */
#include "commands.h" // for virt_addr_t
#include <stdio.h> // for size_t, FILE
#include <stdint.h> // for uint32_t
#include <string.h>

int program_init(program_t* program){
 program->nb_lines = 0;
program->allocated = sizeof(program->listing) ;
//size_t i;
//for(i=0; i<100;++i)
//program->listing [i]=0;
(void)memset(&(program->listing), 0, sizeof(program->listing));//???
return 0; //???
}
