
cmake_minimum_required(VERSION 3.12)
project(LinuxCommon CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(UTIL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(helper/nlohmann_json)

file(GLOB UTIL_INSTALL_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

file(GLOB SUB1_INSTALL_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

if(LIB_NOT_MERGE)

    install(FILES ${UTIL_INSTALL_HEADERS}
        DESTINATION include/haris/linux/common/util
    )

else()

    add_library(command_util STATIC
        $<TARGET_OBJECTS:json_helper_object>
    )

    # 3. CONFIGURE INSTALLATION TARGETS
    install(TARGETS command_util
        ARCHIVE DESTINATION lib
    )

    install(FILES ${UTIL_INSTALL_HEADERS}
        DESTINATION include/haris/linux/common/util
    )

    install(FILES ${UTIL_INSTALL_HEADERS}
        DESTINATION include/haris/linux/common/util
    )

endif()
