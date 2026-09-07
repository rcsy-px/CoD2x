#ifndef REFORGED_MANAGED_CLIENT_H
#define REFORGED_MANAGED_CLIENT_H

// Build policy, never a server-controlled dvar or inherited environment flag.
#ifndef REFORGED_MANAGED_CLIENT
#define REFORGED_MANAGED_CLIENT 0
#endif
#if REFORGED_MANAGED_CLIENT && !defined(_WIN32)
#error Reforged managed client is a Windows client build, not a Linux server policy
#endif

#include <cstdio>
#include <cstring>
#include <cstddef>

// Read-only, exact comparison with the IWD embedded in this DLL. A mismatch
// must be repaired by the signed launcher package, never by the running game.
inline bool reforged_managed_iwd_matches(const char* path,
                                        const unsigned char* bytes,
                                        size_t size) {
    FILE* file = fopen(path, "rb");
    if (!file) return false;
    unsigned char buffer[65536];
    size_t offset = 0;
    bool matches = true;
    while (offset < size) {
        const size_t count = size - offset < sizeof(buffer) ? size - offset : sizeof(buffer);
        if (fread(buffer, 1, count, file) != count ||
            memcmp(buffer, bytes + offset, count) != 0) {
            matches = false;
            break;
        }
        offset += count;
    }
    if (matches) matches = fgetc(file) == EOF && !ferror(file);
    fclose(file);
    return matches;
}
#endif
