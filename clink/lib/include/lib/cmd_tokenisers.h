// Copyright (c) 2021 Christopher Antos
// License: http://opensource.org/licenses/MIT

#pragma once

#include "word_collector.h"

//------------------------------------------------------------------------------
#ifndef _ENUM_FLAG_CONSTEXPR // Because MINGW is missing this.
# if _MSC_VER >= 1900
#  define _ENUM_FLAG_CONSTEXPR constexpr
# else
#  define _ENUM_FLAG_CONSTEXPR
# endif
#endif

//------------------------------------------------------------------------------
enum tokeniser_state : int32;

//------------------------------------------------------------------------------
enum state_flag
{
    flag_none               = 0x00,
    flag_internal           = 0x01,
    flag_specialwordbreaks  = 0x02,
    flag_rem                = 0x04,
};
DEFINE_ENUM_FLAG_OPERATORS(state_flag);
inline _ENUM_FLAG_CONSTEXPR bool operator ! (state_flag a) throw() { return a == flag_none; }

//------------------------------------------------------------------------------
class cmd_state
{
public:
    cmd_state(bool only_rem=false) : m_only_rem(only_rem) {}
    void clear(bool first);
    void next_word();
    bool test(int32 c, tokeniser_state new_state);
    bool is_first() const { return m_first; }
    void cancel() { m_failed = true; }
private:
    str<16> m_word;
    bool m_first = false;
    bool m_failed = true;
    bool m_match = false;
    state_flag m_match_flag = flag_none;
    const bool m_only_rem;
    static const char* const c_command_delimiters;
};

//------------------------------------------------------------------------------
class cmd_tokeniser_impl : public collector_tokeniser
{
public:
    cmd_tokeniser_impl();
    ~cmd_tokeniser_impl();
    void begin_line();
    void start(const str_iter& iter, const char* quote_pair, bool at_beginning=true) override;
protected:
    char get_opening_quote() const;
    char get_closing_quote() const;
protected:
    str_iter m_iter;
    const char* m_start;
    const char* m_quote_pair;
    alias_cache* m_alias_cache = nullptr;
    bool m_next_redir_arg;
};

//------------------------------------------------------------------------------
class cmd_command_tokeniser : public cmd_tokeniser_impl
{
public:
    word_token next(uint32& offset, uint32& length) override;
    bool has_deprecated_argmatcher(char const* command) override;
};

//------------------------------------------------------------------------------
class cmd_word_tokeniser : public cmd_tokeniser_impl
{
    typedef cmd_tokeniser_impl base;
public:
    void start(const str_iter& iter, const char* quote_pair, bool at_beginning=true) override;
    word_token next(uint32& offset, uint32& length) override;
private:
    cmd_state m_cmd_state;
    bool m_command_word;
};

//------------------------------------------------------------------------------
state_flag is_cmd_command(const char* word);
int32 skip_leading_parens(str_iter& iter, bool& first, alias_cache* alias_cache=nullptr);
uint32 trim_trailing_parens(const char* start, uint32 offset, uint32 length, int32 parens);

//------------------------------------------------------------------------------
extern const char* const c_cmd_exes[];
extern const char* const c_cmd_commands_basicwordbreaks[];
extern const char* const c_cmd_commands_shellwordbreaks[];
