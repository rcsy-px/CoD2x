#pragma once

// Opt-in, local-only incident capture. No capacity/index or rendering changes.
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include "../shared/patch.h"

namespace material_diagnostics {
static uintptr_t renderer = 0;
static char output[MAX_PATH] = {};
typedef void (__cdecl *ErrorFn)(int, const char*, ...);
static ErrorFn original_error = nullptr;

static bool read(uintptr_t address, void* target, size_t size) {
    SIZE_T copied = 0;
    return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address),
                             target, size, &copied) && copied == size;
}

// Invoked at the renderer's capacity-error call, before R_Error tears it down.
static void __cdecl limit_error(int code, const char* format, int count) {
    FILE* f = fopen(output, "wb");
    if (f) {
        uint32_t used = 0;
        read(renderer + 0x1d0b04, &used, sizeof(used));
        fprintf(f, "material-limit snapshot v1\ncount=%u reported=%d renderer=%08lx\n",
                used, count, static_cast<unsigned long>(renderer));
        void* frames[32] = {};
        USHORT n = CaptureStackBackTrace(0, 32, frames, nullptr);
        for (USHORT i = 0; i < n; ++i) fprintf(f, "stack %u %p\n", i, frames[i]);
        // Bounded names only: no identity, config, ticket or memory dump.
        for (uint32_t i = 0; i < used && i < 1024; ++i) {
            uint32_t material = 0, name = 0;
            char text[193] = {};
            if (read(renderer + 0x1d0b08 + i * 4, &material, 4) &&
                material && read(material, &name, 4) && name) {
                for (size_t j = 0; j < sizeof(text) - 1; ++j) {
                    unsigned char ch = 0;
                    if (!read(name + j, &ch, 1) || !ch) break;
                    text[j] = (ch >= 32 && ch < 127) ? static_cast<char>(ch) : '?';
                }
            }
            fprintf(f, "%04u %08x %s\n", i, material, text);
        }
        fclose(f);
    }
    // Preserve the original fatal path and arguments, even if capture failed.
    original_error(code, format, count);
}

static bool known_renderer(HMODULE module) {
    char path[MAX_PATH] = {};
    if (!GetModuleFileNameA(module, path, sizeof(path))) return false;
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) { fclose(f); return false; }
    bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1;
    unsigned char buffer[8192], digest[32] = {};
    size_t n;
    while (ok && (n = fread(buffer, 1, sizeof(buffer), f)) != 0)
        ok = EVP_DigestUpdate(ctx, buffer, n) == 1;
    ok = ok && !ferror(f);
    fclose(f);
    unsigned int length = 0;
    ok = ok && EVP_DigestFinal_ex(ctx, digest, &length) == 1 && length == 32;
    EVP_MD_CTX_free(ctx);
    static const unsigned char expected[32] = {
        0x24,0x35,0x05,0xf5,0x6e,0xc0,0xf8,0x61,0x22,0xfc,0x09,0xb3,0xa6,0xc1,0x8f,0x0d,
        0xaf,0x49,0xa0,0x0a,0xbb,0x2d,0x96,0x91,0x47,0xf5,0x95,0x99,0x15,0x84,0xa6,0xb2};
    return ok && memcmp(digest, expected, sizeof(expected)) == 0;
}

static void install(unsigned int base) {
    // Marker beside the EXE opts in. Absent marker means no patch and no I/O log.
    char path[MAX_PATH] = {};
    DWORD length = GetModuleFileNameA(nullptr, path, sizeof(path));
    if (!length || length >= sizeof(path)) return;
    char* filename = strrchr(path, '\\');
    if (!filename) return;
    ++filename;
    const size_t available = sizeof(path) - (filename - path);
    const char* marker = ".reforged-material-diagnostics";
    if (available <= strlen(marker)) return;
    strcpy(filename, marker);
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) return;
    if (!known_renderer(reinterpret_cast<HMODULE>(base))) return;
    unsigned char call[5] = {}, compare[5] = {};
    uint32_t count_address = 0;
    if (!read(base + 0x1712a, call, 5) || call[0] != 0xe8 ||
        !read(base + 0x17116, compare, 5) ||
        memcmp(compare, "\x3d\x00\x04\x00\x00", 5) != 0 ||
        !read(base + 0x1711c, &count_address, 4) || count_address != base + 0x1d0b04) return;
    int32_t displacement;
    memcpy(&displacement, call + 1, 4);
    if (base + 0x1712f + displacement != base + 0x13a30) return;
    const char* report = "reforged-material-limit.txt";
    if (available <= strlen(report)) return;
    strcpy(filename, report);
    strcpy(output, path);
    renderer = base;
    original_error = reinterpret_cast<ErrorFn>(base + 0x13a30);
    patch_call(base + 0x1712a, reinterpret_cast<unsigned int>(&limit_error));
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(base + 0x1712a), 5);
}
}
