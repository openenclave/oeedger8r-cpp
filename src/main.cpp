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

static Warning parse_warning_option(std::string& warning)
{
    if (warning == "all")
        return Warning::All;
    else if (warning == "error")
        return Warning::Error;
    else if (warning == "foreign-type-ptr")
        return Warning::ForeignTypePtr;
    else if (warning == "non-portable-type")
        return Warning::NonPortableType;
    else if (warning == "ptr-in-struct")
        return Warning::PtrInStruct;
    else if (warning == "ptr-in-function")
        return Warning::PtrInFunction;
    else if (warning == "unsupported-allow")
        return Warning::UnsupportedAllow;
    else if (warning == "return-ptr")
        return Warning::ReturnPtr;
    else if (warning == "signed-size-or-count")
        return Warning::SignedSizeOrCount;
    else
        return Warning::Unknown;
}

bool operator>(WarningState l, WarningState r)
{
    /* The priority of the states from high to low is:
     * Ignore, Error, Warning, Unknown.
     */
    if (l == r)
        return false;
    if (l == WarningState::Ignore || r == WarningState::Unknown)
        return true;
    if (l == WarningState::Error && r == WarningState::Warning)
        return true;
    return false;
}

static void set_default_warning_options(
    std::unordered_map<Warning, WarningState, WarningHash>& warnings)
{
    /* Initialize the two special warning options. */
    warnings[Warning::Error] = WarningState::Unknown;
    warnings[Warning::All] = WarningState::Unknown;

    /* Turn on the following options by default. */
    warnings[Warning::NonPortableType] = WarningState::Warning;
    warnings[Warning::SignedSizeOrCount] = WarningState::Warning;
}

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
    "--search-path <path>   Specify the search path of EDL files\n"
    "--use-prefix           Prefix untrusted proxy with Enclave name\n"
    "--header-only          Only generate header files\n"
    "--untrusted            Generate untrusted proxy and bridge\n"
    "--trusted              Generate trusted proxy and bridge\n"
    "--untrusted-dir <dir>  Specify the directory for saving untrusted code\n"
    "--trusted-dir   <dir>  Specify the directory for saving trusted code\n"
    "-D<name>               Define the name to be used by the C-style "
    "preprocessor\n"
    "-W<warning>            Enable the specified warning\n"
    "-Wunsupported-allow    Warn if an untrusted function uses the unsupported "
    "allow syntax\n"
    "-Wforeign-type-ptr     Warn if a function includes the pointer of a "
    "foreign type\n"
    "                       as a parameter.\n"
    "-Wnon-portable-type    Warn if a function includes a non-portable type."
    "-Wsigned-size-or-count Warn if a size or count parameter is signed."
    "-Wptr-in-function      Warn if a function includes a pointer as a "
    "parameter without\n"
    "                       a direction attribute.\n"
    "-Wptr-in-struct        Warn if a struct includes a pointer type as a "
    "member\n"
    "-Wreturn-ptr           Warn if a function returns a pointer type\n"
    "-Wno-<warning>         Disable the specified warning\n"
    "-Wall                  Enable all the available warnings\n"
    "-Werror                Turn warnings into errors\n"
    "-Werror=<warning>      Turn the specified warning into an error\n"
    "--experimental         Enable experimental features\n"
    "--help                 Print this help message\n"
    "\n"
    "If neither `--untrusted' nor `--trusted' is specified, generate both.\n";

int main(int argc, char** argv)
{
    std::vector<std::string> searchpaths;
    bool use_prefix = false;
    bool header_only = false;
    bool gen_untrusted = false;
    bool gen_trusted = false;
    bool experimental = false;
    std::string untrusted_dir = ".";
    std::string trusted_dir = ".";
    std::vector<std::string> files;
    std::vector<std::string> defines;
    std::unordered_map<Warning, WarningState, WarningHash> warnings;
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

    /* Initialize the warning options. */
    set_default_warning_options(warnings);

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
            experimental = true;
        else if (a.rfind("-D", 0) == 0)
        {
            std::string define = a.substr(2);
            if (define.empty())
            {
                fprintf(stderr, "error: macro name missing after '-D'\n");
                fprintf(stderr, "%s", usage);
                return -1;
            }
            defines.push_back(define);
        }
        else if (a.rfind("-W", 0) == 0)
        {
            std::string option;
            WarningState state;
            if (a.rfind("-Wno-", 0) == 0)
            {
                state = WarningState::Ignore;
                option = a.substr(5);
            }
            else if (a.rfind("-Werror=", 0) == 0)
            {
                state = WarningState::Error;
                option = a.substr(8);
            }
            else
            {
                state = WarningState::Warning;
                option = a.substr(2);
            }
            Warning warning = parse_warning_option(option);
            if (warning == Warning::Unknown)
            {
                fprintf(
                    stderr, "error: unknown warning option '%s'\n", a.c_str());
                fprintf(stderr, "%s", usage);
                return -1;
            }
            if (state == WarningState::Error &&
                (warning == Warning::Error || warning == Warning::All))
            {
                /* Error out -Werror=error and -Werror=all. */
                fprintf(stderr, "error: invalid option '%s'\n", a.c_str());
                fprintf(stderr, "%s", usage);
                return -1;
            }
            if (warnings.find(warning) == warnings.end())
            {
                /* The warning option is not set yet. */
                warnings[warning] = state;
            }
            else
            {
                /*
                 * Only the state with higher priority can override the
                 * existing state. In the current implementation, -Wno-
                 * has the highest priority, which is followed by -Werror
                 * or -Werror= and then -Wall or -W.
                 */
                if (state > warnings[warning])
                    warnings[warning] = state;
            }
        }
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
        Parser p(file, searchpaths, defines, warnings, experimental);
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
