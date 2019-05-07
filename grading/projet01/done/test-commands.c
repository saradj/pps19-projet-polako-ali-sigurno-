#include "error.h"
#include "commands.h"
#include <stdio.h>

int main(int argc, char *argv[])
{

    /*  if (argc < 2)
    {
        fprintf(stderr, "please provide command filename to read from\n");
        return 1;
<<<<<<< HEAD
    } */

    program_t pgm;
    fprintf(stderr, "\nCalling program read");
    if (program_read(argv[1], &pgm) == ERR_NONE)
    {

        (void)program_print(stdout, &pgm);
    }
=======
    }

    program_t pgm;
    if (program_read(argv[1], &pgm) == ERR_NONE) {
        (void)program_print(stdout, &pgm);
    }
    (void)program_free(&pgm);
>>>>>>> 3dd57ed59575794473e08dca5ff2eddfb93e5b59

    return 0;
}
