set(CMAKE_EXE_LINKER_FLAGS /MANIFEST:NO)
set(CMAKE_SHARED_LINKER_FLAGS /MANIFEST:NO)

add_library (KeyCodeHooker SHARED KeyCodeHooker.cpp KeyCodeHooker.h)
add_executable (KeyCodeBlocker WIN32 KeyCodeBlocker.cpp KeyCodeBlocker.h KeyCodeBlocker.rc)
target_link_libraries (KeyCodeBlocker PRIVATE KeyCodeHooker)

set_source_files_properties(KeyCodeBlocker.rc PROPERTIES LANGUAGE RC)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET KeyCodeBlocker PROPERTY CXX_STANDARD 20)
  set_property(TARGET KeyCodeHooker PROPERTY CXX_STANDARD 20)
endif()

target_compile_definitions (KeyCodeBlocker PUBLIC UNICODE)
target_compile_definitions (KeyCodeBlocker PUBLIC _UNICODE)

target_compile_definitions (KeyCodeHooker PUBLIC UNICODE)
target_compile_definitions (KeyCodeHooker PUBLIC _UNICODE)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
