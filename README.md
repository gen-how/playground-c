# playground-c

C11 playground, built with [nob.h](https://github.com/tsoding/nob.h).

## Build

Each top-level folder with a `main.c` is one program: `hello-world/main.c` builds to `hello-world/build/main.exe`. `nob` builds them all, skips programs that are up to date, and rebuilds itself when `nob.c` changes. Bootstrap it once:

gcc / clang:

```sh
cc -std=c11 -o nob nob.c
./nob
./hello-world/build/main
```

MSVC (run in a Developer PowerShell for VS):

```powershell
cl /nologo /std:c11 /utf-8 nob.c
.\nob.exe
.\hello-world\build\main.exe
```
