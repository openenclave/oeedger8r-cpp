#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <string>
#include <vector>

class Preprocessor
{
public:
    Preprocessor(std::vector<std::string>& defines);

    std::string process(
        const std::string& file,
        const std::string& basename);

private:
    std::string get_cpp_cmd(const std::string& src, const std::string& dest);

    std::vector<std::string> _defines;
};

#endif // PREPROCESSOR_H
