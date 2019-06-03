#include "cache.h"
#include "cache_mng.h"
#define LRU_age_increase(TYPE, WAYS, WAY_INDEX, LINE_INDEX)          \
                                                                     \
    foreach_way(way, WAYS)                                           \
    {                                                                \
        if (way != WAY_INDEX)                                        \
        {                                                            \
                                                                     \
            if (cache_age(TYPE, WAYS, LINE_INDEX, way) < (WAYS - 1)) \
                cache_age(TYPE, WAYS, LINE_INDEX, way) += 1;         \
        }                                                            \
        else                                                         \
            cache_age(TYPE, WAYS, LINE_INDEX, way) = 0;              \
    }

#define LRU_age_update(TYPE, WAYS, WAY_INDEX, LINE_INDEX)                                                \
                                                                                                         \
    foreach_way(way, WAYS)                                                                               \
    {                                                                                                    \
        if (way != WAY_INDEX)                                                                            \
        {                                                                                                \
                                                                                                         \
            if (( cache_age(TYPE, WAYS, LINE_INDEX, way) < (WAYS - 1)) && (cache_age(TYPE, WAYS, LINE_INDEX, way) < cache_age(TYPE, WAYS, LINE_INDEX, WAY_INDEX))) \
                cache_age(TYPE, WAYS, LINE_INDEX, way) += 1;                                             \
        }                                                                                                \
        else                                                                                             \
            cache_age(TYPE, WAYS, LINE_INDEX, way) = 0;                                                  \
    }
