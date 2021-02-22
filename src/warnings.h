// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#ifndef WARNINGS_H
#define WARNINGS_H

enum class Warning
{
    All,
    Error,
    ForeignTypePtr,
    NonPortableType,
    PtrInStruct,
    PtrInFunction,
    ReturnPtr,
    SignedSizeOrCount,
    UnsupportedAllow,
    Unknown
};

/* A bug with C++11 prevents an enum class directly be used as a key in
 * an unordered_map. As a workaround, we have to provide a customized hash
 * function as a third argument to the unordered_map. Note that the bug
 * was fixed in C++14.
 * See http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#2148.
 */
struct WarningHash
{
    std::size_t operator()(Warning w) const
    {
        return static_cast<std::size_t>(w);
    }
};

enum class WarningState
{
    Ignore,
    Warning,
    Error,
    Unknown
};

#endif // WARNINGS_H
