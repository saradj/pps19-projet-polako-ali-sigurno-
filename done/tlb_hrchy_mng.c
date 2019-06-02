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
#define OFF 2
#define LINE_OFF 4
int tlb_flush(void *tlb, tlb_t tlb_type)
{
    M_REQUIRE_NON_NULL(tlb);
    switch (tlb_type)
    {
    case L1_ITLB:
    {

        l1_itlb_entry_t *t = tlb; // cast the tlb to l1_i...
        for (list_content_t i = 0; i < L1_ITLB_LINES; i++)
        {
            memset(&t[i], 0, sizeof(t[i])); // initialise all entiries to 0
        }
    }
    break;

    case L1_DTLB: // case l1 Data
    {
        l1_dtlb_entry_t *t = tlb; // cast the tlb to l1_d
        for (list_content_t i = 0; i < L1_DTLB_LINES; i++)
        {
            memset(&t[i], 0, sizeof(t[i])); // initialise all entiries to 0
        }
    }
    break;

    case L2_TLB: // case l2
    {
        l2_tlb_entry_t *t = tlb; // cast the tlb to L2 tlb
        for (list_content_t i = 0; i < L2_TLB_LINES; i++)
        {
            memset(&t[i], 0, sizeof(t[i])); // initialise all entiries to 0
        }
    }
    break;
    default:
        return ERR_BAD_PARAMETER;
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
#define init(type, LINES_BITS)                                                          \
    ((type *)tlb_entry)->tag = virt_addr_t_to_virtual_page_number(vaddr) >> LINES_BITS; \
    ((type *)tlb_entry)->phy_page_num = paddr->phy_page_num;                            \
    ((type *)tlb_entry)->v = 1;
    switch (tlb_type)
    {
    case L1_ITLB:
    {
        init(l1_itlb_entry_t, L1_ITLB_LINES_BITS);
    }
    break;
    case L1_DTLB:
    {
        init(l1_dtlb_entry_t, L1_DTLB_LINES_BITS);
    }
    break;
    case L2_TLB:
    {
        init(l2_tlb_entry_t, L2_TLB_LINES_BITS);
    }
    break;
    default:
        return ERR_BAD_PARAMETER;
    }
    return ERR_NONE;
}

int tlb_insert(uint32_t line_index,
               const void *tlb_entry,
               void *tlb,
               tlb_t tlb_type)
{
#define insert(type, LINES)                                                                                                        \
    M_REQUIRE(line_index < LINES, ERR_BAD_PARAMETER, "line index: %u , to insert at is greater then number of lines", line_index); \
    ((type *)tlb)[line_index] = *((type *)tlb_entry);

    M_REQUIRE_NON_NULL(tlb);
    M_REQUIRE_NON_NULL(tlb_entry);
    switch (tlb_type)
    {
    case L1_ITLB:
    {
        insert(l1_itlb_entry_t, L1_ITLB_LINES);
        break;
    }
    case L1_DTLB:
    {
        insert(l1_dtlb_entry_t, L1_DTLB_LINES);
        break;
    }
    case L2_TLB:
    {
        insert(l2_tlb_entry_t, L2_TLB_LINES);
        break;
    }
    default:
        return ERR_BAD_PARAMETER;
    }
    return ERR_NONE;
}

int tlb_hit(const virt_addr_t *vaddr,
            phy_addr_t *paddr,
            const void *tlb,
            tlb_t tlb_type)
{
#define hit(type, LINES, LINES_BITS)                                                           \
    line_index = vpg_num % LINES;                                                              \
    type *tmp = tlb;                                                                           \
    if (tmp[line_index].tag == (vpg_num >> LINES_BITS) && tmp[line_index].v == 1)              \
    {                                                                                          \
        init_phy_addr(paddr, tmp[line_index].phy_page_num << PAGE_OFFSET, vaddr->page_offset); \
        return 1;                                                                              \
    }

    if (tlb == NULL || paddr == NULL || vaddr == NULL)
        return 0; // if arguments are not valid it's a miss
    uint64_t vpg_num = virt_addr_t_to_virtual_page_number(vaddr);
    list_content_t line_index = 0;
    // getting the line index from the virtual pg num and checking if it's a hit
    switch (tlb_type)
    {
    case L1_ITLB:
    {
        hit(l1_itlb_entry_t, L1_ITLB_LINES, L1_ITLB_LINES_BITS);
    }
    break;
    case L1_DTLB:
    {
        hit(l1_dtlb_entry_t, L1_DTLB_LINES, L1_DTLB_LINES_BITS);
    }
    break;
    case L2_TLB:
    {
        hit(l2_tlb_entry_t, L2_TLB_LINES, L2_TLB_LINES_BITS);
    }
    break;
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
    if ((access != INSTRUCTION) && (access != DATA))
        return ERR_BAD_PARAMETER;

#define l1hit(acces, type, tlb_type)                                \
    if (access == acces && (tlb_hit(vaddr, paddr, type, tlb_type))) \
    {                                                               \
        *hit_or_miss = 1;                                           \
        return ERR_NONE;                                            \
    }


    #define l2hit(type, tlb_type, LINES,tlbe)\
     line_index = vpg_num % LINES; \
            type ie;\
            M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &ie, tlb_type), "while initialising instruction tlb entry");\
            M_EXIT_IF_ERR(tlb_insert(line_index, &ie, tlbe, tlb_type), "while inserting the instruction tlb entry");\
            return ERR_NONE;


    #define l2_to_l1(type, LINES, tlb_type, tlbthis, tlbother)\
      line_index = vpg_num % LINES;\
        type ientry;\
        M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &ientry, tlb_type), "while initialising tlb entry");\
        M_EXIT_IF_ERR(tlb_insert(line_index, &ientry, tlbthis, tlb_type);, "while inserting the tlb entry in L1 ");\
        if (isValid == 1 && tlbother[line_index].tag == tag)\
            tlbother[line_index].v = 0; 


    l1hit(INSTRUCTION, l1_itlb, L1_ITLB);//checking if hit in level 1 tlb
    l1hit(DATA, l1_dtlb, L1_DTLB);
    uint64_t vpg_num = virt_addr_t_to_virtual_page_number(vaddr);
    list_content_t line_index = 0;
    if (tlb_hit(vaddr, paddr, l2_tlb, L2_TLB)) // it's a hit in l2
    {
        *hit_or_miss = 1; // hit in l2, but must recopy the information in the corresponding l1 tlb

        if (access == INSTRUCTION)// getting the corresponding line index for l1 instruction AND putting the informationin l1 instruction tbl
        {
           l2hit(l1_itlb_entry_t,L1_ITLB, L1_ITLB_LINES, l1_itlb);    }
        else
        {
           l2hit(l1_dtlb_entry_t,L1_DTLB, L1_DTLB_LINES, l1_dtlb);        }
    }
    *hit_or_miss = 0; // if it's not in l1 or l2

    M_EXIT_IF_ERR(page_walk(mem_space, vaddr, paddr), "while calling page walk");
    l2_tlb_entry_t entry;
    M_EXIT_IF_ERR(tlb_entry_init(vaddr, paddr, &entry, L2_TLB), "while initialising tlb entry"); // initialise a level 2 tlb entry
    line_index = vpg_num % L2_TLB_LINES;
    int isValid = 0;
    uint32_t tag = 0;
    if (l2_tlb[line_index].v == 1) // if there was an entry before in l2
    {
        isValid = 1;
        tag = l2_tlb[line_index].tag << OFF;
        tag = tag | (line_index >> LINE_OFF);                                                                     // 32 bit tag = (30 bit tag from l2 & 2 first bits of line index of l2)
    }                                                                                                      // getting the right index in l2
    M_EXIT_IF_ERR(tlb_insert(line_index, &entry, l2_tlb, L2_TLB);, "while inserting the tlb entry in L2"); // insert it
    if (access == INSTRUCTION) // inserting the data in the tlb l1 according to the access and de-validate the other l1 tlb entry at that index if it was valid and it's tag was=l2 tag
    {
       l2_to_l1(l1_itlb_entry_t, L1_ITLB_LINES, L1_ITLB, l1_itlb, l1_dtlb) ;   }
    else
    {
      l2_to_l1(l1_dtlb_entry_t, L1_DTLB_LINES, L1_DTLB, l1_dtlb, l1_itlb) ;      }

    return ERR_NONE;
}
