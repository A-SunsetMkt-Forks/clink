// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "match_builder_lua.h"

#include <core/base.h>
#include <core/str.h>
#include <lib/matches.h>

//------------------------------------------------------------------------------
static match_builder_lua::method g_methods[] = {
    { "addmatch",           &match_builder_lua::add_match },
    { "addmatches",         &match_builder_lua::add_matches },
    { "setprefixincluded",  &match_builder_lua::set_prefix_included },
    {}
};



//------------------------------------------------------------------------------
static const char* get_string(lua_State* state, int index)
{
    if (lua_gettop(state) < index || !lua_isstring(state, index))
        return nullptr;

    return lua_tostring(state, index);
}



//------------------------------------------------------------------------------
match_builder_lua::match_builder_lua(match_builder& builder)
: lua_bindable<match_builder_lua>("match_builder_lua", g_methods)
, m_builder(builder)
{
}

//------------------------------------------------------------------------------
match_builder_lua::~match_builder_lua()
{
}

//------------------------------------------------------------------------------
/// -name:  builder:addmatch
/// -arg:   match:string|table
/// -arg:   [type:string]
/// -ret:   boolean
/// -show:  builder:addmatch({ match="some_word", type="word" })
/// -show:  builder:addmatch({ match="abbrev", type="alias" })
/// -show:  builder:addmatch({ match="foo.cpp", type="file" })
/// -show:  builder:addmatch({ match="bar", type="dir" })
/// -show:  builder:addmatch({ match=".git", type="dir hidden" })
/// Adds a match.  If <em>match</em> is a string, in which case it's added as a
/// match and <em>type</em> (or "none") is the match type.  Or <em>match</em>
/// can be a table with the following scheme: <em>{ match:string, [suffix:string],
/// [type:string] }</em>.  If <em>suffix</em> is provided, the first character
/// is used as a suffix when completing the match (for example "%" for env var
/// matches).  If <em>type</em> is not provided then "none" is used, otherwise
/// <em>type</em> can be "word", "alias" (doskey macro), "file", "dir", or
/// "link" (symlink).<br/>
/// <br/>
/// The match type influences the color when listing possible matches, and files
/// and dirs can also include "hidden" and/or "readonly" in the type string.
/// The match type also affects how the match is displayed:  "word" matches show
/// the whole word even if it contains slashes, "file" and "dir" matches only
/// show the last path component (text after the last slash, if any), and "dir"
/// matches show a trailing path separator.
int match_builder_lua::add_match(lua_State* state)
{
    int ret = 0;
    if (lua_gettop(state) > 0)
    {
        match_type type = to_match_type(get_string(state, 2));
        ret = !!add_match_impl(state, 1, type);
    }

    lua_pushboolean(state, ret);
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  builder:setprefixincluded
/// -arg:   [state:boolean]
/// Sets whether the prefix from the match generator should be included as part
/// of the matches.  For example the env var match generator uses this to make
/// "%" be part of the completed match.
int match_builder_lua::set_prefix_included(lua_State* state)
{
    bool included = true;
    if (lua_gettop(state) > 0)
        included = (lua_toboolean(state, 1) != 0);

    m_builder.set_prefix_included(included);

    return 0;
}

//------------------------------------------------------------------------------
/// -name:  builder:addmatches
/// -arg:   matches:table
/// -arg:   [type:string]
/// -ret:   integer, boolean
/// -show:  builder:addmatches({"abc", "def"}) -- Adds two matches of type "none"
/// -show:  builder:addmatches({"abc", "def"}, "file") -- Adds two matches of type "file"
/// -show:  builder:addmatches({
/// -show:  &nbsp;&nbsp;-- Same table scheme per entry here as in builder:addmatch()
/// -show:  &nbsp;&nbsp;{ match="remote/origin/master", type="word" },
/// -show:  &nbsp;&nbsp;{ match="remote/origin/topic", type="word" }
/// -show:  })
/// This is the equivalent of calling <code>builder:addmatch()</code> in a
/// for-loop. Returns the number of matches added and a boolean indicating if
/// all matches were added successfully.<br/>
/// <br/>
/// <em>matches</em> can be a table of match strings, or a table of tables
/// describing the matches.<br/>
/// <em>type</em> is used as the type when a match doesn't explicitly include a
/// type, and is "none" if omitted.
int match_builder_lua::add_matches(lua_State* state)
{
    if (lua_gettop(state) <= 0 || !lua_istable(state, 1))
    {
        lua_pushinteger(state, 0);
        lua_pushboolean(state, 0);
        return 2;
    }

    match_type type = to_match_type(get_string(state, 2));

    int count = 0;
    int total = int(lua_rawlen(state, 1));
    for (int i = 1; i <= total; ++i)
    {
        lua_rawgeti(state, 1, i);
        count += !!add_match_impl(state, -1, type);
        lua_pop(state, 1);
    }

    lua_pushinteger(state, count);
    lua_pushboolean(state, count == total);
    return 2;
}

//------------------------------------------------------------------------------
bool match_builder_lua::add_match_impl(lua_State* state, int stack_index, match_type type)
{
    if (lua_isstring(state, stack_index))
    {
        const char* match = lua_tostring(state, stack_index);
        return m_builder.add_match(match, type);
    }
    else if (lua_istable(state, stack_index))
    {
        if (stack_index < 0)
            --stack_index;

        match_desc desc = {};
        desc.type = type;

        lua_pushliteral(state, "match");
        lua_rawget(state, stack_index);
        if (lua_isstring(state, -1))
            desc.match = lua_tostring(state, -1);
        lua_pop(state, 1);

#ifdef NYI_MATCHES
        lua_pushliteral(state, "displayable");
        lua_rawget(state, stack_index);
        if (lua_isstring(state, -1))
            desc.displayable = lua_tostring(state, -1);
        lua_pop(state, 1);

        lua_pushliteral(state, "aux");
        lua_rawget(state, stack_index);
        if (lua_isstring(state, -1))
            desc.aux = lua_tostring(state, -1);
        lua_pop(state, 1);
#endif

        lua_pushliteral(state, "suffix");
        lua_rawget(state, stack_index);
        if (lua_isstring(state, -1))
            desc.suffix = lua_tostring(state, -1)[0];
        lua_pop(state, 1);

        lua_pushliteral(state, "type");
        lua_rawget(state, stack_index);
        if (lua_isstring(state, -1))
            desc.type = to_match_type(lua_tostring(state, -1));
        lua_pop(state, 1);

        if (desc.match != nullptr)
            return m_builder.add_match(desc);
    }

    return false;
}
