#include "page_walk.h"
#include "addr_mng.h"
#include "addr.h"
#include "error.h"

static inline pte_t read_page_entry(const pte_t* start, 
                                    pte_t page_start,
                                    uint16_t index){
										
					 				
int i = index +  (page_start>>2);
fprintf(stderr, "i == %d\n", i);	
				
return start[i]; }

int page_walk(const void* mem_space, const virt_addr_t* vaddr, phy_addr_t* paddr){
 
 pte_t start_pud = read_page_entry((pte_t *) mem_space, 0 ,vaddr->pgd_entry); 
  fprintf(stderr, "after start");
 pte_t start_pmd =read_page_entry((pte_t *) mem_space, start_pud ,vaddr->pud_entry);

 pte_t start_pte =read_page_entry((pte_t *) mem_space, start_pmd ,vaddr->pmd_entry);
 pte_t page_begin =read_page_entry((pte_t *)mem_space, start_pte ,vaddr->pte_entry);
 uint16_t page_offset = vaddr->page_offset;
 return  init_phy_addr(paddr,page_begin,page_offset);

 
 
}
