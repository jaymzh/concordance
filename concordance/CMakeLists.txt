file(GLOB SRCS
  ./*.c
)
add_executable(concordance ${SRCS})
target_link_libraries(concordance libconcord)
