add_library(procon2droid_common STATIC
    common/device_finder.cpp
    common/haptic_report.cpp
)

target_include_directories(procon2droid_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(procon2droid_common PUBLIC
    spdlog::spdlog
)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(procon2droid_common PRIVATE /W4 /WX)
else()
    target_compile_options(procon2droid_common PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_link_options(procon2droid_common PUBLIC -static-libstdc++)
endif()

# Main binaries are Linux/Android only (require kernel headers)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_executable(enable
        enable/main.cpp
    )

    target_link_libraries(enable PRIVATE procon2droid_common)

    target_compile_options(enable PRIVATE
        -Wall -Wextra -Wpedantic -Werror
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "Android")
        set_target_properties(enable PROPERTIES
            LINK_FLAGS "-static-libstdc++"
        )
        add_custom_command(TARGET enable POST_BUILD
            COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:enable>
        )
    endif()

    add_executable(rumble
        rumble/main.cpp
        rumble/uinput_proxy.cpp
    )

    target_link_libraries(rumble PRIVATE procon2droid_common)

    target_compile_options(rumble PRIVATE
        -Wall -Wextra -Wpedantic -Werror
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "Android")
        set_target_properties(rumble PROPERTIES
            LINK_FLAGS "-static-libstdc++"
        )
        add_custom_command(TARGET rumble POST_BUILD
            COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:rumble>
        )
    endif()
endif()
