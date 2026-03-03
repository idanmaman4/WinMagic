#pragma once

#include <vector>
#include <exception>
#include <expected>
#include <string>
#include "Windows.h"

using Bytes = std::vector<std::byte>;

template <typename T>
using Expected = std::expected<T,std::exception>;

using Address = uintptr_t;

struct FieldInfo
{
    std::string name;
    std::string type_name;
    ULONG       type_id  = 0;
    ULONG       offset   = 0;
    ULONG       size     = 0;
    bool        is_pointer = false;
};

template <std::size_t N>
struct FixedString {
    std::array<char, N> buf{};

    constexpr FixedString(const char (&s)[N]) {
        std::copy_n(s, N, buf.data());
    }

    constexpr std::string_view view() const { return {buf.data(), N - 1}; }
};

template <std::size_t N>
FixedString(const char (&)[N]) -> FixedString<N>;