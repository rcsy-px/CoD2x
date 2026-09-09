# Reforged managed Windows client

## Current release preparation (2026-09-09)

The current source uses the accepted static connection artwork and native map
splash. The retired RoQ video prototype is excluded from this build; a private
backup remains in build/release-preparation-20260909. The old binary/IWD/symbol
working-tree changes remain untouched and must not be committed as release inputs.

Rebuild the IWD from the pinned upstream Git blob and the two reviewed custom
assets under src/reforged-assets (never from a played game installation):

```powershell
../cod2/.venv/Scripts/python tools/build_reforged_iwd.py --git PATH_TO_GIT_EXE --output build/NEW_BUILD/iw_CoD2x_01.iwd
powershell -File tools/build_managed_client.ps1 -ClientIwd build/NEW_BUILD/iw_CoD2x_01.iwd
../cod2/.venv/Scripts/python tests/test_managed_client.py
```

Use a new output path; the recipe refuses existing output. Member order, timestamps,
permissions and compression are fixed. CMake accepts the explicit reviewed IWD
path; the managed helper requires it and binds that exact IWD into the policy.
The old commands below describe the historical first candidate.

The newly built DLL and all nine IWD members passed targeted local-identifier and
retired updater/telemetry/video string checks. Evidence is private under
build/release-preparation-20260909/privacy-audit.json. This does not replace full
malware scans or fresh-user/gameplay acceptance. Source asset provenance: the
previously accepted Reforged static connection artwork, unchanged in content;
original seven upstream IWD members are unchanged in content.


Implemented 2026-09-07 on develop. This is a separate build profile for client
packages owned by CoD2 Reforged Launcher. Default CoD2x builds keep their existing
behavior; Linux game servers are unaffected. A dvar, remote server command or
inherited environment variable cannot turn this build policy off.

## Build and test

From this repository, with the existing local MinGW and sibling Python tools:

```powershell
powershell -ExecutionPolicy Bypass -File tools/build_managed_client.ps1
../cod2/.venv/Scripts/python tests/test_managed_client.py
```

The script explicitly configures REFORGED_MANAGED_CLIENT=ON and Release, then
runs two build passes for the crash symbol table. Output is
build/win-Managed/managed/mss32.build.dll plus reforged-client-policy.json.
Generated symbols stay in build/win-Managed/generated/. No original game DLL,
tracked bin/zip DLL, IWD or source symbol table is overwritten.

The DLL embeds src/embedded/iw_CoD2x_01.iwd; package that exact IWD as
main/iw_CoD2x_01.iwd alongside mss32.dll and the policy JSON. Add all three to the
reviewed candidate allowlist and signed release. The policy binds both hashes;
it is publisher attestation, not a way to turn an arbitrary DLL into managed
code. Never add a policy to an unreviewed/upstream DLL to bypass the launch gate.

## Ownership and network behavior

- The upstream updater's DNS/request/retry/response/download/replacement paths
  are compiled out. The original game's updater call remains patched out.
  Direct download entry points fail; unsolicited update packets are ignored.
  Update dialog confirmation directs the player to close the game and repair
  through the launcher. No mss32_new.dll or mss32_old.dll is created.
- Automatic crash/error UDP transmission is compiled out, including calls
  before ordinary initialization. Local reports and optional local minidumps
  remain; managed dialogs explain that nothing was uploaded and that personal
  data needs review before manual sharing.
- Embedded IWD extraction becomes an exact, bounded, read-only byte comparison.
  Missing, truncated, extended or same-sized damaged files cause an actionable
  repair error. No in-game replacement or size-only acceptance.
- CoD2x IWD cleanup and automatic zPAM cleanup/download are omitted from this
  profile, preserving unrelated files. Server-supplied FastDL mod downloads
  remain a separate game mechanism, not the launcher release transport.

This does not disable all networking. The server browser still requests master
lists on demand; game connections and server-directed features remain. HWID
calculation and userinfo, Reforged identity/ticket getters, local registry state,
profiles, logs, demos and crash files are unchanged. No identifier values belong
in the package or Git.

Review findings in existing code: updater_sendRequest previously transmitted
CD-key hash, REGID, HWIDs/change history and OS information in a base64/CRC UDP
payload. DLL downloads checked HTTP status/length, without the launcher's
signature/hash trust. error_sendCrashData/error_sendErrorData transmitted IDs,
logs and crash/system detail automatically. Those sinks are absent in this build.

Further runtime boundaries: registry.cpp/hwid.cpp use HKLM and Windows registry
virtualization/migration; fresh non-admin Windows acceptance is still required.
The demo subsystem defaults to no upload URL but can upload recordings when a
server supplies one, and resumes local .upload markers. This is not the updater
telemetry sink; it remains unchanged and needs gameplay/privacy/budget acceptance
before a public release. Master browsing and FastDL likewise remain enabled.

## Validation and local inputs

The native test compiles actual updater/error implementations and calls their
managed entry points without loading a game. It checks original hook locations,
no output DLLs, null telemetry inputs, and missing/changed/truncated/extended IWDs.
It checks emitted objects for updater/telemetry sinks, syntax-checks the regular
profile, and verifies managed IWD cleanup/extraction writers are omitted.
The full Release DLL builds successfully with the isolated two-pass symbol table.

The first local candidate also uses pre-existing, uncommitted downloading.cpp
loading-screen changes and the custom embedded IWD. These inputs were preserved
and hashed; they are not silently included in the managed-policy commit. Thus
this local DLL is not yet a clean-checkout reproducible public release. Reconcile
those earlier changes and asset provenance before publication. Binary artifacts
and test data remain ignored under build/ and the sibling launcher's .build/.

No real game execution, registry change, installation into the original game,
R2 upload, server restart or production deployment is part of this acceptance.
Fresh-user startup, actual loading/joins/identity, manually triggered crash UX
and network capture remain runtime acceptance gates.
