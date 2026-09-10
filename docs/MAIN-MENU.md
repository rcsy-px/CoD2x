# Profile/options follow-up (2026-09-10)

User liked the main-menu preview; reported profile background widening, old Options
art and low multiplayer-settings contrast. Follow-up staged to developer only:

- background.menu uses rect 0 0 640 480 4/4 instead of stock -128 0 896 480;
  this matches main_text when player_profile closes the foreground menu.
- rfg_options.menu uses the same map-table background, gold hover/focus, existing
  logo and restrained header/footer lines; left settings get a dark panel.
- options_multi.menu gets a local dark panel below its native controls.

Preserve native setting positions, all original actions and profile behavior.
IMPORTANT: zpam408.iwd overrides ui_mp/options.menu. Use its original reviewed
inspection source (.tmp-zpam-inspect/zpam408.iwd), keeping options_zpam and its
extra controls. The latest stock iw_15 menus.txt routes just that one loadMenu to
ui_mp/rfg_options.menu; it otherwise preserves the full stock menu list. Native
menu names remain unchanged, so submenu close/apply/escape scripts still work.
Multiplayer source also comes from latest iw_15, not older iw_06. No runtime
profiles/configs are packaged; builder uses only explicit tracked menu sources.

FINAL artifact build/mainmenu-settings-20260910-r4/iw_CoD2x_01.iwd:
17 members,5,401,407bytes; IWD SHA256
8137f40290283052a01bbe476562adb101d326fd50f705c76beb3cce71fdfdf4;
DLL SHA256 2a0eb7ae0b79dd96c755f36dc4a77b14eaea6c4ce72caae270065ab990a25d53.
Earlier follow-up builds are superseded and were never staged. Matching final
DLL/IWD/policy staged only to fixed developer game; prior pair preserved under
cod2/.build/client-workspace/before-mainmenu-settings-20260910.
Build, archive integrity, menu brace checks, exact preservation of settings actions
and latest menu-list routing checks pass. Managed/reset regressions pass.
Developer launcher reopened; in-game visual follow-up acceptance is pending.
Public R2 sequence4 and live/dev game services unchanged.

---

# Reforged client main menu

Implemented 2026-09-10 for the isolated developer client only. User mockup:
web/mockups/mainmenu.png. Layout follows the dark campaign-table design with
reused native Reforged logo, six right-side buttons, gold hover, top campaign
caption and bottom quote/footer. Background was generated with built-in imagegen;
source/provenance/prompt: src/reforged-assets/mainmenu/PROMPT.md.

## Source and build

- src/reforged-assets/ui_mp/main.menu: native UI controls and decoration.
- src/reforged-assets/mainmenu: reviewed background PNG, existing logo PNG,
  accepted material template, provenance. No runtime files are imported.
- tools/build_reforged_iwd.py: pinned historical IWD plus explicit reviewed
  overrides. Requires Pillow (server workspace Python venv includes it).

Build a new path, never overwrite a published artifact:

```powershell
../cod2/.venv/Scripts/python tools/build_reforged_iwd.py --git <git.exe> --output build/<new-id>/iw_CoD2x_01.iwd
./tools/build_managed_client.ps1 -ClientIwd build/<new-id>/iw_CoD2x_01.iwd
../cod2/.venv/Scripts/python tests/test_managed_client.py
```

Only ui_mp/main.menu changes among previous IWD members. Four additional members:
images/materials rfg_mnbg and rfg_mnlg. Connection art remains byte-identical.
Controls/art use fullscreen alignment 4/4; captions and all buttons are native,
not baked into images. Existing stock profile picker (including in-game warning),
multiplayer options, options, mods and quit confirmation keep their actions.
Default underlying bg reads rfg_mnbg; the old stock logo/version/separator are
hidden while this main menu is active. No server-IWD or server deployment needed.

## Authenticated join behavior

When connected, first button is RETURN TO BATTLE and closes main.
When disconnected, JOIN REFORGED SERVER opens an explicit confirmation explaining
that a fresh verified join is obtained through launcher Play. RETURN TO LAUNCHER
quits the game; CANCEL/ESC leaves it running. This assumes the normal supported
launcher-started session (launcher remains behind the game). It does not launch
a missing launcher, reconnect with an expired ticket, or bypass admission with
a hardcoded IP. Automatic launcher handoff is not implemented here.

## Current developer artifact / acceptance

build/mainmenu-20260910/iw_CoD2x_01.iwd: 13 members, 5,396,241 bytes.
IWD SHA256 cfc38f524a27925c756027a758fd094347cda20cbe064596f583d116451fefbd
DLL SHA256 12afc3839b05811ed977046d3edacc12ee9fdea378864f2f64dd88d73f305f6a
Two builds yielded identical IWD bytes. Native menu brace/texture/material checks,
managed safety regressions and renderer-reset regressions pass. Two-pass managed
DLL build passed. IWD/DLL/policy staged together ONLY to the fixed developer game.
Previous pair is preserved outside that played tree under the server workspace
.build/client-workspace/before-mainmenu-20260910. The fixed embedded filename was
retired there before staging the new matching pair; published artifacts unchanged.

The developer launcher is open. User has been asked to enter via Play, open Main
Menu and check appearance, hover, profile/options actions. Actual in-game visual
acceptance and resolution changes remain pending. Public R2 sequence4 and launcher
0.2.4 are unchanged. Do not publish from this played developer tree; a new release
needs independent source/privacy hash review, clean assembly and antivirus scans
under cod2/docs/CLIENT-WORKSPACE.md and RELEASE-WORKFLOW.md.
