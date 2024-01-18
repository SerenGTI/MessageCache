include(ExternalProject)
include(GNUInstallDirs)

ExternalProject_Add(asio-project
  PREFIX deps/asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-26-0
  BUILD_COMMAND ""
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND ""
  INSTALL_COMMAND cmake -E copy_directory <SOURCE_DIR> <INSTALL_DIR>
  DOWNLOAD_NO_PROGRESS ON
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
ExternalProject_Get_Property(asio-project INSTALL_DIR)

set(ASIO_INCLUDE_DIR ${INSTALL_DIR}/asio/include)

# configure asio
add_definitions(-DASIO_STANDALONE)
add_definitions(-DASIO_HAS_STD_ADDRESSOF)
add_definitions(-DASIO_HAS_STD_ARRAY)
add_definitions(-DASIO_HAS_CSTDINT)
# add_definitions(-DASIO_HAS_CO_AWAIT)
add_definitions(-DASIO_HAS_DECLTYPE)
add_definitions(-DASIO_HAS_MOVE)
add_definitions(-DASIO_HAS_NOEXCEPT)
add_definitions(-DASIO_HAS_NULLPTR)
# add_definitions(-DASIO_HAS_STD_COROUTINE)
add_definitions(-DASIO_HAS_STD_EXCEPTION_PTR)
add_definitions(-DASIO_HAS_CONSTEXPR)
add_definitions(-DASIO_HAS_STD_SHARED_PTR)
add_definitions(-DASIO_HAS_STD_TYPE_TRAITS)
add_definitions(-DASIO_HAS_VARIADIC_TEMPLATES)
add_definitions(-DASIO_HAS_STD_FUNCTION)
add_definitions(-DASIO_HAS_STD_CHRONO)
add_definitions(-DASIO_HAS_STD_TUPLE)
add_definitions(-DASIO_HAS_STD_INVOKE_RESULT)

unset(INSTALL_DIR)
