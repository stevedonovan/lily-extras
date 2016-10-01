include_directories("/usr/local/include/lily")

add_library(xstr SHARED lily_xstr.c)
set_target_properties(
    xstr
    PROPERTIES
        PREFIX ""
        COMPILE_FLAGS "-fPIC -g"
)

