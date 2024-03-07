
set(glog_source_dir ${thirdparty_source}/glog)

FetchContent_Declare(
  glog
  SOURCE_DIR ${glog_source_dir}
  OVERRIDE_FIND_PACKAGE
)

block()
  set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
  set(WITH_GFLAGS OFF)
  FetchContent_MakeAvailable(glog)
endblock()
set(glog_INCLUDE_DIR ${glog_BINARY_DIR} ${glog_SOURCE_DIR}/src)
