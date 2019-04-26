
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
	virt_addr_t  vaddr;
    phy_addr_t  paddr;
    error_code ret=ERR_NONE;
    M_REQUIRE((ret=init_phy_addr(&paddr,0,0))!=ERR_NONE, ret, "Error while initialising physical address");
    M_REQUIRE((ret=init_virt_addr64(&vaddr,0))!=ERR_NONE, ret, "Error while initialising virtual address");
   
	for(int i =0;i< TLB_LINES; i++){
		 M_REQUIRE((ret=tlb_entry_init(&vaddr, &paddr, (tlb+i)))!=ERR_NONE, ret, "Error while initialising tlb entry");
  
	}
	return ret;
	
}
int tlb_insert( uint32_t line_index,
                const tlb_entry_t * tlb_entry,
                tlb_entry_t * tlb){
		M_REQUIRE_NON_NULL(tlb);			
		M_REQUIRE_NON_NULL(tlb_entry);
		tlb_entry_t * entry= tlb+line_index;// this returns the pointer to the entry to be modified
		M_REQUIRE_NON_NULL(entry);
		entry->tag= tlb_entry->tag;
		entry->phy_page_num= tlb_entry->phy_page_num;
		entry->v= tlb_entry->v;
		return ERR_NONE;
}

int tlb_hit(const virt_addr_t * vaddr, phy_addr_t * paddr, const tlb_entry_t * tlb, replacement_policy_t * replacement_policy){
	if(tlb == NULL || replacement_policy==NULL || paddr==NULL||vaddr==NULL)
		return 0;
	uint64_t vadr= virt_addr_t_to_uint64_t(vaddr);
	list_content_t i=0;
		for_all_nodes_reverse(n, replacement_policy->ll){
			i=n->value;
			if((tlb+i)->tag==vadr && (tlb+i)->v==1){
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
					
		if(tlb_hit(vaddr, paddr, tlb, replacement_policy)==1){
		*hit_or_miss=1;
		return ERR_NONE;}
		M_EXIT_IF_ERR(page_walk(mem_space,vaddr, paddr ), "while calling page walk");
		tlb_entry_t entry;
		tlb_entry_init(vaddr, paddr, &entry);
		//put the entry in the tlb at the value stored at node 0 of the ll 
		list_content_t index= replacement_policy->ll->front->value;
		*(tlb+index)=entry;
		replacement_policy->move_back(replacement_policy->ll, replacement_policy->ll->front);
		return ERR_NONE;
		}
