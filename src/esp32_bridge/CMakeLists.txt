cmake_minimum_required(VERSION 3.8)
project(esp32_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
rosidl_generate_interfaces(${PROJECT_NAME} "msg/Imu.msg")
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

add_executable(udp_bridge src/udp_bridge.cpp)
ament_target_dependencies(udp_bridge rclcpp std_msgs rosidl_default_runtime)
target_link_libraries(udp_bridge "${cpp_typesupport_target}")
install(TARGETS udp_bridge DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

add_executable(imu_subscriber src/imu_subscriber.cpp)
ament_target_dependencies(imu_subscriber rclcpp std_msgs rosidl_default_runtime)
target_link_libraries(imu_subscriber "${cpp_typesupport_target}")
install(TARGETS imu_subscriber DESTINATION lib/${PROJECT_NAME})

add_executable(stoper_publisher src/stoper_publisher.cpp)
ament_target_dependencies(stoper_publisher rclcpp std_msgs rosidl_default_runtime)
target_link_libraries(stoper_publisher "${cpp_typesupport_target}")
install(TARGETS stoper_publisher DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY msg DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()



ament_package()
