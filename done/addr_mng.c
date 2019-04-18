#include "addr_mng.h" 
#include <stdio.h> 
#include "inttypes.h"
//#include "addr.h"
#include "error.h"


int print_virtual_address(FILE* where, const virt_addr_t* vaddr){
  int sum =fprintf(where, " PGD = %x" PRIX16 ,vaddr->pgd_entry);
  sum +=fprintf(where, " PMD = %x" PRIX16 ,vaddr->pmd_entry);
  sum +=fprintf(where, " PTE = %x" PRIX16 ,vaddr->pte_entry);
  sum +=fprintf(where, " offset = %x" PRIX16 ,vaddr->page_offset);

  return sum ; 

}

int init_virt_addr(virt_addr_t * vaddr,   uint16_t pgd_entry, uint16_t pud_entry, uint16_t pmd_entry, uint16_t pte_entry, uint16_t page_offset){

 M_REQUIRE_NON_NULL(vaddr);
 vaddr->pgd_entry = pgd_entry;
 vaddr->pud_entry= pud_entry;
 vaddr->pmd_entry= pmd_entry;
 vaddr->pte_entry=pte_entry;
 vaddr->page_offset=page_offset;
 return ERR_NONE;

}

int init_virt_addr64(virt_addr_t * vaddr, uint64_t vaddr64){
M_REQUIRE_NON_NULL(vaddr);
int mask_entry = 0b111111111;
int mask_offset= 0b111111111111;
uint16_t offset = vaddr64 & mask_offset;
uint16_t pte= (vaddr64>>12 )& mask_entry;
uint16_t pmd= (vaddr64>>21)& mask_entry;
uint16_t pud= (vaddr64>>30)& mask_entry;
uint16_t pgd= (vaddr64>>39)& mask_entry;
init_virt_addr(vaddr,pgd,pud,pmd,pte,offset);
return ERR_NONE;

}

uint64_t virt_addr_t_to_virtual_page_number(const virt_addr_t * vaddr){
M_REQUIRE_NON_NULL(vaddr);
uint16_t pte = vaddr->pte_entry;
uint16_t pmd = vaddr->pmd_entry;
uint16_t pud = vaddr->pud_entry;
uint16_t pgd = vaddr->pgd_entry;

uint64_t r = pte | (pmd<<9)|(pud<<18)|(pgd<<27);
return r;

}

uint64_t virt_addr_t_to_uint64_t(const virt_addr_t * vaddr){
    uint64_t r =  virt_addr_t_to_virtual_page_number(vaddr);
    uint16_t offset = vaddr->page_offset;
    return  r<<12| offset;
	

}

int print_physical_address(FILE* where, const phy_addr_t* paddr){
     int sum =fprintf(where, " page num = 0x%" PRIX32 ,paddr->phy_page_num);
     sum+= fprintf(where, " offset = 0x%" PRIX16 ,paddr->page_offset);
 return sum;
}


int init_phy_addr(phy_addr_t* paddr, uint32_t page_begin, uint32_t page_offset){
    int mask = 0XFFFFF;
    paddr->phy_page_num= (mask&page_begin);
    paddr->page_offset=page_offset;
     return ERR_NONE;


}




