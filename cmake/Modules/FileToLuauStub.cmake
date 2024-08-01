# Taken from stackoverflow; licensed under CC BY SA 3.0; orig. author Youka
# Modified by checkraisefold
# https://stackoverflow.com/a/27206982 https://creativecommons.org/licenses/by-sa/3.0/

# Creates a Luau loader stub from a given luac file
function(create_luau_stub file_name output lua_file)
    if (NOT IS_ABSOLUTE ${file_name})
        set(file_name "${CMAKE_CURRENT_SOURCE_DIR}/${file_name}")
    endif ()
    get_filename_component(basedir_input ${file_name} PATH)
    file(MAKE_DIRECTORY ${basedir_input})

    if (NOT IS_ABSOLUTE ${output})
        set(output "${CMAKE_CURRENT_SOURCE_DIR}/${output}")
    endif ()
    get_filename_component(basedir_output ${output} PATH)
    file(MAKE_DIRECTORY ${basedir_output})

    # Create empty output file
    file(WRITE ${output} "")

    # Get short filename
    string(REGEX MATCH "([^/\\]+)$" file_name_short ${lua_file})
    # Get short filename with no extension
    string(REGEX MATCH "[^\\.]+" file_name_short_noext ${file_name_short})
    # Read hex data from file
    file(READ ${file_name} filedata HEX)

    # Convert hex data for C compatibility
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})

    # Append data to output file
    #file(APPEND ${output} "const unsigned char ${filename}[] = {${filedata}};\nconst unsigned ${filename}_size = sizeof(${filename});\n")
    file(APPEND ${output} "\
/* generated source for Lua codes */\n \
\n \
#include <lua.h>\n \
\n \
LUALIB_API int luaopen_${file_name_short_noext}(lua_State *L) {\n \
    const char chunk[] = {${filedata}};\n \
    size_t len = sizeof(chunk);\n \
    \n \
    if (luau_load(L, \"${file_name_short}\", chunk, len, 0) != 0)\n \
        lua_error(L);\n \
    lua_insert(L, 1);\n \
    lua_call(L, lua_gettop(L)-1, LUA_MULTRET);\n \
    return lua_gettop(L);\n \
}\n \
    ")
endfunction()

create_luau_stub(${INPUT_LUAU_FILE} ${OUTPUT_C_FILE} ${REAL_LUA_FILE})