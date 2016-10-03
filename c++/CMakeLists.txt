include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(dfkcpp
  src/core.cpp)

target_link_libraries(dfkcpp dfk)

set_target_properties(dfkcpp
  PROPERTIES
  VERSION ${DFK_VERSION_MAJOR}.${DFK_VERSION_MINOR}.${DFK_VERSION_PATCH}
  SOVERSION ${DFK_VERSION_MAJOR}.${DFK_VERSION_MINOR})

install(TARGETS dfkcpp DESTINATION lib)

install(DIRECTORY include/dfk DESTINATION include)
install(FILES
  include/dfk.hpp
  DESTINATION include)
