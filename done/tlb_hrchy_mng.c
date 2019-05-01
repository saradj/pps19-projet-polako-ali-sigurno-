#include "tlb_hrchy.h"
#include "tlb_hrchy_mng.h"
#include "addr_mng.h"
#include "error.h"
#include "util.h"
#include "page_walk.h"

int tlb_flush(void *tlb, tlb_t tlb_type){


if(tlb_type==L1_ITLB){
    
l1_itlb_entry_t * t= tlb;

for(int i =0;i< L1_ITLB_LINES; i++){
    zero_init_var(t[i]);
}

}

else if(tlb_type==L1_DTLB){
    l1_dtlb_entry_t * t= tlb;

for(int i =0;i<L1_DTLB_LINES; i++){
     zero_init_var(t[i]);

}
	
}

else{
   l2_tlb_entry_t * t= tlb;

for(int i =0;i<L2_TLB_LINES; i++){
 zero_init_var(t[i]);
}
	
}
return ERR_NONE;
}



int tlb_entry_init( const virt_addr_t * vaddr,
                    const phy_addr_t * paddr,
                    void * tlb_entry,
                    tlb_t tlb_type){// how can we return an error if it's not type of tlb type?
M_REQUIRE_NON_NULL(tlb_entry);
M_REQUIRE_NON_NULL(vaddr);
M_REQUIRE_NON_NULL(paddr);

if(tlb_type==L1_ITLB){
    l1_itlb_entry_t* temp=tlb_entry;
    temp->tag=virt_addr_t_to_virtual_page_number(vaddr)>>L1_ITLB_LINES_BITS;
    temp->phy_page_num=paddr->phy_page_num;
    temp->v=1;
}
else if(tlb_type==L1_DTLB){
     l1_dtlb_entry_t* temp=tlb_entry;
    temp->tag=virt_addr_t_to_virtual_page_number(vaddr)>>L1_DTLB_LINES_BITS;
    temp->phy_page_num=paddr->phy_page_num;
    temp->v=1;
}
else{
    l2_tlb_entry_t* temp=tlb_entry;
    temp->tag=virt_addr_t_to_virtual_page_number(vaddr)>>L2_TLB_LINES_BITS;
    temp->phy_page_num=paddr->phy_page_num;
    temp->v=1;
}
return ERR_NONE;
  }                 

  int tlb_insert( uint32_t line_index,
                const void * tlb_entry,
                void * tlb,
                tlb_t tlb_type){

    M_REQUIRE_NON_NULL(tlb);
    M_REQUIRE_NON_NULL(tlb_entry);
       switch (tlb_type)
    {
    case L1_ITLB:{
         M_REQUIRE(line_index<L1_ITLB_LINES, ERR_BAD_PARAMETER, "line index: %u to insert at greater then number of lines", line_index);
      // l1_itlb_entry_t* tmp = (tlb + line_index);
     // *tmp=*((l1_itlb_entry_t*)tlb_entry);
      l1_itlb_entry_t* tmp=tlb;
      l1_itlb_entry_t* entry=tlb_entry;
      tmp[line_index]=*entry;
        break;
    }
     case L1_DTLB:{
         M_REQUIRE(line_index<L1_DTLB_LINES, ERR_BAD_PARAMETER, "line index: %u to insert at greater then number of lines", line_index);
       l1_dtlb_entry_t* tmp=tlb;
      l1_dtlb_entry_t* entry=tlb_entry;
      tmp[line_index]=*entry;
        break;
    }
     case L2_TLB:{
         M_REQUIRE(line_index<L2_TLB_LINES, ERR_BAD_PARAMETER, "line index: %u to insert at greater then number of lines", line_index);
        l2_tlb_entry_t* tmp=tlb;
      l2_tlb_entry_t* entry=tlb_entry;
      tmp[line_index]=*entry;
        break;
    }
    default: 
        break;
    }
    return ERR_NONE;
                
                }

    int tlb_hit( const virt_addr_t * vaddr,
             phy_addr_t * paddr,
             const void  * tlb,
             tlb_t tlb_type){
	if(tlb == NULL || paddr==NULL||vaddr==NULL)
		return 0;
	uint64_t vadr= virt_addr_t_to_virtual_page_number(vaddr);
    uint32_t line_index=0;
   

       switch (tlb_type)
    {
    case L1_ITLB:{
         line_index=vadr%L1_ITLB_LINES;
    l1_itlb_entry_t* tmp=tlb;
	if(tmp[line_index].tag==(vadr>>L1_ITLB_LINES_BITS) && tmp[line_index].v==1){
   init_phy_addr(paddr,tmp[line_index].phy_page_num<<PAGE_OFFSET,vaddr->page_offset);
            	return 1;
    }
      
        
    }
     case L1_DTLB:{
        line_index=vadr%L1_DTLB_LINES;
    l1_dtlb_entry_t* tmp=tlb;
		if(tmp[line_index].tag==(vadr>>L1_DTLB_LINES_BITS) && tmp[line_index].v==1){
              
            init_phy_addr(paddr,tmp[line_index].phy_page_num<<PAGE_OFFSET,vaddr->page_offset);
            	return 1;
    }
        
    }
     case L2_TLB:{
        line_index=vadr%L2_TLB_LINES;

    l2_tlb_entry_t* tmp=tlb;
		if(tmp[line_index].tag==(vadr>>L2_TLB_LINES_BITS) && tmp[line_index].v==1){
     init_phy_addr(paddr,tmp[line_index].phy_page_num<<PAGE_OFFSET,vaddr->page_offset);
            	return 1;
    }
       
    }
    default: return 0;
       
    }
    return 0;
             }
int tlb_search( const void * mem_space,
                const virt_addr_t * vaddr,
                phy_addr_t * paddr,
                mem_access_t access,
                l1_itlb_entry_t * l1_itlb,
                l1_dtlb_entry_t * l1_dtlb,
                l2_tlb_entry_t * l2_tlb,
                int* hit_or_miss){

		M_REQUIRE_NON_NULL(mem_space);
		M_REQUIRE_NON_NULL(vaddr);
		M_REQUIRE_NON_NULL(paddr);
		M_REQUIRE_NON_NULL(l1_itlb);
 		M_REQUIRE_NON_NULL(l1_dtlb);
        M_REQUIRE_NON_NULL(l2_tlb);
		M_REQUIRE_NON_NULL(hit_or_miss);

if(access==INSTRUCTION && (tlb_hit(vaddr,paddr,l1_itlb,L1_ITLB)==1)){
    *hit_or_miss=1;
    return ERR_NONE;
}
if(access==DATA && (tlb_hit(vaddr,paddr,l1_dtlb,L1_DTLB)==1)){
    *hit_or_miss=1;
    return ERR_NONE;
}
if(tlb_hit(vaddr,paddr,l2_tlb,L2_TLB)){
     *hit_or_miss=1;
    //, mais aussi à RECOPIEE les bonnes information dans le TLB
//niveau 1 correspondant (ITLB ou DTLB en fonction de l’accès demandé) ; how ??
 uint64_t vadr= virt_addr_t_to_virtual_page_number(vaddr);
    uint32_t line_index;
if(access==INSTRUCTION){
    line_index=vadr%L1_ITLB_LINES;
    l1_itlb_entry_t ie;
    // what should we insert??
    tlb_entry_init(vaddr,paddr,&ie, L1_ITLB);
    tlb_insert(line_index,&ie,l1_itlb,L1_ITLB);
}
else
{
     line_index=vadr%L1_DTLB_LINES;
    l1_dtlb_entry_t de;
    // what should we insert??
    tlb_entry_init(vaddr,paddr,&de, L1_DTLB);
    tlb_insert(line_index,&de,l1_dtlb,L1_DTLB);
}

return ERR_NONE;
}
 *hit_or_miss=0;
 
 	M_EXIT_IF_ERR(page_walk(mem_space,vaddr, paddr ), "while calling page walk");
     l2_tlb_entry_t entry;
     M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &entry, L2_TLB), "while initialising tlb entry");
     uint64_t vadr= virt_addr_t_to_virtual_page_number(vaddr);
    uint32_t line_index=vadr%L2_TLB_LINES;
      M_EXIT_IF_ERR(tlb_insert(line_index, &entry, l2_tlb, L2_TLB);, "while inserting the tlb entry in L2");
    if(access==INSTRUCTION){
         line_index=vadr%L1_ITLB_LINES;
         l1_itlb_entry_t ientry;
         M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &ientry, L1_ITLB), "while initialising tlb entry");
         M_EXIT_IF_ERR(tlb_insert(line_index, &ientry, l1_itlb, L1_ITLB);, "while inserting the tlb entry in L1 I");
          l1_dtlb[line_index].v=0;

    }
    else{
line_index=vadr%L1_DTLB_LINES;
         l1_dtlb_entry_t d_entry;
         M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &d_entry, L1_DTLB), "while initialising tlb entry");
         M_EXIT_IF_ERR(tlb_insert(line_index, &d_entry, l1_dtlb, L1_DTLB);, "while inserting the tlb data entry in L1");
    l1_itlb[line_index].v=0;
    }
    return ERR_NONE;
                }
