# Provides a minimal `write_config()` implementation for isolated builds.
#
# Some upstream Thunder/entservices build systems provide `write_config` via
# shared CMake include files. In this repo validation flow, those includes
# are not present, so CMake errors out.
#
# This compat macro is intentionally minimal: it will write the provided content
# to the requested file path if arguments are supplied, otherwise it no-ops.

# PUBLIC_INTERFACE
function(write_config)
    # Common calling patterns in Thunder plugin builds vary. We support the two
    # most typical shapes:
    #   write_config(<output_file> <content_string>)
    #   write_config(OUTPUT <output_file> CONTENT <content_string>)
    set(options)
    set(oneValueArgs OUTPUT CONTENT)
    set(multiValueArgs)
    cmake_parse_arguments(WC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(WC_OUTPUT AND WC_CONTENT)
        file(WRITE "${WC_OUTPUT}" "${WC_CONTENT}")
        return()
    endif()

    # Positional fallback: first arg is output file, rest is treated as content.
    if(ARGC GREATER 0)
        set(_out "${ARGV0}")
        if(ARGC GREATER 1)
            # Join remaining args with spaces.
            set(_content "")
            foreach(_i RANGE 1 $<SUBTRACT:${ARGC},1>)
                string(APPEND _content "${ARGV${_i}}")
                if(NOT _i EQUAL $<SUBTRACT:${ARGC},1>)
                    string(APPEND _content " ")
                endif()
            endforeach()
            file(WRITE "${_out}" "${_content}")
        else()
            # No content provided; create empty file.
            file(WRITE "${_out}" "")
        endif()
    endif()
endfunction()
