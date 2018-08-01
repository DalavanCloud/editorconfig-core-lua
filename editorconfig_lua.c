/*
 * Copyright (c) 2016 João Valverde <joao.valverde@tecnico.ulisboa.pt>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <editorconfig/editorconfig.h>
#include "lua.h"
#include "lauxlib.h"

#ifndef LEC_VERSION
#error "LEC_VERSION is not defined."
#endif

#define strequal(s1, s2) \
    (strcmp((s1), (s2)) == 0)

#define E_OK         1
#define E_NULL       0
#define E_ERROR     -1

typedef int err_t;


static err_t
push_ec_boolean(lua_State *L, const char *value)
{
    if (strequal(value, "true")) {
        lua_pushboolean(L, 1);
        return E_OK;
    }
    if (strequal(value, "false")) {
        lua_pushboolean(L, 0);
        return E_OK;
    }
    return E_ERROR;
}

static err_t
push_ec_number(lua_State *L, const char *value)
{
    long number;
    const char *nptr = value;
    char *endptr = NULL;

    number = strtol(nptr, &endptr, 0);
    if (*nptr == '\0' || *endptr != '\0') {
        /* Accept all digits in dec/hex/octal only */
        return E_ERROR;
    }
    if (number <= 0) {
        /* Accept positive integers only */
        return E_ERROR;
    }
    lua_pushinteger(L, (lua_Integer)number);
    return E_OK;
}

static void
push_ec_value(lua_State *L, const char *value)
{
    if (push_ec_boolean(L, value) == E_OK)
        return;
    if (push_ec_number(L, value) == E_OK)
        return;
    lua_pushstring(L, value);
}

/* Receives 3 arguments, two optional */
static editorconfig_handle
open_ec_handle(lua_State *L)
{
    const char *file_full_path;
    const char *conf_file_name;
    const char *version_to_set;
    int major = -1, minor = -1, patch = -1;
    editorconfig_handle eh;
    int err_num;

    file_full_path = luaL_checkstring(L, 1);
    conf_file_name = luaL_opt(L, luaL_checkstring, 2, NULL);
    version_to_set = luaL_opt(L, luaL_checkstring, 3, NULL);
    eh = editorconfig_handle_init();
    if (eh == NULL) {
        luaL_error(L, "not enough memory to create handle");
    }
    if (conf_file_name != NULL) {
        editorconfig_handle_set_conf_file_name(eh, conf_file_name);
    }
    if (version_to_set != NULL) {
        sscanf(version_to_set, "%d.%d.%d", &major, &minor, &patch);
        editorconfig_handle_set_version(eh, major, minor, patch);
    }
    err_num = editorconfig_parse(file_full_path, eh);
    if (err_num != 0) {
        const char *err_msg = editorconfig_get_error_msg(err_num);
        if (err_num > 0) {
            const char *err_file = editorconfig_handle_get_err_file(eh);
            luaL_error(L, "'%s' at line %d: %s",
                        err_file ? err_file : "<null>", err_num, err_msg);
        }
        luaL_error(L, "%s", err_msg);
    }
    return eh;
}

/* One mandatory argument (file_full_path) */
/* One optional argument (conf_file_name) */
/* One optional argument (version_to_set) */
/* Returns two tables: { names = values }, { names } */
static int
lec_parse(lua_State *L)
{
    editorconfig_handle eh;
    int name_value_count;
    const char *name, *value;
    lua_Integer idx = 1;

    eh = open_ec_handle(L);
    assert(eh != NULL);
    lua_settop(L, 0);
    name_value_count = editorconfig_handle_get_name_value_count(eh);
    lua_createtable(L, 0, name_value_count);
    lua_createtable(L, name_value_count, 0);
    for (int i = 0; i < name_value_count; i++) {
        name = value = NULL;
        editorconfig_handle_get_name_value(eh, i, &name, &value);
        lua_pushstring(L, name);
        push_ec_value(L, value);
        lua_settable(L, 1);
        lua_pushinteger(L, idx);
        lua_pushstring(L, name);
        lua_settable(L, 2);
        idx += 1;
    }
    editorconfig_handle_destroy(eh);
    return 2;
}

static void
add_version(lua_State *L)
{
    int major = 0, minor = 0, patch = 0;
    const char *fmt;

    fmt = "EditorConfig Lua Core Version %s";
    lua_pushfstring(L, fmt, LEC_VERSION);
    lua_setfield(L, -2, "_VERSION");
    editorconfig_get_version(&major, &minor, &patch);
    fmt = "EditorConfig C Core Version %d.%d.%d";
    lua_pushfstring(L, fmt, major, minor, patch);
    lua_setfield(L, -2, "_C_VERSION");
}

static const struct luaL_Reg editorconfig_core[] = {
    {"parse", lec_parse},
    {NULL, NULL}
};

int luaopen_editorconfig_core (lua_State *L) {
    luaL_newlib(L, editorconfig_core);
    add_version(L);
    return 1;
}
