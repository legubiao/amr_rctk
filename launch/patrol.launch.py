#!/usr/bin/env python3

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch.conditions import IfCondition
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    """
    启动巡逻节点和RViz可视化
    """
    
    # 声明launch参数
    config_file_arg = DeclareLaunchArgument(
        'config_file',
        default_value=PathJoinSubstitution([
            FindPackageShare('amr_rctk'),
            'config',
            'patrol',
            'patrol_points.yaml'
        ]),
        description='巡逻点配置文件路径'
    )
    
    use_rviz_arg = DeclareLaunchArgument(
        'use_rviz',
        default_value='true',
        description='是否启动RViz可视化'
    )
    
    rviz_config_arg = DeclareLaunchArgument(
        'rviz_config',
        default_value=PathJoinSubstitution([
            FindPackageShare('amr_rctk'),
            'config',
            'patrol',
            'patrol.rviz'
        ]),
        description='RViz配置文件路径'
    )
    
    # 巡逻节点
    patrol_node = Node(
        package='amr_rctk',
        executable='patrol_node',
        name='patrol_node',
        output='screen',
        parameters=[LaunchConfiguration('config_file')],
        remappings=[
            # 如果需要重映射话题，可以在这里添加
            ('/odom', '/odometry/filtered'),
            # ('/cmd_vel', '/robot/cmd_vel'),
        ]
    )
    
    # RViz节点
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', LaunchConfiguration('rviz_config')],
        output='screen',
        condition=IfCondition(LaunchConfiguration('use_rviz'))
    )
    
    return LaunchDescription([
        config_file_arg,
        use_rviz_arg,
        rviz_config_arg,
        patrol_node,
        rviz_node
    ])

