cmake_minimum_required(VERSION 3.6)

project(SimpleIPC LANGUAGES C VERSION 0.0.1)

set(export_dest "lib/${PROJECT_NAME}-${PROJECT_VERSION}")
set(include_dest "include/${PROJECT_NAME}-${PROJECT_VERSION}")

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE
    $<INSTALL_INTERFACE:${include_dest}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME} DESTINATION "${export_dest}")
install(EXPORT ${PROJECT_NAME} DESTINATION "${export_dest}")
install(DIRECTORY include DESTINATION ${include_dest})
install(FILES ${PROJECT_NAME}Config.cmake DESTINATION ${export_dest})
