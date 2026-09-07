#include "../src/shared/managed_client.h"
#include "../src/mss32/updater.h"
#include "../src/mss32/error.h"
#include "../src/shared/cod2_net.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <string>

bool updater_sendRequest();
bool updater_downloadDLL(const char*, const char*, char*, size_t, int);
bool updater_downloadAndReplaceDllFile(const char*, char*, size_t);

static unsigned int callTarget, jumpTarget, nopTarget, nopLength;
void* patch_call(unsigned int target, unsigned int) { callTarget = target; return nullptr; }
void* patch_jump(unsigned int target, unsigned int) { jumpTarget = target; return nullptr; }
void patch_nop(unsigned int target, unsigned int length, void*) { nopTarget = target; nopLength = length; }

static void write(const char* path, const std::vector<unsigned char>& data) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    assert(file.good());
}
int main(int argc, char** argv) {
    assert(argc == 2); // runner-owned temporary directory, never a real game
    assert(SetCurrentDirectoryA(argv[1]));
    std::vector<unsigned char> bytes(65536 + 17, 42);
    write("owned.iwd", bytes);
    assert(reforged_managed_iwd_matches("owned.iwd", bytes.data(), bytes.size()));
    auto damaged = bytes; damaged.back() = 17;
    write("owned.iwd", damaged);
    assert(!reforged_managed_iwd_matches("owned.iwd", bytes.data(), bytes.size()));
    // Validation must not repair/replace even same-sized corrupt files.
    assert(reforged_managed_iwd_matches("owned.iwd", damaged.data(), damaged.size()));
    damaged.pop_back(); write("owned.iwd", damaged);
    assert(!reforged_managed_iwd_matches("owned.iwd", bytes.data(), bytes.size()));
    damaged = bytes; damaged.push_back(1); write("owned.iwd", damaged);
    assert(!reforged_managed_iwd_matches("owned.iwd", bytes.data(), bytes.size()));
    assert(!reforged_managed_iwd_matches("missing.iwd", bytes.data(), bytes.size()));
    assert(GetFileAttributesA("missing.iwd") == INVALID_FILE_ATTRIBUTES);
    assert(!reforged_managed_iwd_matches(".", bytes.data(), bytes.size()));
    write("mss32.dll", bytes);
    char error[128] = {};
    assert(!updater_resolveServerAddress());
    assert(!updater_sendRequest());
    assert(!updater_downloadDLL("http://127.0.0.1:1/forbidden", "new.dll", error, sizeof(error), 0));
    assert(strstr(error, "Launcher"));
    assert(!updater_downloadAndReplaceDllFile("http://127.0.0.1:1/forbidden", error, sizeof(error)));
    assert(!updater_downloadAndReplaceDllFile(nullptr, nullptr, 0));
    assert(reforged_managed_iwd_matches("mss32.dll", bytes.data(), bytes.size()));
    assert(GetFileAttributesA("new.dll") == INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesA("mss32_new.dll") == INVALID_FILE_ATTRIBUTES);
    assert(GetFileAttributesA("mss32_old.dll") == INVALID_FILE_ATTRIBUTES);
    // These actual compiled hook targets must not dereference game addresses,
    // fetch command arguments, resolve DNS, or inspect telemetry inputs.
    updater_patch();
    assert(callTarget == 0x0040ef9c && jumpTarget == 0x0053bc40);
    assert(nopTarget == 0x0041162f && nopLength == 5);
    updater_checkForUpdate(); updater_renderer(); updater_frame();
    netaddr_s address = {}; updater_updatePacketResponse(address);
    error_sendCrashData(1, 2, nullptr, 3, nullptr);
    error_sendErrorData(nullptr);
    puts("PASS: managed updater hooks, local-only errors, read-only IWD checks");
}
