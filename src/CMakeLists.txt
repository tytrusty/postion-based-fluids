set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
SET(pwd ${CMAKE_CURRENT_LIST_DIR})
SET(src "")

AUX_SOURCE_DIRECTORY(${pwd} src)

include_directories(${PROJECT_SOURCE_DIR})

# eigen
# set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
# find_package(Eigen3 3.3 REQUIRED NO_MODULE)

# glfw
find_package(glfw3 REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS})
link_libraries(${GLFW_LIBRARY_DIRS})

# target_link_libraries(${APP_TARGET} imgui)

# opengl
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

# glew
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

add_executable(${APP_TARGET} ${src})
target_link_libraries(
        ${APP_TARGET}
        glfw
        imgui
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARIES}
        ${EXTRA_LIBS}
        # Eigen3::Eigen
)
