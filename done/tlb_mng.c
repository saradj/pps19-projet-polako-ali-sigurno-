
/**
 * @file tlb_mng.c
 * @brief implementations of TLB management functions for fully-associative TLB
 *
 * @date 2019
 */
#include "tlb_mng.h"
#include "tlb.h"
#include "addr_mng.h"
#include "addr.h"
#include "list.h"
#include "error.h"
#include "page_walk.h"
#include "util.h"

int tlb_entry_init(const virt_addr_t *vaddr,
				   const phy_addr_t *paddr,
				   tlb_entry_t *tlb_entry)
{
	M_REQUIRE_NON_NULL(tlb_entry);
	M_REQUIRE_NON_NULL(vaddr);
	M_REQUIRE_NON_NULL(paddr);
	tlb_entry->tag = virt_addr_t_to_virtual_page_number(vaddr);
	tlb_entry->phy_page_num = paddr->phy_page_num;
	tlb_entry->v = 1;
	return ERR_NONE;
}

int tlb_flush(tlb_entry_t *tlb)
{
	M_REQUIRE_NON_NULL(tlb);
	for (list_content_t i = 0; i < TLB_LINES; i++) //initializing all of the entries in the tlb to 0
	{
		memset(&tlb[i],0, sizeof(tlb[i]));
	}
	return ERR_NONE;
}

int tlb_insert(uint32_t line_index,
			   const tlb_entry_t *tlb_entry,
			   tlb_entry_t *tlb)
{
	M_REQUIRE_NON_NULL(tlb);
	M_REQUIRE_NON_NULL(tlb_entry);
	M_REQUIRE(line_index < TLB_LINES, ERR_BAD_PARAMETER, "line index is greater that max number of lines line_index = %u", line_index);
	tlb[line_index] = *tlb_entry; // after verification inserting the given entry at the given index in the tlb passed as an argument
	return ERR_NONE;
}

int tlb_hit(const virt_addr_t *vaddr, phy_addr_t *paddr, const tlb_entry_t *tlb, replacement_policy_t *replacement_policy)
{
	if (tlb == NULL || replacement_policy == NULL || paddr == NULL || vaddr == NULL || replacement_policy->ll == NULL)
	{
		return 0; // if the arguments are not valid return 0=miss
	}
	uint64_t vpg_num = virt_addr_t_to_virtual_page_number(vaddr);
	list_content_t i = 0;							 // i will store the index of the required entry in the tlb
	for_all_nodes_reverse(n, replacement_policy->ll) // looping trough all the nodes of the linked list in reverse order
	{
		i = n->value;
		if (tlb[i].tag == vpg_num && tlb[i].v == 1) // finding the right entry in the tlb if it exists
		{
			replacement_policy->move_back(replacement_policy->ll, n);					  //move back node that keeps the index i, since it was just used
			init_phy_addr(paddr, tlb[i].phy_page_num << PAGE_OFFSET, vaddr->page_offset); // initialise physical address

			return 1; // return hit
		}
	}
	return 0; // no corresponding entry for that virt address was found = miss
}
int tlb_search(const void *mem_space, const virt_addr_t *vaddr, phy_addr_t *paddr,
			   tlb_entry_t *tlb, replacement_policy_t *replacement_policy, int *hit_or_miss)
{

	M_REQUIRE_NON_NULL(mem_space);
	M_REQUIRE_NON_NULL(vaddr);
	M_REQUIRE_NON_NULL(paddr);
	M_REQUIRE_NON_NULL(tlb);
	M_REQUIRE_NON_NULL(replacement_policy);
	M_REQUIRE_NON_NULL(hit_or_miss);
	M_REQUIRE_NON_NULL(replacement_policy->ll);

	if (tlb_hit(vaddr, paddr, tlb, replacement_policy) == 1)
	{
		*hit_or_miss = 1; // if it was a hit, everything was done in tlb_hit
		return ERR_NONE;
	}
	*hit_or_miss = 0; // it was a miss
	M_EXIT_IF_ERR(page_walk(mem_space, vaddr, paddr), "while calling page walk");
	tlb_entry_t entry;
	M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &entry), "while initialising tlb entry"); // initialising a new tlb entry with the data from the virt address
	M_REQUIRE(!is_empty_list(replacement_policy->ll), ERR_BAD_PARAMETER, "linked list in replacement policy is empty, should have at least %d element", 1);
	//put the entry in the tlb at the value stored at node 0 of the ll
	list_content_t index = replacement_policy->ll->front->value;
	M_EXIT_IF_ERR(tlb_insert(index, &entry, tlb), "while inserting the tlb entry"); // inserting the initialised entry at the correct index in the tlb
	replacement_policy->move_back(replacement_policy->ll, replacement_policy->ll->front);
	return ERR_NONE;
}
