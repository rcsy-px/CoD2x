#pragma once
#include <string.h>

// zPAM v7 copied these stock material binaries under aliased filenames without
// changing their embedded names. Registering the alias every loading frame
// misses the renderer name cache and allocates another material each time.
// Keep true custom map names intact: a generic _fix/_tls suffix removal is wrong.
inline const char* loading_map_material_name(const char* map) {
    struct Alias { const char* map; const char* canonical; };
    static const Alias aliases[] = {
        {"mp_burgundy_fix", "mp_burgundy"},
        {"mp_carentan_bal", "mp_carentan"},
        {"mp_carentan_fix", "mp_carentan"},
        {"mp_dawnville_fix", "mp_dawnville"},
        {"mp_matmata_fix", "mp_matmata"},
        {"mp_toujane_fix", "mp_toujane"},
        {"mp_trainstation_fix", "mp_trainstation"},
    };
    for (const auto& alias : aliases)
        if (strcmp(map, alias.map) == 0) return alias.canonical;
    return map;
}
