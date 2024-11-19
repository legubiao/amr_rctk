from launch import LaunchDescription
from launch.substitutions import LaunchConfiguration
from launch.actions import DeclareLaunchArgument
from launch_ros.actions import Node


def generate_launch_description():
    use_sim_time = LaunchConfiguration('use_sim_time', default='true')
    return LaunchDescription([
        DeclareLaunchArgument(
            'use_sim_time',
            default_value='true',
            description='Use simulation (Gazebo) clock if true'),
        Node(
            package='rosbridge_server',
            executable='rosbridge_websocket',
            name='rosbridge_websocket',
            output='screen',
            parameters=[{'use_sim_time': use_sim_time}]
        ),
        Node(
            package='rosapi',
            parameters=[
                {'use_sim_time': use_sim_time},
                {'params_glob': "[*]"}
            ],
            executable='rosapi_node',
            name='rosapi',
        ),

        Node(
            package='amr_rctk',
            parameters=[{'use_sim_time': use_sim_time}],
            executable='pose_publisher',
            name='pose_publisher',
            namespace='amr_rctk'
        )
    ])
