# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles\\bms_host_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\bms_host_autogen.dir\\ParseCache.txt"
  "bms_host_autogen"
  )
endif()
