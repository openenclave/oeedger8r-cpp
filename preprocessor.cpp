#include "preprocessor.h"
#include "utils.h"

#ifdef _WIN32
#define CPP_CMD "cl.exe /EP /nologo"
#else
#define CPP_CMD "cpp -P"
#endif

Preprocessor::Preprocessor(
    std::vector<std::string>& defines)
{
    _defines = defines;
}

/*
 * Outputs a command in the following format:
 * CPP_CMD {defines...} {input_edl} > {output_file}
 */
std::string Preprocessor::get_cpp_cmd(
    const std::string& src,
    const std::string& dest)
{
    std::ostringstream oss;
    oss << CPP_CMD;
    for (const auto& d : _defines)
        oss << " " << d;

    oss << " " << src << " > " << dest;
    return oss.str();
}

std::string Preprocessor::process(
    const std::string& file,
    const std::string& basename)
{
    std::string out_file(basename + ".edl.out");
    std::string cmd = get_cpp_cmd(file, out_file);

    if (system(cmd.c_str()) != 0)
    {
        /*
         * The preprocessor dependency is a soft one. If the preprocessor
         * command fails for any reason, just output and error message
         * and move on with the original file.
         *
         * If the file really did depend on the preprocessor, it will contain
         * invalid characters and parsing will fail later.
         */
        printf("warning: preprocessing file %s\n", file.c_str());
        return file;
    }

    return out_file;
}
