
set(gflags_source_dir ${thirdparty_source}/gflags)

FetchContent_Declare(
  gflags
  SOURCE_DIR ${gflags_source_dir}
  OVERRIDE_FIND_PACKAGE
)

block()
  
  if(BUILD_STATIC_LIBS)
    # Fix this bug
    set(INSTALL_STATIC_LIBS ON)
  endif()
  if(BUILD_SHARED_LIBS)
    set(INSTALL_SHARED_LIBS ON)
  endif()

#  set(INSTALL_HEADERS ON)
  set(BUILD_TESTING OFF)
  set(BUILD_gflags_LIB ON)
  FetchContent_MakeAvailable(gflags)
endblock()
set(gflags_INCLUDE_DIR ${gflags_BINARY_DIR}/include)  
