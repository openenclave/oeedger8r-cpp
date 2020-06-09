// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <cstdlib>
#include <string>
#include <vector>

#include "args_h_emitter.h"
#include "c_emitter.h"
#include "h_emitter.h"
#include "parser.h"

#ifdef _WIN32
#include <windows.h>
#endif

static void _ensure_directory(const std::string& dir)
{
#if _WIN32
    std::string::size_type pos = 0;
    do
    {
        pos = dir.find_first_of("\\/", pos + 1);
        std::string subdir = dir.substr(0, pos);
        CreateDirectoryA(subdir.c_str(), NULL);
    } while (pos != std::string::npos);
#else
    system(("mkdir -p " + dir).c_str());
#endif
}

const char* usage =
    "usage: oeedger8r [options] <file> ...\n"
    "\n"
    "[options]\n"
    "--search-path <path>  Specify the search path of EDL files\n"
    "--use-prefix          Prefix untrusted proxy with Enclave name\n"
    "--header-only         Only generate header files\n"
    "--untrusted           Generate untrusted proxy and bridge\n"
    "--trusted             Generate trusted proxy and bridge\n"
    "--untrusted-dir <dir> Specify the directory for saving untrusted code\n"
    "--trusted-dir   <dir> Specify the directory for saving trusted code\n"
    "--experimental        Enable experimental features\n"
    "--help                Print this help message\n"
    "\n"
    "If neither `--untrusted' nor `--trusted' is specified, generate both.\n";

int main(int argc, char** argv)
{
    std::vector<std::string> searchpaths;
    bool use_prefix = false;
    bool header_only = false;
    bool gen_untrusted = false;
    bool gen_trusted = false;
    std::string untrusted_dir = ".";
    std::string trusted_dir = ".";
    std::vector<std::string> files;
    int i = 1;

    if (argc == 1)
    {
        printf("%s\n", usage);
        return 1;
    }

    auto get_dir = [argc, argv](int i) {
        if (i == argc)
        {
            fprintf(
                stderr,
                "error: missing directory name after %s\n",
                argv[i - 1]);
            fprintf(stderr, "%s\n", usage);
            exit(1);
        }
        return fix_path_separators(argv[i]);
    };

    while (i < argc)
    {
        std::string a = argv[i++];
        if (a == "--search-path")
            searchpaths.push_back(get_dir(i++));
        else if (a == "--use-prefix")
            use_prefix = true;
        else if (a == "--header-only")
            header_only = true;
        else if (a == "--untrusted")
            gen_untrusted = true;
        else if (a == "--trusted")
            gen_trusted = true;
        else if (a == "--trusted-dir")
            trusted_dir = get_dir(i++);
        else if (a == "--untrusted-dir")
            untrusted_dir = get_dir(i++);
        else if (a == "--experimental")
            ;
        else if (a == "--help")
        {
            printf("%s\n", usage);
            return 1;
        }
        else
            files.push_back(fix_path_separators(a));
    }

    if (files.empty())
    {
        fprintf(stderr, "error: missing edl filename.\n");
        fprintf(stderr, "%s", usage);
        return -1;
    }

    printf("Generating edge routine, for the Open Enclave SDK.\n");

    if (!gen_trusted && !gen_untrusted)
        gen_trusted = gen_untrusted = true;

    const char* sep = path_sep();

    // Add separators. / works on both Linux and Windows.
    if (trusted_dir.back() != sep[0])
        trusted_dir += sep;
    if (trusted_dir != std::string(".") + sep)
        _ensure_directory(trusted_dir);

    if (untrusted_dir.back() != sep[0])
        untrusted_dir += sep;
    if (untrusted_dir != std::string(".") + sep)
        _ensure_directory(untrusted_dir);

    for (std::string& file : files)
    {
        Parser p(file, searchpaths);
        Edl* edl = p.parse();

        if (gen_trusted)
        {
            ArgsHEmitter(edl).emit(trusted_dir);
            HEmitter(edl).emit_t_h(trusted_dir);
            if (!header_only)
                CEmitter(edl).emit_t_c(trusted_dir);
        }
        if (gen_untrusted)
        {
            std::string prefix = use_prefix ? (edl->name_ + "_") : "";
            ArgsHEmitter(edl).emit(untrusted_dir);
            HEmitter(edl).emit_u_h(untrusted_dir, prefix);
            if (!header_only)
                CEmitter(edl).emit_u_c(untrusted_dir, prefix);
        }
    }

    printf("Success.\n");
}
