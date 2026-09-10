#include "../src/mss32/loading_map_material.h"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
int main() {
    const char* source[] = {"mp_burgundy_fix", "mp_carentan_bal", "mp_carentan_fix",
        "mp_dawnville_fix", "mp_matmata_fix", "mp_toujane_fix", "mp_trainstation_fix"};
    const char* expected[] = {"mp_burgundy", "mp_carentan", "mp_carentan",
        "mp_dawnville", "mp_matmata", "mp_toujane", "mp_trainstation"};
    for (unsigned i = 0; i < 7; ++i) {
        const char* name = loading_map_material_name(source[i]);
        assert(strcmp(name, expected[i]) == 0);
        assert(strcmp(loading_map_material_name(name), name) == 0);
    }
    for (const char* map : {"mp_chelm_fix", "mp_vallente_fix", "mp_breakout_tls",
                           "mp_dawnville_sun", "mp_leningrad_tls", "mp_toujane", ""})
        assert(loading_map_material_name(map) == map);
    puts("PASS: stock splash aliases canonicalized; custom map names retained");
}
