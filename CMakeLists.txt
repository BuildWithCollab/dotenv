cmake_minimum_required(VERSION 3.20)
project(dotenv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CMakePackageConfigHelpers)

find_package(collab-core CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_library(dotenv STATIC
    src/expand.cpp
    src/load.cpp
    src/merge.cpp
    src/parse_dotenv.cpp
    src/parse_json.cpp
    src/parse_yaml.cpp
)

target_include_directories(dotenv PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(dotenv PRIVATE src)
target_link_libraries(dotenv PUBLIC collab::collab-core nlohmann_json::nlohmann_json yaml-cpp::yaml-cpp)

install(TARGETS dotenv EXPORT dotenv-targets ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT dotenv-targets
    FILE dotenv-targets.cmake
    NAMESPACE collab::
    DESTINATION share/dotenv
)

configure_package_config_file(
    cmake/dotenv-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/dotenv-config.cmake"
    INSTALL_DESTINATION share/dotenv
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/dotenv-config.cmake"
    DESTINATION share/dotenv
)
