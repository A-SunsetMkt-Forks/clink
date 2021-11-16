// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "line_state_lua.h"

#include <core/array.h>
#include <lib/line_state.h>

//------------------------------------------------------------------------------
static line_state_lua::method g_methods[] = {
    { "getline",                &line_state_lua::get_line },
    { "getcursor",              &line_state_lua::get_cursor },
    { "getcommandoffset",       &line_state_lua::get_command_offset },
    { "getcommandwordindex",    &line_state_lua::get_command_word_index },
    { "getwordcount",           &line_state_lua::get_word_count },
    { "getwordinfo",            &line_state_lua::get_word_info },
    { "getword",                &line_state_lua::get_word },
    { "getendword",             &line_state_lua::get_end_word },
    {}
};



//------------------------------------------------------------------------------
line_state_lua::line_state_lua(const line_state& line)
: lua_bindable("line_state", g_methods)
, m_line(line)
{
}

//------------------------------------------------------------------------------
/// -name:  line_state:getline
/// -ver:   1.0.0
/// -ret:   string
/// Returns the current line in its entirety.
int line_state_lua::get_line(lua_State* state)
{
    lua_pushstring(state, m_line.get_line());
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  line_state:getcursor
/// -ver:   1.0.0
/// -ret:   integer
/// Returns the position of the cursor.
int line_state_lua::get_cursor(lua_State* state)
{
    lua_pushinteger(state, m_line.get_cursor() + 1);
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  line_state:getcommandoffset
/// -ver:   1.0.0
/// -ret:   integer
/// Returns the offset to the start of the delimited command in the line that's
/// being effectively edited. Note that this may not be the offset of the first
/// command of the line unquoted as whitespace isn't considered for words.
/// -show:  -- Given the following line; abc& 123
/// -show:  -- where commands are separated by & symbols.
/// -show:  line_state:getcommandoffset() == 4
int line_state_lua::get_command_offset(lua_State* state)
{
    lua_pushinteger(state, m_line.get_command_offset() + 1);
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  line_state:getcommandwordindex
/// -ver:   1.2.27
/// -ret:   integer
/// Returns the index of the command word. Usually the index is 1, but if a
/// redirection symbol occurs before the command name then the index can be
/// greater than 1.
/// -show:  -- Given the following line; >x abc
/// -show:  -- the first word is "x" and is an argument to the redirection symbol,
/// -show:  -- and the second word is "abc" and is the command word.
/// -show:  line_state:getcommandwordindex() == 2
int line_state_lua::get_command_word_index(lua_State* state)
{
    lua_pushinteger(state, m_line.get_command_word_index() + 1);
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  line_state:getwordcount
/// -ver:   1.0.0
/// -ret:   integer
/// Returns the number of words in the current line.
int line_state_lua::get_word_count(lua_State* state)
{
    lua_pushinteger(state, m_line.get_word_count());
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  line_state:getwordinfo
/// -ver:   1.0.0
/// -arg:   index:integer
/// -ret:   table
/// Returns a table of information about the Nth word in the line.
///
/// Note:  The length refers to the substring in the line; it omits leading and
/// trailing quotes, but <em><strong>includes</strong></em> embedded quotes.
/// <a href="#line_state:getword">line_state:getword()</a> conveniently strips
/// embedded quotes to help generators naturally complete <code>"foo\"ba</code>
/// to <code>"foo\bar"</code>.
///
/// The table returned has the following scheme:
/// -show:  local t = line_state:getwordinfo(word_index)
/// -show:  -- t.offset     [integer] Offset where the word starts in the line_state:getline() string.
/// -show:  -- t.length     [integer] Length of the word (includes embedded quotes).
/// -show:  -- t.quoted     [boolean] Indicates whether the word is quoted.
/// -show:  -- t.delim      [string] The delimiter character, or an empty string.
/// -show:  -- t.alias      [boolean | nil] true if the word is a doskey alias, otherwise nil.
/// -show:  -- t.redir      [boolean | nil] true if the word is a redirection arg, otherwise nil.
int line_state_lua::get_word_info(lua_State* state)
{
    if (!lua_isnumber(state, 1))
        return 0;

    const std::vector<word>& words = m_line.get_words();
    unsigned int index = int(lua_tointeger(state, 1)) - 1;
    if (index >= words.size())
        return 0;

    const word& word = words[index];

    lua_createtable(state, 0, 6);

    lua_pushliteral(state, "offset");
    lua_pushinteger(state, word.offset + 1);
    lua_rawset(state, -3);

    lua_pushliteral(state, "length");
    lua_pushinteger(state, word.length);
    lua_rawset(state, -3);

    lua_pushliteral(state, "quoted");
    lua_pushboolean(state, word.quoted);
    lua_rawset(state, -3);

    char delim[2] = { char(word.delim) };
    lua_pushliteral(state, "delim");
    lua_pushstring(state, delim);
    lua_rawset(state, -3);

    if (word.is_alias)
    {
        lua_pushliteral(state, "alias");
        lua_pushboolean(state, true);
        lua_rawset(state, -3);
    }

    if (word.is_redir_arg)
    {
        lua_pushliteral(state, "redir");
        lua_pushboolean(state, true);
        lua_rawset(state, -3);
    }

    return 1;
}

//------------------------------------------------------------------------------
/// -name:  line_state:getword
/// -ver:   1.0.0
/// -arg:   index:integer
/// -ret:   string
/// Returns the word of the line at <span class="arg">index</span>.
///
/// Note:  The returned word omits any quotes.  This helps generators naturally
/// complete <code>"foo\"ba</code> to <code>"foo\bar"</code>.  The raw word
/// including quotes can be obtained using the <code>offset</code> and
/// <code>length</code> fields from
/// <a href="#line_state:getwordinfo">line_state:getwordinfo()</a> to extract a
/// substring from the line returned by
/// <a href="#line_state:getline">line_state:getline()</a>.
int line_state_lua::get_word(lua_State* state)
{
    if (!lua_isnumber(state, 1))
        return 0;

    str<32> word;
    unsigned int index = int(lua_tointeger(state, 1)) - 1;
    m_line.get_word(index, word);
    lua_pushlstring(state, word.c_str(), word.length());
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  line_state:getendword
/// -ver:   1.0.0
/// -ret:   string
/// Returns the last word of the line. This is the word that matches are being
/// generated for.
///
/// Note:  The returned word omits any quotes.  This helps generators naturally
/// complete <code>"foo\"ba</code> to <code>"foo\bar"</code>.  The raw word
/// including quotes can be obtained using the <code>offset</code> and
/// <code>length</code> fields from
/// <a href="#line_state:getwordinfo">line_state:getwordinfo()</a> to extract a
/// substring from the line returned by
/// <a href="#line_state:getline">line_state:getline()</a>.
/// -show:  line_state:getword(line_state:getwordcount()) == line_state:getendword()
int line_state_lua::get_end_word(lua_State* state)
{
    str<32> word;
    m_line.get_end_word(word);
    lua_pushlstring(state, word.c_str(), word.length());
    return 1;
}