/**
 * @memory.c
 * @brief memory management functions (dump, init from file, etc.)
 *
 * @author Jean-Cédric Chappelier
 * @date 2018-19
 */

#if defined _WIN32 || defined _WIN64
#define __USE_MINGW_ANSI_STDIO 1
#endif

#include "memory.h"
#include "page_walk.h"
#include "addr_mng.h"
#include "util.h" // for SIZE_T_FMT
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>   // for memset()
#include <inttypes.h> // for SCNx macros
#include <assert.h>

#define BYTE_SIZE 1
#define FOURKI 4096
#define MAXSIZE_STRING 100

// ======================================================================
/**
 * @brief Tool function to print an address.
 *
 * @param show_addr the format how to display addresses; see addr_fmt_t type in memory.h
 * @param reference the reference address; i.e. the top of the main memory
 * @param addr the address to be displayed
 * @param sep a separator to print after the address (and its colon, printed anyway)
 *
 */
static void address_print(addr_fmt_t show_addr, const void* reference,
                          const void* addr, const char* sep)
{
    switch (show_addr) {
    case POINTER:
        (void)printf("%p", addr);
        break;
    case OFFSET:
        (void)printf("%zX", (const char*)addr - (const char*)reference);
        break;
    case OFFSET_U:
        (void)printf(SIZE_T_FMT, (const char*)addr - (const char*)reference);
        break;
    default:
        // do nothing
        return;
    }
    (void)printf(":%s", sep);
}



// ======================================================================
/**
 * @brief Tool function to print the content of a memory area
 *
 * @param reference the reference address; i.e. the top of the main memory
 * @param from first address to print
 * @param to first address NOT to print; if less that `from`, nothing is printed;
 * @param show_addr the format how to display addresses; see addr_fmt_t type in memory.h
 * @param line_size how many memory byted to print per stdout line
 * @param sep a separator to print after the address and between bytes
 *
 */
static void mem_dump_with_options(const void* reference, const void* from, const void* to,
                                  addr_fmt_t show_addr, size_t line_size, const char* sep)
{
    assert(line_size != 0);
    size_t nb_to_print = line_size;
    for (const uint8_t* addr = (const uint8_t*)from; addr < (const uint8_t*) to; ++addr) {
        if (nb_to_print == line_size) {
            address_print(show_addr, reference, addr, sep);
        }
        (void)printf("%02" PRIX8 "%s", *addr, sep);
        if (--nb_to_print == 0) {
            nb_to_print = line_size;
            putchar('\n');
        }
    }
    if (nb_to_print != line_size) putchar('\n');
}
// ======================================================================

int mem_init_from_dumpfile(const char* filename, void** memory, size_t* mem_capacity_in_bytes){ //initialising memory from dumpfile
    
    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(memory);
    M_REQUIRE_NON_NULL(mem_capacity_in_bytes);
    FILE *file;
    file = fopen(filename, "rb"); // read binary mode
    M_REQUIRE_NON_NULL_CUSTOM_ERR(file, ERR_IO);
    
    // va tout au bout du fichier
    fseek(file, 0L, SEEK_END);
    // ind ique la position, et donc la taille (en octets)
    *mem_capacity_in_bytes = (size_t) ftell(file);
    // revient au début du fichier (pour le lire par la suite)
    rewind(file);
    *memory = malloc(*mem_capacity_in_bytes);
    if(*memory==NULL){
        fclose(file);
        return ERR_MEM;
    }
    memset(*memory, 0, *mem_capacity_in_bytes );
    
    
    if(fread(*memory, *mem_capacity_in_bytes, BYTE_SIZE, file)!= BYTE_SIZE){
        fclose(file);
        free(*memory);
        return ERR_MEM;
    }
    fclose(file);
    
return ERR_NONE;
}

// ==========================================================================

static int page_file_read(char* filename, void* phyaddr){ //helper method to read at physical address from file
   M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(phyaddr);
    FILE *file;
    file = fopen(filename, "rb"); // read binary mode
    M_REQUIRE_NON_NULL_CUSTOM_ERR(file, ERR_IO);
    if(fread(phyaddr, FOURKI, BYTE_SIZE, file)!= BYTE_SIZE){
       fclose(file);
       return ERR_MEM; 
    }
     fclose(file);
    return ERR_NONE;
    }
    
int mem_init_from_description(const char* master_filename, void** memory, size_t* mem_capacity_in_bytes){

    M_REQUIRE_NON_NULL(master_filename);
    M_REQUIRE_NON_NULL(memory);
    M_REQUIRE_NON_NULL(mem_capacity_in_bytes);
    FILE *file;
    file = fopen(master_filename, "rb"); // read binary mode
    M_REQUIRE_NON_NULL_CUSTOM_ERR(file, ERR_IO);
    fscanf(file,"%zu",mem_capacity_in_bytes);//getting the total bytes to store 
    *memory = malloc(*mem_capacity_in_bytes);
    if(*memory==NULL){
        fclose(file);
        return ERR_MEM;
    }
    error_code ret=ERR_NONE;
    char pgd_filename [MAXSIZE_STRING];
    fscanf(file, "%s", pgd_filename );//getting the filename string
    if(ret= page_file_read(pgd_filename, *memory)!=ERR_NONE){
        fclose(file);
        free(*memory);
        return ret;
    }
    int num = 0;
    fscanf(file, "%d", &num); //getting the number of ilnes to read next
    uint32_t phaddr = 0;
    for(int i=0; i<num; i++){
    fscanf(file,"%x" SCNx32 ,&phaddr); //getting the physical address
    fscanf(file, "%s", pgd_filename ); // getting the filename where to take the bytes from
    
     if(ret= page_file_read(pgd_filename, ((uint8_t*)(*memory) + phaddr))!=ERR_NONE){
        fclose(file);
        free(*memory);
        return ret;
    }
    }

    uint64_t vadd = 0;
    phy_addr_t paddr;
    init_phy_addr(&paddr, 0,0); //initializing the physical address
    virt_addr_t virtaddr;
     init_virt_addr64(&virtaddr,0);//initializing the virtual address
    while (fscanf(file,"%lx" SCNx64 ,&vadd)!=EOF){ //getting the virtual address and string untill the end
        init_virt_addr64(&virtaddr, vadd);
        if(ret=page_walk(*memory, &virtaddr, &paddr)!=ERR_NONE){
            fclose(file);
            free(*memory);
            return ret;
        } //getting the physical address from virtual address 
        fscanf(file, "%s", pgd_filename ); // getting the filename where to take the bytes from
        uint32_t numM = (paddr.phy_page_num) << PAGE_OFFSET | paddr.page_offset; //getting the physical address from the struct
          if(ret= page_file_read(pgd_filename, ((uint8_t*)(*memory) + numM))!=ERR_NONE){
            fclose(file);
            free(*memory);
            return ret;
        }
    }
    fclose(file);
    return ERR_NONE;
    }
// See memory.h for description

int vmem_page_dump_with_options(const void *mem_space, const virt_addr_t *from,
                                addr_fmt_t show_addr, size_t line_size, const char *sep)
{
#ifdef DEBUG
    debug_print("mem_space=%p\n", mem_space);
    (void)fprintf(stderr, __FILE__ ":%d:%s(): virt. addr.=", __LINE__, __func__);
    print_virtual_address(stderr, from);
    (void)fputc('\n', stderr);
#endif
    phy_addr_t paddr;
    zero_init_var(paddr);

    M_EXIT_IF_ERR((error_code)page_walk(mem_space, from, &paddr),
                  "calling page_walk() from vmem_page_dump_with_options()");
#ifdef DEBUG
    (void)fprintf(stderr, __FILE__ ":%d:%s(): phys. addr.=", __LINE__, __func__);
    print_physical_address(stderr, &paddr);
    (void)fputc('\n', stderr);
#endif

    const uint32_t paddr_offset = ((uint32_t)paddr.phy_page_num << PAGE_OFFSET);
    const char *const page_start = (const char *)mem_space + paddr_offset;
    const char *const start = page_start + paddr.page_offset;
    const char *const end_line = start + (line_size - paddr.page_offset % line_size);
    const char *const end = page_start + PAGE_SIZE;
    debug_print("start=%p (offset=%zX)\n", (const void *)start, start - (const char *)mem_space);
    debug_print("end  =%p (offset=%zX)\n", (const void *)end, end - (const char *)mem_space);
    mem_dump_with_options(mem_space, page_start, start, show_addr, line_size, sep);
    const size_t indent = paddr.page_offset % line_size;
    if (indent == 0)
        putchar('\n');
    address_print(show_addr, mem_space, start, sep);
    for (size_t i = 1; i <= indent; ++i)
        printf("  %s", sep);
    mem_dump_with_options(mem_space, start, end_line, NONE, line_size, sep);
    mem_dump_with_options(mem_space, end_line, end, show_addr, line_size, sep);
    return ERR_NONE;
}
