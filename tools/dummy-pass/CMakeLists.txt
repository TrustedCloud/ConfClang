add_clang_executable(dummy-pass DummyPass.cpp)
target_link_libraries(dummy-pass clangTooling)
