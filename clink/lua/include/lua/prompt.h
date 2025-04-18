// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class lua_state;
class str_base;

//------------------------------------------------------------------------------
class prompt
{
public:
                    prompt();
                    prompt(prompt&& rhs);
                    prompt(const prompt& rhs) = delete;
                    ~prompt();
    prompt&         operator = (prompt&& rhs);
    prompt&         operator = (const prompt& rhs) = delete;
    void            clear();
    const wchar_t*  get() const;
    void            set(const wchar_t* chars, int32 char_count=0);
    bool            is_set() const;

protected:
    wchar_t*        m_data;
};

//------------------------------------------------------------------------------
class tagged_prompt
    : public prompt
{
public:
    void            set(const wchar_t* chars, int32 char_count=0);
    void            tag(const wchar_t* value);

private:
    int32           is_tagged(const wchar_t* chars, int32 char_count=0);
};

//------------------------------------------------------------------------------
class prompt_filter
{
public:
                    prompt_filter(lua_state& lua);
    bool            filter(const char* in, str_base& out); // For unit tests.
    bool            filter(const char* in, const char* rin, str_base& out, str_base& rout, bool transient=false, bool final=false);

    static bool     is_filtering() { return s_filtering; }
    static bool     is_transient_filtering() { return s_transient_filtering; }

private:
    lua_state&      m_lua;

    static bool s_filtering;
    static bool s_transient_filtering;
};

//------------------------------------------------------------------------------
enum class expand_prompt_flags
{
    none        = 0x00,
    single_line = 0x01,
    omit_pushd  = 0x02,
};
DEFINE_ENUM_FLAG_OPERATORS(expand_prompt_flags);

//------------------------------------------------------------------------------
class prompt_utils
{
public:
    static prompt   extract_from_console();
    static void     get_rprompt(str_base& rout);
    static void     get_transient_prompt(str_base& out);
    static void     get_transient_rprompt(str_base& rout);
    static bool     expand_prompt_codes(const char* in, str_base& out, expand_prompt_flags flags);
};
