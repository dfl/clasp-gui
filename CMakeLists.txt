cmake_minimum_required(VERSION 3.16)
project(clasp-gui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CLASP_GUI_BUILD_EXAMPLES "Build examples" OFF)

# Sources
set(CLASP_GUI_SOURCES
    src/webview.cpp
    src/clap/gui_helper.cpp
)

# Platform-specific sources
if(APPLE)
    list(APPEND CLASP_GUI_SOURCES src/platform/cocoa.mm)
elseif(WIN32)
    list(APPEND CLASP_GUI_SOURCES src/platform/win32.cpp)
else()
    list(APPEND CLASP_GUI_SOURCES src/platform/linux.cpp)
endif()

# Library
add_library(clasp-gui STATIC ${CLASP_GUI_SOURCES})

target_include_directories(clasp-gui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# CHOC (optional, but recommended)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/choc")
    target_include_directories(clasp-gui PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/extern/choc>
    )
    message(STATUS "clasp-gui: Using bundled CHOC")
elseif(DEFINED CHOC_DIR)
    target_include_directories(clasp-gui PUBLIC
        $<BUILD_INTERFACE:${CHOC_DIR}>
    )
    message(STATUS "clasp-gui: Using CHOC from ${CHOC_DIR}")
else()
    message(STATUS "clasp-gui: CHOC not found - WebView will be disabled")
endif()

# CLAP (required for CLAP helper)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/clap/include")
    target_include_directories(clasp-gui PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/extern/clap/include>
    )
elseif(DEFINED CLAP_DIR)
    target_include_directories(clasp-gui PUBLIC
        $<BUILD_INTERFACE:${CLAP_DIR}/include>
    )
elseif(TARGET clap)
    target_link_libraries(clasp-gui PUBLIC clap)
else()
    message(WARNING "clasp-gui: CLAP not found - CLAP helpers will not compile")
endif()

# Platform-specific dependencies
if(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(WEBKIT_FRAMEWORK WebKit REQUIRED)
    target_link_libraries(clasp-gui PUBLIC
        ${COCOA_FRAMEWORK}
        ${WEBKIT_FRAMEWORK}
    )
elseif(WIN32)
    # WebView2 is loaded dynamically by CHOC
elseif(UNIX)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(GTK3 gtk+-3.0)
        pkg_check_modules(WEBKIT2 webkit2gtk-4.0)
        if(GTK3_FOUND AND WEBKIT2_FOUND)
            target_include_directories(clasp-gui PRIVATE
                ${GTK3_INCLUDE_DIRS}
                ${WEBKIT2_INCLUDE_DIRS}
            )
            target_link_libraries(clasp-gui PUBLIC
                ${GTK3_LIBRARIES}
                ${WEBKIT2_LIBRARIES}
            )
        endif()
    endif()
    find_package(X11)
    if(X11_FOUND)
        target_link_libraries(clasp-gui PUBLIC ${X11_LIBRARIES})
    endif()
endif()

# Examples
if(CLASP_GUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install
install(TARGETS clasp-gui
    EXPORT clasp-gui-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/clasp-gui
    DESTINATION include
)

install(EXPORT clasp-gui-targets
    FILE clasp-gui-config.cmake
    NAMESPACE clasp-gui::
    DESTINATION lib/cmake/clasp-gui
)
