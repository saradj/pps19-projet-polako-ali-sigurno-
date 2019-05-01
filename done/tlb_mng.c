
/**
 * @file tlb_mng.c
 * @brief TLB management functions for fully-associative TLB
 *
 * @author 
 * @date 2018-19
 */
 #include "tlb_mng.h"
 
#include "tlb.h"
#include "addr_mng.h"
#include "addr.h"
#include "list.h"
#include "error.h"
#include "page_walk.h"
#include "util.h"

int tlb_entry_init( const virt_addr_t * vaddr,
                    const phy_addr_t * paddr,
                    tlb_entry_t * tlb_entry){
M_REQUIRE_NON_NULL(tlb_entry);
M_REQUIRE_NON_NULL(vaddr);
M_REQUIRE_NON_NULL(paddr);
	tlb_entry->tag = virt_addr_t_to_virtual_page_number(vaddr);	
	tlb_entry->phy_page_num= paddr->phy_page_num;	
	tlb_entry->v=1;		
	return ERR_NONE;
	
}
 
int tlb_flush(tlb_entry_t * tlb){

M_REQUIRE_NON_NULL(tlb);    
   
	for(int i =0;i< TLB_LINES; i++){
	//	tlb[i].phy_page_num=0;
	//	tlb[i].tag=0;
	//	tlb[i].v=0;
	zero_init_var(tlb[i]);
	}
	return ERR_NONE;
	
}
int tlb_insert( uint32_t line_index,
                const tlb_entry_t * tlb_entry,
                tlb_entry_t * tlb){
				
		M_REQUIRE_NON_NULL(tlb);			
		M_REQUIRE_NON_NULL(tlb_entry);
		 tlb[line_index]= *tlb_entry;// this returns the pointer to the entry to be modified	
		return ERR_NONE;
}

int tlb_hit(const virt_addr_t * vaddr, phy_addr_t * paddr, const tlb_entry_t * tlb, replacement_policy_t * replacement_policy){
	
	if(tlb == NULL || replacement_policy==NULL || paddr==NULL||vaddr==NULL){
		
		return 0;
		}
		
	uint64_t vadr= virt_addr_t_to_virtual_page_number(vaddr);
	list_content_t i=0;
		for_all_nodes_reverse(n, replacement_policy->ll){
			i=n->value;
			if(tlb[i].tag==vadr && tlb[i].v==1){
			replacement_policy->move_back(replacement_policy->ll,n); //move back node i
				paddr->phy_page_num=(tlb+i)->phy_page_num;
			paddr->page_offset= vaddr->page_offset;
						return 1;
			}
		}
return 0;				
	}
	//– puis on l’insère dans le TLB à l’index stocké en tête de liste chaînée ;
//– et on déplace cette tête en « fin » de liste (appel de la méthodemove_back() de la politique de remplacement)
	int tlb_search( const void * mem_space, const virt_addr_t * vaddr, phy_addr_t * paddr, 
	tlb_entry_t * tlb, replacement_policy_t * replacement_policy, int* hit_or_miss){
		
		M_REQUIRE_NON_NULL(mem_space);
		M_REQUIRE_NON_NULL(vaddr);
		M_REQUIRE_NON_NULL(paddr);
		M_REQUIRE_NON_NULL(tlb);
		M_REQUIRE_NON_NULL(replacement_policy);
		M_REQUIRE_NON_NULL(hit_or_miss);
		
		if(tlb_hit(vaddr, paddr, tlb, replacement_policy)==1){
		*hit_or_miss=1;
		return ERR_NONE;}
		*hit_or_miss=0;
		M_EXIT_IF_ERR(page_walk(mem_space,vaddr, paddr ), "while calling page walk");
		
		tlb_entry_t entry;
		M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &entry), "while initialising tlb entry");
		
		list_content_t index=0;
		//put the entry in the tlb at the value stored at node 0 of the ll 
		if(0==is_empty_list(replacement_policy->ll)){

		 index= replacement_policy->ll->front->value;
		 }
		
			M_EXIT_IF_ERR(tlb_insert(index, &entry, tlb), "while inserting the tlb entry");
		replacement_policy->move_back(replacement_policy->ll, replacement_policy->ll->front);
		return ERR_NONE;
		}
