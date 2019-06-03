
/**
 * @file cache_mng.c
 * @brief cache management functions
 *
 * @author Mirjana Stojilovic
 * @date 2018-19
 */
#include "addr_mng.h"
#include "addr.h"
#include "error.h"
#include "util.h"
#include "cache.h"
#include "cache_mng.h"
#include "lru.h"
#include "page_walk.h"
#include <inttypes.h> // for PRIx macros

//=========================================================================
#define PRINT_CACHE_LINE(OUTFILE, TYPE, WAYS, LINE_INDEX, WAY, WORDS_PER_LINE)                 \
    do                                                                                         \
    {                                                                                          \
        fprintf(OUTFILE, "V: %1" PRIx8 ", AGE: %1" PRIx8 ", TAG: 0x%03" PRIx16 ", values: ( ", \
                cache_valid(TYPE, WAYS, LINE_INDEX, WAY),                                      \
                cache_age(TYPE, WAYS, LINE_INDEX, WAY),                                        \
                cache_tag(TYPE, WAYS, LINE_INDEX, WAY));                                       \
        for (int i_ = 0; i_ < WORDS_PER_LINE; i_++)                                            \
            fprintf(OUTFILE, "0x%08" PRIx32 " ",                                               \
                    cache_line(TYPE, WAYS, LINE_INDEX, WAY)[i_]);                              \
        fputs(")\n", OUTFILE);                                                                 \
    } while (0)

#define PRINT_INVALID_CACHE_LINE(OUTFILE, TYPE, WAYS, LINE_INDEX, WAY, WORDS_PER_LINE)                                    \
    do                                                                                                                    \
    {                                                                                                                     \
        fprintf(OUTFILE, "V: %1" PRIx8 ", AGE: -, TAG: -----, values: ( ---------- ---------- ---------- ---------- )\n", \
                cache_valid(TYPE, WAYS, LINE_INDEX, WAY));                                                                \
    } while (0)

#define DUMP_CACHE_TYPE(OUTFILE, TYPE, WAYS, LINES, WORDS_PER_LINE)                                  \
    do                                                                                               \
    {                                                                                                \
        for (uint16_t index = 0; index < LINES; index++)                                             \
        {                                                                                            \
            foreach_way(way, WAYS)                                                                   \
            {                                                                                        \
                fprintf(output, "%02" PRIx8 "/%04" PRIx16 ": ", way, index);                         \
                if (cache_valid(TYPE, WAYS, index, way))                                             \
                    PRINT_CACHE_LINE(OUTFILE, const TYPE, WAYS, index, way, WORDS_PER_LINE);         \
                else                                                                                 \
                    PRINT_INVALID_CACHE_LINE(OUTFILE, const TYPE, WAYS, index, way, WORDS_PER_LINE); \
            }                                                                                        \
        }                                                                                            \
    } while (0)

//=========================================================================
// see cache_mng.h
int cache_dump(FILE *output, const void *cache, cache_t cache_type)
{
    M_REQUIRE_NON_NULL(output);
    M_REQUIRE_NON_NULL(cache);

    fputs("WAY/LINE: V: AGE: TAG: WORDS\n", output);
    switch (cache_type)
    {
    case L1_ICACHE:
        DUMP_CACHE_TYPE(output, l1_icache_entry_t, L1_ICACHE_WAYS,
                        L1_ICACHE_LINES, L1_ICACHE_WORDS_PER_LINE);
        break;
    case L1_DCACHE:
        DUMP_CACHE_TYPE(output, l1_dcache_entry_t, L1_DCACHE_WAYS,
                        L1_DCACHE_LINES, L1_DCACHE_WORDS_PER_LINE);
        break;
    case L2_CACHE:
        DUMP_CACHE_TYPE(output, l2_cache_entry_t, L2_CACHE_WAYS,
                        L2_CACHE_LINES, L2_CACHE_WORDS_PER_LINE);
        break;
    default:
        debug_print("%d: unknown cache type", cache_type);
        return ERR_BAD_PARAMETER;
    }
    putc('\n', output);

    return ERR_NONE;
}

#define SEL_BYTE 2
#define MASK_TWO_BITS 0b11
#define MASK_THREE_BITS 0b111
#define WORDS_PER_LINE L1_DCACHE_WORDS_PER_LINE
#define BYTE_MASK 0b11111111
#define BITS_IN_BYTE 8
#define MASK_LINE_INDEX_L1 0b111111
#define LINE_INDEX_L1_BITS 6
static inline uint32_t getPhaddr(const phy_addr_t *paddr) //helper method to get Physical address as 32 uint
{
    return ((paddr->phy_page_num << PAGE_OFFSET) | paddr->page_offset);
}
int cache_entry_init(const void *mem_space,
                     const phy_addr_t *paddr,
                     void *cache_entry,
                     cache_t cache_type)
{

#define init(type, nbBits, LINES, WORDS_PER_LINE)  \
    ((type *)cache_entry)->v = 1;                  \
    uint32_t phaddr = getPhaddr(paddr);            \
    ((type *)cache_entry)->tag = phaddr >> nbBits; \
    ((type *)cache_entry)->age = 0;                \
    uint32_t off = phaddr - (phaddr % LINES);      \
    uint32_t index = off / WORDS_PER_LINE;         \
    for (size_t i = 0; i < WORDS_PER_LINE; ++i)    \
        ((type *)cache_entry)->line[i] = *((word_t *)mem_space + index + i);
    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(cache_entry);
    switch (cache_type)
    {
    case L1_ICACHE:
    {
        init(l1_icache_entry_t, L1_ICACHE_TAG_REMAINING_BITS, L1_ICACHE_LINE, L1_ICACHE_WORDS_PER_LINE);
    }
    break;
    case L1_DCACHE:
    {
        init(l1_dcache_entry_t, L1_DCACHE_TAG_REMAINING_BITS, L1_DCACHE_LINE, L1_DCACHE_WORDS_PER_LINE);
    }
    break;
    case L2_CACHE:
    {
        init(l2_cache_entry_t, L2_CACHE_TAG_REMAINING_BITS, L2_CACHE_LINE, L2_CACHE_WORDS_PER_LINE);
    }
    break;
    default:
        return ERR_BAD_PARAMETER;
    }
    return ERR_NONE;
}

int cache_flush(void *cache, cache_t cache_type)
{
#define flush(type, nbLines)                                          \
    for (size_t i = 0; i < nbLines; i++)                              \
    {                                                                 \
        memset(&(((type *)cache)[i]), 0, sizeof(((type *)cache)[i])); \
    }

    M_REQUIRE_NON_NULL(cache);
    switch (cache_type)
    {
    case L1_ICACHE:
    {
        flush(l1_icache_entry_t, L1_ICACHE_LINES * L1_ICACHE_WAYS);
    }
    break;
    case L1_DCACHE:
    {
        flush(l1_dcache_entry_t, L1_DCACHE_LINES * L1_DCACHE_WAYS);
    }
    break;
    case L2_CACHE:
    {
        flush(l2_cache_entry_t, L2_CACHE_LINES * L2_CACHE_WAYS);
    }
    break;
    default:
        return ERR_BAD_PARAMETER;
    }

    return ERR_NONE;
}

int cache_insert(uint16_t cache_line_index,
                 uint8_t cache_way,
                 const void *cache_line_in,
                 void *cache,
                 cache_t cache_type)
{

    M_REQUIRE_NON_NULL(cache_line_in);
    M_REQUIRE_NON_NULL(cache);
#define insert(TYPE, LINES, WAYS)                                                                                               \
    M_REQUIRE(cache_line_index < LINES, ERR_BAD_PARAMETER, "cache line index %z bigger than max  line size", cache_line_index); \
    M_REQUIRE(cache_way < WAYS, ERR_BAD_PARAMETER, "cache way %z bigger than max nb of ways", cache_way);                       \
    *((TYPE *)cache_entry(TYPE, WAYS, cache_line_index, cache_way)) = *((TYPE *)cache_line_in);

    switch (cache_type)
    {
    case L1_ICACHE:
    {
        insert(l1_icache_entry_t, L1_ICACHE_LINES, L1_ICACHE_WAYS);
    }
    break;
    case L1_DCACHE:
    {
        insert(l1_dcache_entry_t, L1_DCACHE_LINES, L1_DCACHE_WAYS);
    }
    break;
    case L2_CACHE:
    {
        insert(l2_cache_entry_t, L2_CACHE_LINES, L2_CACHE_WAYS);
    }
    break;
    default:
        return ERR_BAD_PARAMETER;
    }
    return ERR_NONE;
}

int cache_hit(const void *mem_space,
              void *cache,
              phy_addr_t *paddr,
              const uint32_t **p_line,
              uint8_t *hit_way,
              uint16_t *hit_index,
              cache_t cache_type)
{
    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(cache);
    M_REQUIRE_NON_NULL(p_line);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(hit_index);
    M_REQUIRE_NON_NULL(hit_way);

    uint32_t phaddr = getPhaddr(paddr);
#define hit(TYPE, WORDS_PER_LINE, LINES, REMAINING_BITS, WAYS)                                        \
    uint32_t index = (phaddr / (WORDS_PER_LINE * sizeof(word_t))) % LINES;                            \
    uint32_t tag = phaddr >> REMAINING_BITS;                                                          \
    foreach_way(way, WAYS)                                                                            \
    {                                                                                                 \
        if ((cache_valid(TYPE, WAYS, index, way) == 1) && (cache_tag(TYPE, WAYS, index, way) == tag)) \
        {                                                                                             \
            *hit_way = way;                                                                           \
            *hit_index = index;                                                                       \
            *p_line = cache_line(TYPE, WAYS, index, way);                                             \
            LRU_age_update(TYPE, WAYS, way, index);                                                   \
            return ERR_NONE;                                                                          \
        }                                                                                             \
    }                                                                                                 \
    *hit_way = HIT_WAY_MISS;                                                                          \
    *hit_index = HIT_INDEX_MISS;                                                                      \
    return ERR_NONE;
    //calculating the index and the tag in the corresponding cache and checing if an entry in one of the ways or the line index if valid and has the right tag
    switch (cache_type)
    {
    case L1_ICACHE:
    {
        hit(l1_icache_entry_t, L1_ICACHE_WORDS_PER_LINE, L1_ICACHE_LINES, L1_ICACHE_TAG_REMAINING_BITS, L1_ICACHE_WAYS);
    }
    break;
    case L1_DCACHE:
    {
        hit(l1_dcache_entry_t, L1_DCACHE_WORDS_PER_LINE, L1_DCACHE_LINES, L1_DCACHE_TAG_REMAINING_BITS, L1_DCACHE_WAYS);
    }
    break;
    case L2_CACHE:
    {
        hit(l2_cache_entry_t, L2_CACHE_WORDS_PER_LINE, L2_CACHE_LINES, L2_CACHE_TAG_REMAINING_BITS, L2_CACHE_WAYS);
    }
    break;
    default:
        return ERR_BAD_PARAMETER;
    }
    return ERR_NONE;
}

#define initLine(lineTo, lineFrom)              \
    for (size_t i = 0; i < WORDS_PER_LINE; ++i) \
        lineTo[i] = lineFrom[i];

#define initEntry(type, ENTRY, lineFrom, TAG) \
    ((type *)ENTRY)->v = 1;                   \
    ((type *)ENTRY)->age = 0;                 \
    ((type *)ENTRY)->tag = TAG;               \
    initLine(((type *)ENTRY)->line, lineFrom);
#define l1_search(TYPE)                                                                                                               \
    M_EXIT_IF_ERR(cache_hit(mem_space, l1_cache, paddr, &p_line, &hit_way, &hit_index, TYPE), "calling cache hit on l1 instruction"); \
    if (hit_way != HIT_WAY_MISS)                                                                                                      \
        *word = p_line[word_index];                                                                                                   \
    return ERR_NONE;

#define findPlace(CACHE_TYPE, TYPE, WAYS)                                                                 \
    foreach_way(way, WAYS) {if (cache_valid(TYPE, WAYS, hit_index, way) == 0)                              \
    {                                                                                                     \
        M_EXIT_IF_ERR(cache_insert(hit_index, way, entry_toinsert, cache, CACHE_TYPE), "calling insert"); \
        LRU_age_increase(TYPE, WAYS, way, hit_index);                                                 \
        return ERR_NONE;                                                                                  \
    }}

#define evict(TYPE, WAYS, CACHE_TYPE)                                                                    \
    max_age = 0;                                                                                         \
    foreach_way(way, WAYS)                                                                               \
    {                                                                                                    \
        if (cache_age(TYPE, WAYS, hit_index, way) >= max_age)                                            \
        {                                                                                                \
            way_to = way;                                                                                \
            max_age = cache_age(TYPE, WAYS, hit_index, way);                                             \
        }                                                                                                \
    }                                                                                                    \
    evicted = cache_line(TYPE, WAYS, hit_index, way_to);                                                 \
    l1tag = cache_tag(TYPE, CACHE_TYPE, hit_index, way_to);                                              \
    M_EXIT_IF_ERR(cache_insert(hit_index, way_to, entry_toinsert, cache, CACHE_TYPE), "CALLING insert"); \
    LRU_age_update(TYPE, WAYS, way_to, hit_index);
// END OF EVICT

#define moveL1_to_L2(L1TYPE, WAYS, type, LINES)                                                              \
    uint8_t max_age = 0;                                                                                     \
    uint8_t way_to = 0;                                                                                      \
    uint32_t l1tag = 0;                                                                                      \
    const word_t *evicted = NULL;                                                                            \
    cache = l1_cache;                                                                                        \
    hit_index = saved_index & MASK_LINE_INDEX_L1;                                                            \
    evict(type, WAYS, L1TYPE);                                                                               \
    cache = l2_cache;                                                                                        \
    initEntry(l2_cache_entry_t, entry_toinsert, evicted, l1tag >> (L1_ICACHE_TAG_BITS - L2_CACHE_TAG_BITS)); \
    hit_index = ((l1tag & MASK_THREE_BITS) << LINE_INDEX_L1_BITS) | hit_index;                               \
    findPlace(L2_CACHE, l2_cache_entry_t, L2_CACHE_WAYS);                                                    \
    evict(l2_cache_entry_t, L2_CACHE_WAYS, L2_CACHE);                                                        \
    return ERR_NONE;

#define moveL2_to_L1(L1TYPE, WAYS, REMAINING_BITS, type, LINES)                                                                                                         \
    cache_valid(l2_cache_entry_t, L2_CACHE_WAYS, hit_index, hit_way) = 0;                                                                                               \
    uint32_t newtag = (cache_tag(l2_cache_entry_t, L2_CACHE_WAYS, hit_index, hit_way) << (L1_ICACHE_TAG_BITS - L2_CACHE_TAG_BITS)) | (hit_index >> LINE_INDEX_L1_BITS); \
    initEntry(type, entry_toinsert, cache_line(l2_cache_entry_t, L2_CACHE_WAYS, hit_index, hit_way), newtag);                                                           \
    hit_index = saved_index & MASK_LINE_INDEX_L1;                                                                                                                       \
    cache = l1_cache;                                                                                                                                                   \
    findPlace(L1TYPE, type, WAYS);                                                                                                                                      \
    moveL1_to_L2(L1TYPE, WAYS, type, LINES)

#define afterl2Hit                                                                                                 \
    saved_index = hit_index;                                                                                       \
    cache = l2_cache;                                                                                              \
    *word = p_line[word_index];                                                                                    \
    if (access == INSTRUCTION)                                                                                     \
    {                                                                                                              \
        moveL2_to_L1(L1_ICACHE, L1_ICACHE_WAYS, L1_ICACHE_TAG_REMAINING_BITS, l1_icache_entry_t, L1_ICACHE_LINES); \
    }                                                                                                              \
    else                                                                                                           \
    {                                                                                                              \
        moveL2_to_L1(L1_DCACHE, L1_DCACHE_WAYS, L1_DCACHE_TAG_REMAINING_BITS, l1_dcache_entry_t, L1_DCACHE_LINES); \
    }

#define miss_in_both(CACHE_TYPE, LINE, LINES, type, WAYS)                                                            \
    M_EXIT_IF_ERR(cache_entry_init(mem_space, paddr, entry_toinsert, CACHE_TYPE), "while initialising cache entry"); \
    *word = entry_toinsert->line[word_index];                                                                        \
    hit_index = (phaddr / (LINE)) % LINES;                                                                           \
    saved_index = hit_index;                                                                                         \
    cache = l1_cache;                                                                                                \
    findPlace(CACHE_TYPE, type, WAYS);                                                                               \
    moveL1_to_L2(CACHE_TYPE, WAYS, type, L1_DCACHE_LINES);

int cache_read(const void *mem_space,
               phy_addr_t *paddr,
               mem_access_t access,
               void *l1_cache,
               void *l2_cache,
               uint32_t *word,
               cache_replace_t replace)
{

    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(l1_cache);
    M_REQUIRE_NON_NULL(l2_cache);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(word);
    uint32_t phaddr = getPhaddr(paddr);                                                                           //get the physical address
    M_REQUIRE((phaddr % WORDS_PER_LINE) == 0, ERR_BAD_PARAMETER, "physical address %d not word aligned", phaddr); // CHECK IF LAST 2 BITS == 0
    const word_t *p_line = NULL;
    uint8_t hit_way = 0;
    uint16_t saved_index, hit_index = 0;
    uint8_t word_index = (phaddr >> SEL_BYTE) % WORDS_PER_LINE; //getting the index of the word
    void *cache = l1_cache;
    switch (access)
    {
    case INSTRUCTION:
    {
        l1_icache_entry_t l1i;
        l1_icache_entry_t *entry_toinsert = &l1i;
        l1_search(L1_ICACHE); // search in l1 i cache, eighter returns ERR_NONE OR is was a miss
        M_EXIT_IF_ERR(cache_hit(mem_space, l2_cache, paddr, &p_line, &hit_way, &hit_index, L2_CACHE), "calling cache hit on l2");
        if (hit_way != HIT_WAY_MISS) //if it was a hit in l2
        {
            afterl2Hit;
        }
        miss_in_both(L1_ICACHE, L1_ICACHE_LINE, L1_ICACHE_LINES, l1_icache_entry_t, L1_ICACHE_WAYS);
    }
    break;
    case DATA:
    {
        l1_dcache_entry_t l1d;
        l1_dcache_entry_t *entry_toinsert = &l1d;
        l1_search(L1_DCACHE);
        M_EXIT_IF_ERR(cache_hit(mem_space, l2_cache, paddr, &p_line, &hit_way, &hit_index, L2_CACHE), "calling cache hit on l1 instruction");
        if (hit_way != HIT_WAY_MISS)
        {
            afterl2Hit;
        }

        miss_in_both(L1_DCACHE, L1_DCACHE_LINE, L1_DCACHE_LINES, l1_dcache_entry_t, L1_DCACHE_WAYS);
    }
    break;
    default:
        return ERR_BAD_PARAMETER;
    }
    return ERR_NONE;
}

int cache_read_byte(const void *mem_space,
                    phy_addr_t *p_paddr,
                    mem_access_t access,
                    void *l1_cache,
                    void *l2_cache,
                    uint8_t *p_byte,
                    cache_replace_t replace)
{
    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(l1_cache);
    M_REQUIRE_NON_NULL(l2_cache);
    M_REQUIRE_NON_NULL(p_paddr);
    M_REQUIRE_NON_NULL(p_byte);
    uint32_t phaddr = getPhaddr(p_paddr);
    uint8_t byteIndex = phaddr % sizeof(word_t); // gives the last 2 bits
    word_t word = 0;
    p_paddr->page_offset -= (p_paddr->page_offset % WORDS_PER_LINE); //making the physica address word aligned
    M_EXIT_IF_ERR(cache_read(mem_space, p_paddr, access, l1_cache, l2_cache, &word, replace), "calling cache read");
    *p_byte = (word >> ( byteIndex) * BITS_IN_BYTE)& BYTE_MASK; //little endian, lsb byte in word is index 0
    return ERR_NONE;
}

int cache_write(void *mem_space,
                phy_addr_t *paddr,
                void *l1_cache,
                void *l2_cache,
                const uint32_t *word,
                cache_replace_t replace)
{
    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(l1_cache);
    M_REQUIRE_NON_NULL(l2_cache);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(word);
    uint32_t phaddr = getPhaddr(paddr);
    M_REQUIRE((phaddr % WORDS_PER_LINE) == 0, ERR_BAD_PARAMETER, "physical address %d not word aligned", phaddr);
    uint32_t *p_line = NULL;
    uint8_t hit_way = 0;
    uint16_t saved_index, hit_index = 0;
    uint32_t word_index = (phaddr >> SEL_BYTE) & MASK_TWO_BITS;
#define read_mod_ins(cachet, CACHE_TYPE, WAYS, type, WORDS_PER_LINE, REMAINING_BITS, LINES)                             \
    M_EXIT_IF_ERR(cache_hit(mem_space, cachet, paddr, &p_line, &hit_way, &hit_index, CACHE_TYPE), "while calling hit"); \
    if (hit_way != HIT_WAY_MISS)                                                                                        \
    {                                                                                                                   \
        saved_index = hit_index;                                                                                        \
        p_line[word_index] = *word;                                                                                     \
        initEntry(type, entry_toinsert, p_line, phaddr >> REMAINING_BITS);                                              \
        cache_insert(hit_index, hit_way, entry_toinsert, cachet, CACHE_TYPE);                                           \
        void *cache = cachet;                                                                                           \
        LRU_age_update(type, WAYS, hit_way, hit_index);                                                                 \
        off = phaddr - (phaddr % LINES);                                                                                \
        index = off / WORDS_PER_LINE;                                                                                   \
        for (size_t i = 0; i < WORDS_PER_LINE; ++i)                                                                     \
            *((word_t *)mem_space + index + i) = p_line[i];                                                             \
        if (CACHE_TYPE == L2_CACHE)       {   \
            entry_toinsert=&l1d;                                                                          \
            moveL2_to_L1(L1_DCACHE, L1_DCACHE_WAYS, L1_DCACHE_TAG_REMAINING_BITS, l1_dcache_entry_t, L1_DCACHE_LINES);  \
          }  return ERR_NONE;                                                                                                \
    }

    uint32_t off, index;
    void *entry_toinsert;
    l1_dcache_entry_t l1d;
    entry_toinsert = &l1d;
    read_mod_ins(l1_cache, L1_DCACHE, L1_DCACHE_WAYS, l1_dcache_entry_t, L1_DCACHE_WORDS_PER_LINE, L1_DCACHE_TAG_REMAINING_BITS, L1_DCACHE_LINES);
    l2_cache_entry_t l2;
    entry_toinsert = &l2;
    read_mod_ins(l2_cache, L2_CACHE, L2_CACHE_WAYS, l2_cache_entry_t, L2_CACHE_WORDS_PER_LINE, L2_CACHE_TAG_REMAINING_BITS, L2_CACHE_LINES);
    entry_toinsert = &l1d;//alrady done in red_mod_ins but just to make it clean that it points to the right type entry
    M_EXIT_IF_ERR(cache_entry_init(mem_space, paddr, entry_toinsert, L1_DCACHE), "while calling cache entry init");
    off = phaddr - (phaddr % L1_DCACHE_LINES);
    index = off / L1_DCACHE_WORDS_PER_LINE; //recalculate the index in main memory
    
  (( l1_dcache_entry_t *) entry_toinsert)->line[word_index] = *word;
    for (size_t i = 0; i < L1_DCACHE_WORDS_PER_LINE; ++i)
        *((word_t *)mem_space + index + i) =  (( l1_dcache_entry_t *) entry_toinsert)->line[i];
   void *cache = l1_cache;
    hit_index = (phaddr / (L1_DCACHE_LINE)) % L1_DCACHE_LINES;
    saved_index=hit_index;
    findPlace(L1_DCACHE, l1_dcache_entry_t, L1_DCACHE_WAYS)
        moveL1_to_L2(L1_DCACHE, L1_DCACHE_WAYS, l1_dcache_entry_t, L1_DCACHE_LINES);
    return ERR_NONE;
}

int cache_write_byte(void *mem_space,
                     phy_addr_t *paddr,
                     void *l1_cache,
                     void *l2_cache,
                     uint8_t p_byte,
                     cache_replace_t replace)
{

    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(l1_cache);
    M_REQUIRE_NON_NULL(l2_cache);
    M_REQUIRE_NON_NULL(paddr);
    uint32_t phaddr = getPhaddr(paddr);
    uint8_t byteIndex = phaddr % sizeof(word_t); // gives the last 2 bits
    word_t word = 0;
   paddr->page_offset -= (paddr->page_offset % WORDS_PER_LINE); //making the physica address word aligned
    M_EXIT_IF_ERR(cache_read(mem_space, paddr, DATA, l1_cache, l2_cache, &word, replace), "CALLING read");
    uint32_t mask = (BYTE_MASK << ( byteIndex * BITS_IN_BYTE)); //little endian 
    mask = ~mask;
    uint32_t modified = p_byte << (byteIndex * BITS_IN_BYTE);
    word = (word & mask) | modified;
    M_EXIT_IF_ERR(cache_write(mem_space, paddr, l1_cache, l2_cache, &word, replace), "CALLING WRITE");
    return ERR_NONE;
}