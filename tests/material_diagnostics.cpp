#include "../src/mss32/material_diagnostics.h"
#include <assert.h>
#include <stdarg.h>
#include <string>
#include <fstream>
#include <iterator>
static int calls = 0;
static const char* expected_format = "fixture %d";
static void __cdecl error_stub(int code, const char* format, ...) {
    assert(code == 7 && format == expected_format);
    va_list ap; va_start(ap, format); assert(va_arg(ap, int) == 1024); va_end(ap);
    ++calls;
}
int main(int argc, char** argv) {
    assert(argc == 2);
    using namespace material_diagnostics;
    unsigned char* pool = static_cast<unsigned char*>(VirtualAlloc(nullptr, 0x1d2000,
                                          MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    assert(pool);
    renderer = reinterpret_cast<uintptr_t>(pool);
    *reinterpret_cast<uint32_t*>(pool + 0x1d0b04) = 1025; // capture must clamp
    const char* name = "material with spaces\nnext";
    uint32_t material = reinterpret_cast<uint32_t>(name);
    *reinterpret_cast<uint32_t*>(pool + 0x1d0b08) = reinterpret_cast<uint32_t>(&material);
    *reinterpret_cast<uint32_t*>(pool + 0x1d0b0c) = 1; // invalid pointer: safe read
    snprintf(output, sizeof(output), "%s/materials.txt", argv[1]);
    original_error = error_stub;
    limit_error(7, expected_format, 1024);
    assert(calls == 1);
    std::ifstream file(output);
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    assert(text.find("material with spaces?next") != std::string::npos);
    assert(text.find("1023 ") != std::string::npos);
    assert(text.find("\n1024 ") == std::string::npos);
    assert(text.size() < 220000);
    // A failed output cannot suppress/change the original fatal call.
    snprintf(output, sizeof(output), "%s/does-not-exist/materials.txt", argv[1]);
    limit_error(7, expected_format, 1024);
    assert(calls == 2);
    VirtualFree(pool, 0, MEM_RELEASE);
    puts("PASS: bounded material snapshot, invalid pointers, preserved fatal arguments and failed output");
}
