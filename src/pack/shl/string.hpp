#pragma once

/* string.hpp
 * v1.0
 *
 * string utility header
 * str(...)     converts parameters to one string
 * wstr(...)    converts parameters to one wstring
 * bstr<C>(...) converts parameters to one basic_string<C>
 *
 * is_space(c)              returns true if c is a whitespace character
 * is_newline(c)            returns true if c is a newline character
 * is_alpha(c)              returns true if c is an alphabet character
 * is_digit(c)              returns true if c is a digit
 * is_bin_digit(c)          returns true if c is a binary digit (0 or 1)
 * is_oct_digit(c)          returns true if c is an octal digit (0-7)
 * is_hex_digit(c)          returns true if c is a hexadecimal digit
 * is_alphanum(c)           returns true if is_digit(c) || is_alpha(c)
 * is_upper(c)              returns true if c is an uppercase letter
 * is_lower(c)              returns true if c is an lowercase letter
 *
 * is_blank(s)              returns true if s is an empty string or only contains
 *                          whitespaces
 *
 * ltrim(s)                 trims whitespaces from the left of string s, in-place
 * rtrim(s)                 trims whitespaces from the right of string s, in-place
 * trim(s)                  trims whitespaces from the left and right of string s, in-place
 * begins_with(s, prefix)   returns true if prefix is a prefix of s
 * ends_with(s, suffix)     returns true if suffix is a suffix of s
 *
 * to_integer(s)            converts the given string to an integer
 * to_decimal(s)            converts the given string to a floating point number
 * to_upper(c/s)            converts the given character or string to upper case
 * to_lower(c/s)            converts the given character or string to upper case
 *
 * copy(src, dest)          copies one string to another
 * copy(src, dest, n)       copies one string to another, up to n characters
 *
 * aliases:
 * to_int -> to_integer<int>
 * to_long -> to_integer<long>
 * ...
 * to_float -> to_decimal<float>
 * to_double -> to_decimal<double>
 * ...
 */

#include <sstream>
#include <algorithm>
#include <type_traits>
#include <cctype>
#include <cwctype>

namespace internal
{
    template<typename T>
    using stream_string = std::basic_string<typename T::char_type, typename T::traits_type, typename T::allocator_type>;

    template<typename StringStreamT, typename T>
    stream_string<StringStreamT> _str(StringStreamT &_ss, T &&t)
    {
        _ss << t;
        return _ss.str();
    }

    template<typename StringStreamT, typename T, typename... Ts>
    stream_string<StringStreamT> _str(StringStreamT &_ss, T &&t, Ts &&...ts)
    {
        _ss << t;
        return _str(_ss, std::forward<Ts>(ts)...);
    }
}

template<typename CharT, typename T>
std::basic_string<CharT> bstr(T &&t)
{
    using namespace internal;
    std::basic_stringstream<CharT> _ss;
    _ss << t;
    return _ss.str();
}

template<typename CharT, typename T, typename... Ts>
std::basic_string<CharT> bstr(T &&t, Ts &&...ts)
{
    using namespace internal;
    std::basic_stringstream<CharT> _ss;
    return _str(_ss, std::forward<T>(t), std::forward<Ts>(ts)...);
}

template<typename T>
std::string str(T &&t)
{
    return bstr<std::string::value_type>(std::forward<T>(t));
}

template<typename T, typename... Ts>
std::string str(T &&t, Ts &&...ts)
{
    return bstr<std::string::value_type>(std::forward<T>(t), std::forward<Ts>(ts)...);
}

template<typename T>
std::wstring wstr(T &&t)
{
    return bstr<std::wstring::value_type>(std::forward<T>(t));
}

template<typename T, typename... Ts>
std::wstring wstr(T &&t, Ts &&...ts)
{
    return bstr<std::wstring::value_type>(std::forward<T>(t), std::forward<Ts>(ts)...);
}

template<typename CharT>
inline bool is_space(CharT c)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return std::iswspace(static_cast<wint_t>(c));
    else
        return std::isspace(c);
}

template<typename CharT>
inline bool is_newline(CharT c)
{
    return c == '\n' || c == '\v' || c == '\f';
}

template<typename CharT>
inline bool is_alpha(CharT c)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return std::iswalpha(static_cast<wint_t>(c));
    else
        return std::isalpha(c);
}

template<typename CharT>
inline bool is_digit(CharT c)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return std::iswdigit(static_cast<wint_t>(c));
    else
        return std::isdigit(c);
}

template<typename CharT>
inline bool is_bin_digit(CharT c)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return c == L'0' || c == L'1';
    else
        return c == '0' || c == '1';
}

template<typename CharT>
inline bool is_oct_digit(CharT c)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return c == L'0' || c == L'1' || c == L'2' || c == L'3' || c == L'4' || c == L'5' || c == L'6' || c == L'7';
    else
        return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7';
}

template<typename CharT>
inline bool is_hex_digit(CharT c)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return std::iswxdigit(static_cast<wint_t>(c));
    else
        return std::isxdigit(c);
}

template<typename CharT>
inline bool is_alphanum(CharT c)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return std::iswalnum(static_cast<wint_t>(c));
    else
        return std::isalnum(c);
}

template<typename CharT>
inline bool is_upper(CharT c)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return std::iswupper(static_cast<wint_t>(c));
    else
        return std::isupper(static_cast<wint_t>(c));
}

template<typename CharT>
inline bool is_lower(CharT c)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return std::iswlower(static_cast<wint_t>(c));
    else
        return std::islower(static_cast<wint_t>(c));
}

template<typename CharT>
inline bool is_blank(const std::basic_string<CharT> &s)
{
    return s.empty() || std::all_of(s.begin(), s.end(), [](CharT c){ return is_space(c); });
}

// trim from start (in place)
// https://stackoverflow.com/a/217605
template<typename CharT>
inline void ltrim(std::basic_string<CharT> &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](auto ch)
    {
        return !is_space(ch);
    }));
}

// trim from end (in place)
template<typename CharT>
inline void rtrim(std::basic_string<CharT> &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](auto ch)
    {
        return !is_space(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
template<typename CharT>
inline void trim(std::basic_string<CharT> &s)
{
    ltrim(s);
    rtrim(s);
}

template<typename CharT>
bool begins_with(const std::basic_string<CharT> &value, const std::basic_string<CharT> &prefix)
{
    if (prefix.size() > value.size())
        return false;
    
    return std::equal(prefix.begin(), prefix.end(), value.begin());
}

template<typename CharT>
bool ends_with(const std::basic_string<CharT> &value, const std::basic_string<CharT> &suffix)
{
    if (suffix.size() > value.size())
        return false;
    
    return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

template<typename OutT = int, typename CharT>
OutT to_integer(const CharT *value, CharT **pos = nullptr, int base = 10)
{
    static_assert(std::is_integral_v<OutT>, "to_integer number template argument OutT must be a signed or unsigned integer number type");

    if constexpr (std::is_signed_v<OutT>)
    {
        if constexpr (sizeof(OutT) >= sizeof(long long))
        {
            return strtoll(value, pos, base);
        }
        else
        {
            return strtol(value, pos, base);
        }
    }
    else
    {
        if constexpr (sizeof(OutT) >= sizeof(unsigned long long))
        {
            return strtoull(value, pos, base);
        }
        else
        {
            return strtoul(value, pos, base);
        }
    }
}

template<typename OutT = int, typename CharT>
OutT to_integer(const std::basic_string<CharT> &value, size_t *pos = nullptr, int base = 10)
{
    static_assert(std::is_integral_v<OutT>, "to_integer number template argument OutT must be a signed or unsigned integer number type");

    if constexpr (std::is_signed_v<OutT>)
    {
        if constexpr (sizeof(OutT) >= sizeof(long long))
        {
            return stoll(value, pos, base);
        }
        else if constexpr (sizeof(OutT) >= sizeof(long))
        {
            return stol(value, pos, base);
        }
        else
        {
            return stoi(value, pos, base);
        }
    }
    else
    {
        if constexpr (sizeof(OutT) >= sizeof(unsigned long long))
        {
            return stoull(value, pos, base);
        }
        else
        {
            return stoul(value, pos, base);
        }
    }
}

template<typename OutT = float, typename CharT>
OutT to_decimal(const CharT *value, CharT **pos = nullptr)
{
    static_assert(std::is_floating_point_v<OutT>, "to_decimal number template argument OutT must be a floating point number type");

    if constexpr (sizeof(OutT) >= sizeof(long double))
    {
        return strtold(value, pos);
    }
    else if constexpr (sizeof(OutT) >= sizeof(double))
    {
        return strtod(value, pos);
    }
    else
    {
        return strtof(value, pos);
    }
}

template<typename OutT = float, typename CharT>
OutT to_decimal(const std::basic_string<CharT> &value, size_t *pos = nullptr)
{
    static_assert(std::is_floating_point_v<OutT>, "to_decimal number template argument OutT must be a floating point number type");

    if constexpr (sizeof(OutT) >= sizeof(long double))
    {
        return stold(value, pos);
    }
    else if constexpr (sizeof(OutT) >= sizeof(double))
    {
        return stod(value, pos);
    }
    else
    {
        return stof(value, pos);
    }
}

// alias functions
// i fucking hate c++ so much its unreal just give me fucking function aliases
#define DEFINE_INTEGER_ALIAS(T, NAME) \
template<typename CharT> T NAME(const CharT *value, CharT **pos = nullptr, int base = 10)\
{ return to_integer<T, CharT>(value, pos, base); } \
\
template<typename CharT> T NAME(const std::basic_string<CharT> &value, size_t *pos = nullptr, int base = 10)\
{ return to_integer<T, CharT>(value, pos, base); } \

#define DEFINE_DECIMAL_ALIAS(T, NAME) \
template<typename CharT> T NAME(const CharT *value, CharT **pos = nullptr)\
{ return to_decimal<T, CharT>(value, pos); } \
\
template<typename CharT> T NAME(const std::basic_string<CharT> &value, size_t *pos = nullptr)\
{ return to_decimal<T, CharT>(value, pos); }

DEFINE_INTEGER_ALIAS(int, to_int)
DEFINE_INTEGER_ALIAS(long, to_long)
DEFINE_INTEGER_ALIAS(long long, to_long_long)
DEFINE_INTEGER_ALIAS(unsigned int, to_unsigned)
DEFINE_INTEGER_ALIAS(unsigned long, to_unsigned_long)
DEFINE_INTEGER_ALIAS(unsigned long long, to_unsigned_long_long)
DEFINE_DECIMAL_ALIAS(float, to_float)
DEFINE_DECIMAL_ALIAS(double, to_double)
DEFINE_DECIMAL_ALIAS(long double, to_long_double)

#undef DEFINE_INTEGER_ALIAS
#undef DEFINE_DECIMAL_ALIAS

template<typename CharT>
CharT to_upper(CharT c)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return std::towupper(static_cast<wint_t>(c));
    else
        return std::toupper(static_cast<wint_t>(c));
}

template<typename CharT>
void to_upper(std::basic_string<CharT> &s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](CharT c){ return to_upper(c); });
}

template<typename CharT>
CharT to_lower(CharT c)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return std::towlower(static_cast<wint_t>(c));
    else
        return std::tolower(static_cast<wint_t>(c));
}

template<typename CharT>
void to_lower(std::basic_string<CharT> &s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](CharT c){ return to_lower(c); });
}

template<typename CharT>
CharT *copy(const CharT *src, CharT *dst)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return wcscpy(dst, src);
    else
        return strcpy(dst, src);
}

template<typename CharT>
CharT *copy(const CharT *src, CharT *dst, size_t n)
{
    if constexpr (std::is_same_v<CharT, wchar_t>)
        return wcsncpy(dst, src, n);
    else
        return strncpy(dst, src, n);
}
