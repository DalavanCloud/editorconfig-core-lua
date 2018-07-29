package = "editorconfig-core"
version = "scm-1"
source = {
    url = "git://github.com/editorconfig/editorconfig-core-lua.git"
}
description = {
    summary = "EditorConfig support for the Lua language",
    detailed = [[
EditorConfig makes it easy to maintain the correct coding style when switching between different text editors and between different projects. The EditorConfig project maintains a file format and plugins for various text editors which allow this file format to be read and used by those editors. EditorConfig Lua Core provides the same functionality as the Editorconfig C Core library.
]],
    homepage = "http://editorconfig.org",
    license = "BSD",
}
dependencies = {
    "lua >= 5.2",
}
build = {
    type = "cmake",
    variables = {
        CMAKE_INSTALL_PREFIX = "$(PREFIX)",
        ECL_LIBDIR = "$(LIBDIR)",
    },
}
