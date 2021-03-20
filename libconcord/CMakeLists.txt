find_package(libzip REQUIRED)
find_package(HIDAPI REQUIRED)

file(GLOB SRCS
  ./*.cpp
)
add_library(libconcord ${SRCS})
target_link_libraries(libconcord PUBLIC libzip::zip HIDAPI::hidapi)
target_include_directories(libconcord PUBLIC ./)

target_compile_definitions(libconcord PRIVATE
  #-DWANT_LIBUSB=1
  -DWANT_HIDAPI=1
)
