/**
 * @file tlb_hrchy_mng.c
 * @brief implementation of TLB management functions for two-level hierarchy of TLBs
 * 
 * @date 2019
 */
#include "tlb_hrchy.h"
#include "tlb_hrchy_mng.h"
#include "addr_mng.h"
#include "error.h"
#include "util.h"
#include "page_walk.h"
#include "list.h"

int tlb_flush(void *tlb, tlb_t tlb_type)
{
    M_REQUIRE_NON_NULL(tlb);

    if (tlb_type == L1_ITLB) // case L1 Instruction
    {
        l1_itlb_entry_t *t = tlb; // cast the tlb to l1_i...
        for (list_content_t i = 0; i < L1_ITLB_LINES; i++)
        {
            zero_init_var(t[i]); // initialise all entiries to 0
        }
    }

    else if (tlb_type == L1_DTLB) // case l1 Data
    {
        l1_dtlb_entry_t *t = tlb; // cast the tlb to l1_d
        for (list_content_t i = 0; i < L1_DTLB_LINES; i++)
        {
            zero_init_var(t[i]); // initialise all entiries to 0
        }
    }

    else // case l2
    {
        l2_tlb_entry_t *t = tlb; // cast the tlb to L2 tlb
        for (list_content_t i = 0; i < L2_TLB_LINES; i++)
        {
            zero_init_var(t[i]); // initialise all entiries to 0
        }
    }
    return ERR_NONE;
}

int tlb_entry_init(const virt_addr_t *vaddr,
                   const phy_addr_t *paddr,
                   void *tlb_entry,
                   tlb_t tlb_type)
{
    M_REQUIRE_NON_NULL(tlb_entry);
    M_REQUIRE_NON_NULL(vaddr);
    M_REQUIRE_NON_NULL(paddr);

    if (tlb_type == L1_ITLB)
    {
        l1_itlb_entry_t *temp = tlb_entry;
        temp->tag = virt_addr_t_to_virtual_page_number(vaddr) >> L1_ITLB_LINES_BITS; // if the type is l1, the tag doesn't take the last 4 bits of vpg_num, they are used for line indexing
        temp->phy_page_num = paddr->phy_page_num;
        temp->v = 1;
    }
    else if (tlb_type == L1_DTLB)
    {
        l1_dtlb_entry_t *temp = tlb_entry;
        temp->tag = virt_addr_t_to_virtual_page_number(vaddr) >> L1_DTLB_LINES_BITS;
        temp->phy_page_num = paddr->phy_page_num;
        temp->v = 1;
    }
    else
    {
        l2_tlb_entry_t *temp = tlb_entry;
        temp->tag = virt_addr_t_to_virtual_page_number(vaddr) >> L2_TLB_LINES_BITS; // if the type is l2, the last 6 bits of the vpg_num are used for line indexing, so not taken in tag
        temp->phy_page_num = paddr->phy_page_num;
        temp->v = 1;
    }
    return ERR_NONE;
}

int tlb_insert(uint32_t line_index,
               const void *tlb_entry,
               void *tlb,
               tlb_t tlb_type)
{

    M_REQUIRE_NON_NULL(tlb);
    M_REQUIRE_NON_NULL(tlb_entry);
    switch (tlb_type)
    {
    case L1_ITLB:
    {
        M_REQUIRE(line_index < L1_ITLB_LINES, ERR_BAD_PARAMETER, "line index: %u , to insert at is greater then number of lines", line_index);
        l1_itlb_entry_t *tmp = tlb;         // casting the tlb to l1 instruction
        l1_itlb_entry_t *entry = tlb_entry; // casting the tlb entry to l1 instruction
        tmp[line_index] = *entry;           // insert the entry
        break;
    }
    case L1_DTLB:
    {
        M_REQUIRE(line_index < L1_DTLB_LINES, ERR_BAD_PARAMETER, "line index: %u , to insert at is greater then number of lines", line_index);
        l1_dtlb_entry_t *tmp = tlb;         // casting the tlb to l1 data
        l1_dtlb_entry_t *entry = tlb_entry; // casting the tlb entry to l1 data
        tmp[line_index] = *entry;
        break;
    }
    case L2_TLB:
    {
        M_REQUIRE(line_index < L2_TLB_LINES, ERR_BAD_PARAMETER, "line index: %u , to insert at is greater then number of lines", line_index);
        l2_tlb_entry_t *tmp = tlb;         // casting the tlb to l2
        l2_tlb_entry_t *entry = tlb_entry; // casting the tlb entry to l2
        tmp[line_index] = *entry;
        break;
    }
    default: // only 3 types will never reach default
        break;
    }
    return ERR_NONE;
}

int tlb_hit(const virt_addr_t *vaddr,
            phy_addr_t *paddr,
            const void *tlb,
            tlb_t tlb_type)
{
    if (tlb == NULL || paddr == NULL || vaddr == NULL)
        return 0; // if arguments are not valid it's a miss
    uint64_t vpg_num = virt_addr_t_to_virtual_page_number(vaddr);
    list_content_t line_index = 0;

    switch (tlb_type)
    {
    case L1_ITLB:
    {
        line_index = vpg_num % L1_ITLB_LINES; // getting the line index from the virtual pg num
        l1_itlb_entry_t *tmp = tlb;
        if (tmp[line_index].tag == (vpg_num >> L1_ITLB_LINES_BITS) && tmp[line_index].v == 1) // checking if it's a hit
        {
            init_phy_addr(paddr, tmp[line_index].phy_page_num << PAGE_OFFSET, vaddr->page_offset);
            return 1;
        }
    }
    case L1_DTLB:
    {
        line_index = vpg_num % L1_DTLB_LINES;
        l1_dtlb_entry_t *tmp = tlb;
        if (tmp[line_index].tag == (vpg_num >> L1_DTLB_LINES_BITS) && tmp[line_index].v == 1)
        {
            init_phy_addr(paddr, tmp[line_index].phy_page_num << PAGE_OFFSET, vaddr->page_offset);
            return 1;
        }
    }
    case L2_TLB:
    {
        line_index = vpg_num % L2_TLB_LINES;

        l2_tlb_entry_t *tmp = tlb;
        if (tmp[line_index].tag == (vpg_num >> L2_TLB_LINES_BITS) && tmp[line_index].v == 1)
        {
            init_phy_addr(paddr, tmp[line_index].phy_page_num << PAGE_OFFSET, vaddr->page_offset);
            return 1;
        }
    }
    default:
        return 0;
    }
    return 0; // if it was not found in the specific tlb it's a miss
}
int tlb_search(const void *mem_space,
               const virt_addr_t *vaddr,
               phy_addr_t *paddr,
               mem_access_t access,
               l1_itlb_entry_t *l1_itlb,
               l1_dtlb_entry_t *l1_dtlb,
               l2_tlb_entry_t *l2_tlb,
               int *hit_or_miss)
{

    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(vaddr);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(l1_itlb);
    M_REQUIRE_NON_NULL(l1_dtlb);
    M_REQUIRE_NON_NULL(l2_tlb);
    M_REQUIRE_NON_NULL(hit_or_miss);

    if (access == INSTRUCTION && (tlb_hit(vaddr, paddr, l1_itlb, L1_ITLB)))
    {
        *hit_or_miss = 1;
        return ERR_NONE; // everything was done in tlb_hit
    }
    if (access == DATA && (tlb_hit(vaddr, paddr, l1_dtlb, L1_DTLB)))
    {
        *hit_or_miss = 1;
        return ERR_NONE; // everything was done in tlb_hit
    }
    uint64_t vpg_num = virt_addr_t_to_virtual_page_number(vaddr);
    list_content_t line_index = 0;
    if (tlb_hit(vaddr, paddr, l2_tlb, L2_TLB)) // it's a hit in l2
    {
        *hit_or_miss = 1; // hit in l2, but must recopy the information in the corresponding l1 tlb

        if (access == INSTRUCTION)
        {
            line_index = vpg_num % L1_ITLB_LINES; // getting the corresponding line index for l1 instruction
            l1_itlb_entry_t ie;
            M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &ie, L1_ITLB), "while initialising instruction tlb entry");
            M_EXIT_IF_ERR(tlb_insert(line_index, &ie, l1_itlb, L1_ITLB), "while inserting the instruction tlb entry"); // putting the informationin l1 instruction tbl
        }
        else
        {
            line_index = vpg_num % L1_DTLB_LINES; // getting the corresponding line index for l1 data
            l1_dtlb_entry_t de;
            M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &de, L1_DTLB), "while initialising data tlb entry");
            M_EXIT_IF_ERR(tlb_insert(line_index, &de, l1_dtlb, L1_DTLB), "while inserting the  data tlb entry"); // putting the informationin l1 data tbl
        }
        return ERR_NONE;
    }
    *hit_or_miss = 0; // if it's not in l1 or l2

    M_EXIT_IF_ERR(page_walk(mem_space, vaddr, paddr), "while calling page walk");
    l2_tlb_entry_t entry;
    M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &entry, L2_TLB), "while initialising tlb entry"); // initialise a level 2 tlb entry
    line_index = vpg_num % L2_TLB_LINES;
    int isValid = 0;
    uint32_t tag = 0;
    if (l2_tlb[line_index].v == 1)// if there was an entry before in l2 
    {
        isValid = 1;
        tag = l2_tlb[line_index].tag << 2;
        tag = tag | (line_index >> 4);                                                                     // 32 bit tag = (30 bit tag from l2 & 2 first bits of line index of l2)
    }                                                                                                      // getting the right index in l2
    M_EXIT_IF_ERR(tlb_insert(line_index, &entry, l2_tlb, L2_TLB);, "while inserting the tlb entry in L2"); // insert it
    if (access == INSTRUCTION)
    {
        line_index = vpg_num % L1_ITLB_LINES;
        l1_itlb_entry_t ientry;
        M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &ientry, L1_ITLB), "while initialising tlb entry");
        M_EXIT_IF_ERR(tlb_insert(line_index, &ientry, l1_itlb, L1_ITLB);, "while inserting the tlb entry in L1 I"); // inserting the data in the tlb l1 according to the access
        if (isValid == 1 && l1_dtlb[line_index].tag == tag)
            l1_dtlb[line_index].v = 0; // de-validate the other l1 tlb entry at that index if it was valid and it's tag was=l2 tag
    }
    else
    {
        line_index = vpg_num % L1_DTLB_LINES;
        l1_dtlb_entry_t d_entry;
        M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &d_entry, L1_DTLB), "while initialising tlb entry");
        M_EXIT_IF_ERR(tlb_insert(line_index, &d_entry, l1_dtlb, L1_DTLB);, "while inserting the tlb data entry in L1");
        if (isValid == 1 && l1_itlb[line_index].tag == tag)
            l1_itlb[line_index].v = 0; // de-validate the other l1 tlb entry at that index if it was valid and it's tag was=l2 tag
    }
    return ERR_NONE;
}
