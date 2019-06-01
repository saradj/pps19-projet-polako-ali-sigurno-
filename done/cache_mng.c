
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

static inline uint32_t getPhaddr(const phy_addr_t *paddr)
{
    return ((paddr->phy_page_num << PAGE_OFFSET) | paddr->page_offset);
}
int cache_entry_init(const void *mem_space,
                     const phy_addr_t *paddr,
                     void *cache_entry,
                     cache_t cache_type)
{

#define init(type, nbBits, LINES)                               \
    ((type *)cache_entry)->v = 1;                               \
    uint32_t phaddr = getPhaddr(paddr);                         \
    ((type *)cache_entry)->tag = phaddr >> nbBits;              \
    ((type *)cache_entry)->age = 0;                             \
    uint32_t off = phaddr % LINES;                              \
    uint32_t index = ((phaddr >> off) << off) / sizeof(word_t); \
    for (size_t i = 0; i < 4; ++i)                              \
        ((type *)cache_entry)->line[index + i] = ((word_t *)mem_space)[phaddr + i];

    M_REQUIRE_NON_NULL(mem_space);
    M_REQUIRE_NON_NULL(paddr);
    M_REQUIRE_NON_NULL(cache_entry);
    switch (cache_type)
    {
    case L1_ICACHE:
    {
        init(l1_icache_entry_t, L1_ICACHE_TAG_REMAINING_BITS, L1_ICACHE_LINE);
    }
    break;
    case L1_DCACHE:
    {
        init(l1_dcache_entry_t, L1_DCACHE_TAG_REMAINING_BITS, L1_DCACHE_LINE);
    }
    break;
    case L2_CACHE:
    {
        init(l2_cache_entry_t, L2_CACHE_TAG_REMAINING_BITS, L2_CACHE_LINE);
    }
    break;
    }
    return ERR_NONE;
}

int cache_flush(void *cache, cache_t cache_type)
{
    M_REQUIRE_NON_NULL(cache);

#define flush(type, nbLines)               \
    for (size_t i = 0; i < nbLines; i++)   \
    {                                      \
        zero_init_var(((type *)cache)[i]); \
    }

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
#define insrt(TYPE, LINES, WAYS)                                                                                                \
    M_REQUIRE(cache_line_index < LINES, ERR_BAD_PARAMETER, "cache line index %z bigger than max  line size", cache_line_index); \
    M_REQUIRE(cache_way < WAYS, ERR_BAD_PARAMETER, "cache way %z bigger than max nb of ways", cache_way);                       \
    *((TYPE *)cache_entry(TYPE, WAYS, cache_line_index, cache_way)) = *((TYPE *)cache_line_in);

    switch (cache_type)
    {
    case L1_ICACHE:
    {
        insrt(l1_icache_entry_t, L1_ICACHE_LINES, L1_ICACHE_WAYS);
    }
    break;
    case L1_DCACHE:
    {
        insrt(l1_dcache_entry_t, L1_DCACHE_LINES, L1_DCACHE_WAYS);
    }
    break;
    case L2_CACHE:
    {
        insrt(l2_cache_entry_t, L2_CACHE_LINES, L2_CACHE_WAYS);
    }
    break;
    }
    return ERR_NONE;
}
//or nbLines*cache_way+cache_line_index

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
    uint8_t coldStart = 0;
    uint32_t phaddr = getPhaddr(paddr);
#define hit(TYPE, WORDS_PER_LINE, LINES, REMAINING_BITS, WAYS)             \
    uint32_t index = (phaddr / (WORDS_PER_LINE * sizeof(word_t))) % LINES; \
    uint32_t tag = phaddr >> REMAINING_BITS;                               \
    foreach_way(way, WAYS)                                                 \
    {                                                                      \
        if (cache_valid(TYPE, WAYS, index, way) == 0)                      \
        {                                                                  \
            coldStart = 1;                                                 \
        }                                                                  \
        else if (cache_tag(TYPE, WAYS, index, way) == tag)                 \
        {                                                                  \
            *hit_way = way;                                                \
            *hit_index = index;                                            \
            *p_line = cache_line(TYPE, WAYS, index, way);                  \
            uint8_t age = cache_age(TYPE, WAYS, index, way);               \
            LRU_age_update(TYPE, WAYS, way, index);                        \
            return ERR_NONE;                                               \
        }                                                                  \
        *hit_way = HIT_WAY_MISS;                                           \
        *hit_index = HIT_INDEX_MISS;                                       \
        return ERR_NONE;                                                   \
    }

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
    }
    return ERR_NONE;
}

#define l1_search(TYPE)                                                                                                               \
    M_EXIT_IF_ERR(cache_hit(mem_space, l1_cache, paddr, &p_line, &hit_way, &hit_index, TYPE), "calling cache hit on l1 instruction"); \
    if (hit_way != HIT_WAY_MISS)                                                                                                      \
        *word = p_line[word_index];                                                                                                   \
    return ERR_NONE;

#define findPlace(CACHE_TYPE, TYPE, WAYS)                                    \
    foreach_way(way, WAYS) if (cache_valid(TYPE, WAYS, hit_index, way) == 0) \
    {                                                                        \
        cache_insert(hit_index, way, line_toinsert, cache, CACHE_TYPE);      \
        LRU_age_increase(TYPE, WAYS, hit_way, hit_index);                    \
        return ERR_NONE;                                                     \
    }

#define evict(TYPE, WAYS, CACHE_TYPE)                                  \
    foreach_way(way, WAYS)                                             \
    {                                                                  \
        uint8_t age = cache_age(TYPE, WAYS, hit_index, way);           \
        if (age >= max_age)                                            \
        {                                                              \
            way_to = way;                                              \
            max_age = age;                                             \
            evicted = cache_line(TYPE, WAYS, hit_index, way);          \
        }                                                              \
    }                                                                  \
    cache_insert(hit_index, way_to, line_toinsert, cache, CACHE_TYPE); \
    LRU_age_update(TYPE, WAYS, way_to, hit_index);
// END OF EVICT

#define moveL1_to_L2(L1TYPE, WAYS)                        \
    uint8_t max_age = 0;                                  \
    uint8_t way_to = 0;                                   \
    const uint32_t *evicted = NULL;                       \
    evict(l1_icache_entry_t, WAYS, L1TYPE);               \
    line_toinsert = evicted;                              \
    cache = l2_cache;                                     \
    findPlace(L2_CACHE, l2_cache_entry_t, L2_CACHE_WAYS); \
    evict(l2_cache_entry_t, L2_CACHE_WAYS, L2_CACHE);

#define moveL2_to_L1(L1TYPE, WAYS, REMAINING_BITS)                                                       \
        ((l2_cache_entry_t *)l2_cache + (hit_index) * (L2_CACHE_WAYS) + (hit_way))->v =0; \
    uint32_t tag = phaddr >> REMAINING_BITS;                                                                  \
                                                                                       \
    const uint32_t *line_toinsert = p_line;                                                                   \
    findPlace(L1TYPE, l1_icache_entry_t, WAYS);                                                               \
    moveL1_to_L2(L1TYPE, WAYS)

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
    uint32_t phaddr = getPhaddr(paddr);
    M_REQUIRE((phaddr << 30) == 0, ERR_BAD_PARAMETER, "physical address %d not word aligned", phaddr);
    const uint32_t *p_line = NULL;
    uint8_t hit_way = 0;
    uint16_t hit_index = 0;
    uint32_t word_index = (phaddr << 28) >> 30;
 void * cache = l1_cache;
    switch (access)
    {
    case INSTRUCTION:
    {
        l1_search(L1_ICACHE); // eighter returns ERR_NONE OR is was a miss
        M_EXIT_IF_ERR(cache_hit(mem_space, l2_cache, paddr, &p_line, &hit_way, &hit_index, L2_CACHE), "calling cache hit on l1 instruction");
        if (hit_way != HIT_WAY_MISS)
        {
           
           moveL2_to_L1(L1_ICACHE, L1_ICACHE_WAYS, L1_ICACHE_TAG_REMAINING_BITS);      }
    }
    break;
    case DATA:
    {
        l1_search(L1_DCACHE);
    M_EXIT_IF_ERR(cache_hit(mem_space, l2_cache, paddr, &p_line, &hit_way, &hit_index, L2_CACHE), "calling cache hit on l1 instruction");
        if (hit_way != HIT_WAY_MISS)
        {
           moveL2_to_L1(L1_DCACHE, L1_DCACHE_WAYS, L1_DCACHE_TAG_REMAINING_BITS);      }
    }
    break;
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
    uint8_t byteIndex = phaddr & 0x00000003;
    word_t word = 0;
    phy_addr_t paddr;
    paddr.phy_page_num = p_paddr->phy_page_num;
    paddr.page_offset = ((p_paddr->page_offset >> 2) << 2);
    cache_read(mem_space, &paddr, access, l1_cache, l2_cache, &word, replace);
    *p_byte = (word >> byteIndex) & 0x3;
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
    M_REQUIRE((phaddr << 30) == 0, ERR_BAD_PARAMETER, "physical address %d not word aligned", phaddr);
     uint32_t *p_line = NULL;
    uint8_t hit_way = 0;
    uint16_t hit_index = 0;
    uint32_t word_index = (phaddr << 28) >> 30;
#define read_mod_ins(cachet, CACHE_TYPE, WAYS, type, WORDS_PER_LINE,REMAINING_BITS)                                                    \
    M_EXIT_IF_ERR(cache_hit(mem_space, cachet, paddr, &p_line, &hit_way, &hit_index, CACHE_TYPE), "while calling hit"); \
    if (hit_way != HIT_WAY_MISS)                                                                                       \
    {                                                                                                                  \
        p_line[word_index] = *word;                                                                                    \
        cache_insert(hit_index, hit_way, p_line, cachet, CACHE_TYPE);  \
        void * cache = cachet;                                                 \
        LRU_age_update(type, WAYS, hit_way, hit_index);                                                                \
        for (size_t i = 0; i < WORDS_PER_LINE; ++i)                                                                    \
            ((word_t *)mem_space)[phaddr + i] = p_line[i];                                                             \
        if (CACHE_TYPE == L2_CACHE)                                                                                    \
             moveL2_to_L1(CACHE_TYPE, WAYS, REMAINING_BITS);  \
             return ERR_NONE;    \
    }

    read_mod_ins(l1_cache, L1_DCACHE, L1_DCACHE_WAYS, l1_dcache_entry_t, L1_DCACHE_WORDS_PER_LINE, L1_DCACHE_TAG_REMAINING_BITS);
    read_mod_ins(l2_cache, L2_CACHE, L2_CACHE_WAYS, l2_cache_entry_t, L2_CACHE_WORDS_PER_LINE, L1_DCACHE_TAG_REMAINING_BITS);
    l1_dcache_entry_t data_entry;
    cache_entry_init(mem_space, paddr, &data_entry, L1_DCACHE);

    data_entry.line[word_index] = *word;
    for (size_t i = 0; i < L1_DCACHE_WORDS_PER_LINE; ++i)
        ((word_t *)mem_space)[phaddr + i] = data_entry.line[i];

    const uint32_t *line_toinsert = data_entry.line;
    void * cache = l1_cache;
    findPlace(L1_DCACHE, l1_dcache_entry_t, L1_DCACHE_WAYS)
    moveL1_to_L2(L1_DCACHE, L1_DCACHE_WAYS);
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
    uint8_t byteIndex = phaddr & 0x00000003;
    word_t word = 0;
    phy_addr_t p_paddr;
    p_paddr.phy_page_num = paddr->phy_page_num;
    p_paddr.page_offset = ((paddr->page_offset >> 2) << 2);
    M_EXIT_IF_ERR( cache_read(mem_space, &p_paddr, DATA, l1_cache, l2_cache, &word, replace), "CALLING read") ;
    uint32_t mask = ~(0x3<<byteIndex);
    uint32_t modified = p_byte<<byteIndex;
    word = (word & mask)|modified;
    M_EXIT_IF_ERR(cache_write(mem_space, &p_paddr,l1_cache,l2_cache, &word, replace), "CALLING WRITE") ;
    return ERR_NONE;
}