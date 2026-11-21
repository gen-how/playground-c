/* Build script. Bootstrap once, then just run ./nob (it rebuilds itself).
 *
 *   gcc/clang: cc -std=c11 -o nob nob.c
 *   MSVC: cl /nologo /std:c11 /utf-8 nob.c  (in a Developer PowerShell)
 *
 * Every top-level folder that contains a main.c is one program:
 *
 *   ./hello-world/main.c  ->  ./hello-world/build/main.exe
 *
 * A program is rebuilt only when a .c/.h file in its folder, or nob.c itself,
 * is newer than its executable.
 */
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#ifdef _WIN32
#    define EXE_SUFFIX ".exe"
#else
#    define EXE_SUFFIX ""
#endif

static void append_std_flags(Cmd *cmd)
{
#if defined(_MSC_VER) && !defined(__clang__)
    cmd_append(cmd, "/std:c11", "/utf-8", "/W4", "/WX", "/nologo", "/D_CRT_SECURE_NO_WARNINGS");
#else
    cmd_append(cmd, "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-g");
#endif
}

static bool is_source_file(const char *name)
{
    String_View sv = sv_from_cstr(name);
    return sv_end_with(sv, ".c") || sv_end_with(sv, ".h");
}

/* Returns 1 if rebuilt, 0 if up to date, -1 on error. */
static int build_program(const char *dir)
{
    const char *build_dir = temp_sprintf("%s/build", dir);
    const char *output = temp_sprintf("%s/main", build_dir);
    const char *exe = temp_sprintf("%s" EXE_SUFFIX, output);

    /* Inputs for the up-to-date check: nob.c plus every .c/.h in the folder. */
    File_Paths entries = {0};
    File_Paths inputs = {0};
    if (!read_entire_dir(dir, &entries)) {
        return -1;
    }
    da_append(&inputs, "nob.c");
    da_foreach(const char *, name, &entries)
    {
        if (is_source_file(*name)) {
            da_append(&inputs, temp_sprintf("%s/%s", dir, *name));
        }
    }
    /* Check existence first: on Windows needs_rebuild() reports a missing
     * build/ folder (ERROR_PATH_NOT_FOUND) as an error, not as "rebuild". */
    int rebuild = file_exists(exe) ? needs_rebuild(exe, inputs.items, inputs.count) : 1;
    da_free(entries);
    da_free(inputs);
    if (rebuild < 0) {
        return -1;
    }
    if (rebuild == 0) {
        nob_log(INFO, "%s is up to date", exe);
        return 0;
    }

    if (!mkdir_if_not_exists(build_dir)) {
        return -1;
    }

    Cmd cmd = {0};
    nob_cc(&cmd);
    append_std_flags(&cmd);
    nob_cc_output(&cmd, output);
    nob_cc_inputs(&cmd, temp_sprintf("%s/main.c", dir));
    bool ok = cmd_run(&cmd);
    cmd_free(cmd);
    return ok ? 1 : -1;
}

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);

    File_Paths entries = {0};
    if (!read_entire_dir(".", &entries)) {
        return 1;
    }

    size_t found = 0;
    size_t failed = 0;
    da_foreach(const char *, name, &entries)
    {
        /* Skip ".", "..", and hidden folders such as .git and .vscode. */
        if ((*name)[0] == '.' || get_file_type(*name) != FILE_DIRECTORY) {
            continue;
        }
        if (!file_exists(temp_sprintf("%s/main.c", *name))) {
            continue;
        }

        found += 1;
        if (build_program(*name) < 0) {
            failed += 1;
        }
    }
    da_free(entries);

    if (found == 0) {
        nob_log(WARNING, "No <folder>/main.c found");
    }
    if (failed > 0) {
        nob_log(ERROR, "%zu of %zu program(s) failed to build", failed, found);
        return 1;
    }
    return 0;
}
