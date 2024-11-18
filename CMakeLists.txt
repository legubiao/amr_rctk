cmake_minimum_required(VERSION 3.8)
project(amr_rctk)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif ()

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)

find_package(tf2_ros REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
        "srv/FrameToPose.srv"
        "srv/PoseList.srv"
        DEPENDENCIES geometry_msgs
)

# uncomment the following section in order to fill in
# further dependencies manually.
# find_package(<dependency> REQUIRED)

if (BUILD_TESTING)
    find_package(ament_lint_auto REQUIRED)
    # the following line skips the linter which checks for copyrights
    # comment the line when a copyright and license is added to all source files
    set(ament_cmake_copyright_FOUND TRUE)
    # the following line skips cpplint (only works in a git repo)
    # comment the line when this package is in a git repo and when
    # a copyright and license is added to all source files
    set(ament_cmake_cpplint_FOUND TRUE)
    ament_lint_auto_find_test_dependencies()
endif ()
include_directories(include)

add_executable(pose_publisher src/pose_publisher.cpp)
ament_target_dependencies(pose_publisher
        rclcpp
        tf2_ros
        nav_msgs
        geometry_msgs
        tf2_geometry_msgs
)

add_executable(path_publisher src/path_publisher.cpp)
ament_target_dependencies(path_publisher
        rclcpp
        tf2_ros
        nav_msgs
        geometry_msgs
        tf2_geometry_msgs
)

add_executable(path_tracker_pid src/path_tracker_pid.cpp)
ament_target_dependencies(path_tracker_pid
        rclcpp
        tf2_ros
        nav_msgs
        geometry_msgs
        tf2_geometry_msgs
)

rosidl_get_typesupport_target(cpp_typesupport_library ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(pose_publisher ${cpp_typesupport_library})
target_link_libraries(path_tracker_pid ${cpp_typesupport_library})

install(TARGETS
        pose_publisher
        path_publisher
        path_tracker_pid
        DESTINATION lib/${PROJECT_NAME})

install(PROGRAMS
        scripts/mapping_node.py
        DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
        launch
        config
        param
        rviz
        DESTINATION share/${PROJECT_NAME}/
)

ament_package()
