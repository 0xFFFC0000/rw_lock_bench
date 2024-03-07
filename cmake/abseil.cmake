
set(abseil_source_dir ${thirdparty_source}/abseil-cpp)

FetchContent_Declare(
  abseil
  SOURCE_DIR ${abseil_source_dir}
  OVERRIDE_FIND_PACKAGE
  SYSTEM
)

block()
  set(ABSL_ENABLE_INSTALL ON)
  FetchContent_MakeAvailable(abseil)
endblock()
set(abseil_INCLUDE_DIR ${abseil_source_dir})
