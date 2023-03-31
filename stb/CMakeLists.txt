add_library(stb
    ${CMAKE_CURRENT_LIST_DIR}/stb_image.h
    ${CMAKE_CURRENT_LIST_DIR}/stb_image.cpp
    ${CMAKE_CURRENT_LIST_DIR}/stb_vorbis.c
    ${CMAKE_CURRENT_LIST_DIR}/stb_implementation.c
)

target_include_directories(stb PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_compile_definitions(stb PUBLIC "STBI_WINDOWS_UTF8")

if(MSVC)
    target_compile_options(stb PRIVATE /W0)
endif()

set_target_folder(stb "Third")
group_sources(stb ${CMAKE_CURRENT_LIST_DIR})
