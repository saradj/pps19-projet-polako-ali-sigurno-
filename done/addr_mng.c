#include "addr_mng.h"
#include <stdio.h>
#include "inttypes.h"
#include "addr.h"
#include "error.h"

#define MASK_ENTRY 0b111111111
#define MASK_OFFSET 0b111111111111
#define MASK_PAGE_NUMBER 0xFFFFF
#define MASK_49bits 0x1FFFFFFFFFFFF
#define ENTRY_SIZE 9

int print_virtual_address(FILE *where, const virt_addr_t *vaddr)
{
  M_REQUIRE_NON_NULL(where);
  int sum = fprintf(where, "PGD=0x%" PRIX16, vaddr->pgd_entry);
  sum += fprintf(where, "; PUD=0x%" PRIX16, vaddr->pud_entry);
  sum += fprintf(where, "; PMD=0x%" PRIX16, vaddr->pmd_entry);
  sum += fprintf(where, "; PTE=0x%" PRIX16, vaddr->pte_entry);
  sum += fprintf(where, "; offset=0x%" PRIX16, vaddr->page_offset);
  return sum;
}

int init_virt_addr(virt_addr_t *vaddr,
                   uint16_t pgd_entry,
                   uint16_t pud_entry, uint16_t pmd_entry,
                   uint16_t pte_entry, uint16_t page_offset)
{ //assigning the given entries to their
  //respective bitfields into virtual address vaddr
  // M_REQUIRE_NON_NULL(vaddr);
  M_REQUIRE_NON_NULL(vaddr);
  M_REQUIRE(page_offset <= MASK_OFFSET, ERR_BAD_PARAMETER, "Input value  should be 12 bits bu is %lu bits", page_offset);
  M_REQUIRE(pte_entry <= MASK_ENTRY, ERR_BAD_PARAMETER, "Input value  should be 9 bits bu is %lu bits", pte_entry);
  M_REQUIRE(pmd_entry <= MASK_ENTRY, ERR_BAD_PARAMETER, "Input value  should be 9 bits bu is %lu bits", pmd_entry);
  M_REQUIRE(pud_entry <= MASK_ENTRY, ERR_BAD_PARAMETER, "Input value  should be 9 bits bu is %lu bits", pud_entry);
  M_REQUIRE(pgd_entry <= MASK_ENTRY, ERR_BAD_PARAMETER, "Input value  should be 9 bits bu is %lu bits", pgd_entry);

  vaddr->pgd_entry = pgd_entry;
  vaddr->pud_entry = pud_entry;
  vaddr->pmd_entry = pmd_entry;
  vaddr->pte_entry = pte_entry;
  vaddr->page_offset = page_offset;
  vaddr->reserved = 0;
  return ERR_NONE;
}

int init_virt_addr64(virt_addr_t *vaddr, uint64_t vaddr64)
{
  M_REQUIRE_NON_NULL(vaddr);
  M_REQUIRE(vaddr64 <= MASK_49bits, ERR_BAD_PARAMETER, "vaddr64 is %lu - should be 49 bits long ", vaddr64);
  uint16_t page_offset = vaddr64 & MASK_OFFSET;
  uint16_t pgd = (vaddr64 >> PGD_PREV_SIZE) & MASK_ENTRY;
  uint16_t pud = (vaddr64 >> PUD_PREV_SIZE) & MASK_ENTRY;
  uint16_t pmd = (vaddr64 >> PMD_PREV_SIZE) & MASK_ENTRY;
  uint16_t pte = (vaddr64 >> PTE_PREV_SIZE) & MASK_ENTRY;
  return init_virt_addr(vaddr, pgd, pud, pmd, pte, page_offset);
}

uint64_t virt_addr_t_to_virtual_page_number(const virt_addr_t *vaddr)
{

  M_REQUIRE_NON_NULL(vaddr);
  //shifitng vaddr to the right by offset_size
  //equivalent to starting with pte entry followed by
  //pmd, pud,pgd
  uint64_t vpg_num = 0;
  vpg_num |= vaddr->pgd_entry;
  vpg_num = vpg_num << PUD_ENTRY;
  vpg_num |= vaddr->pud_entry;
  vpg_num = vpg_num << PMD_ENTRY;
  vpg_num |= vaddr->pmd_entry;
  vpg_num = vpg_num << PTE_ENTRY;
  vpg_num |= vaddr->pte_entry;

  return vpg_num;
}

uint64_t virt_addr_t_to_uint64_t(const virt_addr_t *vaddr)
{ // creating a 64 bits address from virtual addr struct
  M_REQUIRE_NON_NULL(vaddr);
  uint64_t r = virt_addr_t_to_virtual_page_number(vaddr);
  uint16_t offset = vaddr->page_offset;
  return r << PAGE_OFFSET | offset;
}

int print_physical_address(FILE *where, const phy_addr_t *paddr)
{
  M_REQUIRE_NON_NULL(where);
  int sum = fprintf(where, "page num = 0x%" PRIX32, paddr->phy_page_num);
  sum += fprintf(where, "; offset = 0x%" PRIX16, paddr->page_offset);
  return sum;
}

int init_phy_addr(phy_addr_t *paddr, uint32_t page_begin, uint32_t page_offset)
{
  M_REQUIRE_NON_NULL(paddr);
  paddr->phy_page_num = (page_begin) >> PAGE_OFFSET;
  paddr->page_offset = page_offset;
  return ERR_NONE;
}
