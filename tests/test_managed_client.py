"""Compile real managed hook implementations, then run without loading the game."""
import argparse
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]

def run(*args):
    return subprocess.run([str(a) for a in args], cwd=ROOT, check=True,
                          stdout=subprocess.PIPE, text=True).stdout

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", type=pathlib.Path,
                        default=ROOT / "tools/mingw/bin/g++.exe")
    args = parser.parse_args()
    parent = ROOT / "build/managed-tests"
    parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=parent) as temp:
        temp = pathlib.Path(temp)
        objects = []
        for rel in ("tests/managed_client.cpp", "src/mss32/updater.cpp", "src/mss32/error.cpp"):
            obj = temp / (pathlib.Path(rel).stem + ".o")
            run(args.compiler, "-std=c++17", "-m32", "-O2",
                "-DREFORGED_MANAGED_CLIENT=1", "-ffunction-sections", "-fdata-sections",
                "-Isrc/shared", "-Isrc/mss32", "-c", rel, "-o", obj)
            objects.append(obj)
        # Code emission check: no upstream wire payload or downloader remains.
        forbidden = (b"getUpdateInfo", b"logCrashData", b"logErrorData",
                     b"InternetOpen", b"MoveFileEx", b"NET_OutOfBandPrint")
        for obj in objects[1:]:
            data = obj.read_bytes()
            for marker in forbidden:
                assert marker not in data, f"Forbidden managed object marker: {marker!r}"
        exe = temp / "managed-client-test.exe"
        run(args.compiler, "-m32", *objects, "-Wl,--gc-sections",
            "-static", "-static-libgcc", "-static-libstdc++", "-o", exe)
        print(run(exe, temp).strip())
        avatar_exe = temp / "avatar-renderer-reset.exe"
        run(args.compiler, "-std=c++17", "-m32", "-msse2", "-O2",
            "-ffunction-sections", "-fdata-sections", "tests/avatar_renderer_reset.cpp",
            "-Wl,--gc-sections", "-static", "-lwininet", "-lole32", "-lwindowscodecs",
            "-o", avatar_exe)
        run(avatar_exe)
        scoreboard_exe = temp / "scoreboard-renderer-reset.exe"
        run(args.compiler, "-std=c++17", "-m32", "-msse2", "-O2",
            "-ffunction-sections", "-fdata-sections", "tests/scoreboard_renderer_reset.cpp",
            "-Wl,--gc-sections", "-static", "-o", scoreboard_exe)
        run(scoreboard_exe)
        print("PASS: scoreboard renderer reset drops every material and renews subscription")

        print("PASS: avatar renderer reset drops stale handles and preserves decoded pixels")

        material_exe = temp / "material-diagnostics.exe"
        run(args.compiler, "-std=c++17", "-m32", "-O2",
            "-ffunction-sections", "-fdata-sections", "tests/material_diagnostics.cpp",
            "-Itools/openssl_mingw/include", "-Wl,--gc-sections", "-static", "-o", material_exe)
        print(run(material_exe, temp).strip())
        loading_exe = temp / "loading-map-material.exe"
        run(args.compiler, "-std=c++17", "-m32", "-O2", "-static",
            "tests/loading_map_material.cpp", "-o", loading_exe)
        print(run(loading_exe).strip())



    print("PASS: managed object code excludes updater/network telemetry sinks")
    for rel in ("src/mss32/updater.cpp", "src/mss32/error.cpp", "src/shared/iwd.cpp"):
        run(args.compiler, "-std=c++17", "-m32", "-w", "-DREFORGED_MANAGED_CLIENT=0",
            "-Isrc/shared", "-Isrc/mss32", "-fsyntax-only", rel)
    managed_iwd = run(args.compiler, "-std=c++17", "-m32", "-DREFORGED_MANAGED_CLIENT=1",
                      "-Isrc/shared", "-Isrc/mss32", "-E", "src/shared/iwd.cpp")
    assert "iwd_cleanupCoD2xIwdFiles" not in managed_iwd
    assert "iwd_processZpamFiles" not in managed_iwd
    body = managed_iwd.split("void iwd_extractFiles(")[-1].split("void iwd_extractIwdFileToMain(")[0]
    assert "reforged_managed_iwd_matches" in body
    assert "fopen(" not in body and "fwrite(" not in body and "remove(" not in body
    print("PASS: regular source profile compiles; managed IWD cleanup/extraction writers excluded")

if __name__ == "__main__":
    main()
