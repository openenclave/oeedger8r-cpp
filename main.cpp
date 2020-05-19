// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <string>
#include <vector>

#include "args_h_emitter.h"
#include "c_emitter.h"
#include "h_emitter.h"
#include "parser.h"

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
    bool header_only = false;
    bool gen_untrusted = false;
    bool gen_trusted = false;
    std::string untrusted_dir;
    std::string trusted_dir;
    std::vector<std::string> files;
    int i = 1;

    if (argc == 1)
    {
        printf("%s\n", usage);
        return 1;
    }

    while (i < argc - 1)
    {
        std::string a = argv[i];
        if (a == "--search-path")
            searchpaths.push_back(argv[++i]);
        else if (a == "--use-prefix")
            continue;
        else if (a == "--header-only")
            header_only = true;
        else if (a == "--untrusted")
            gen_untrusted = true;
        else if (a == "--trusted")
            gen_trusted = true;
        else if (a == "--trusted-dir")
            untrusted_dir = argv[++i];
        else if (a == "--experimental")
            ;
        else if (a == "--help")
        {
            printf("%s\n", usage);
            return 1;
        }
        else
            files.push_back(a);
        ++i;
    }
    if (i < argc)
        files.push_back(argv[i]);

    if (files.empty())
    {
        printf("error: missing edl filename.\n");
        printf("%s", usage);
        return -1;
    }

    printf("Generating edge routine, for the Open Enclave SDK.\n");

    if (!gen_trusted && !gen_untrusted)
        gen_trusted = gen_untrusted = true;

    for (std::string& file : files)
    {
        Parser p(file, searchpaths);
        Edl* edl = p.parse();

        ArgsHEmitter(edl).emit();

        if (gen_trusted)
        {
            HEmitter(edl).emit_t_h();
            if (!header_only)
                CEmitter(edl).emit_t_c();
        }
        if (gen_untrusted)
        {
            HEmitter(edl).emit_u_h();
            if (!header_only)
                CEmitter(edl).emit_u_c();
        }
    }

    printf("Success.\n");
}
