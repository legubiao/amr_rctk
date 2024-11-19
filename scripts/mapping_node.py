#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
import subprocess
import threading
import signal
import os
from std_msgs.msg import String
from std_srvs.srv import Trigger
from amr_rctk.srv import PoseList
from geometry_msgs.msg import PoseStamped
import yaml
import psutil

class MappingNode(Node):
    def __init__(self):
        super().__init__('mapping_node')

        self.foldername = self.declare_parameter("foldername", "/path/to/your/folder").value
        self.navigation_command = self.declare_parameter("navigation_command", "amr_rctk navigation2.launch.py use_sim_time:=True").value
        self.start_mapping_command = self.declare_parameter("start_mapping_command", "ros2 launch amr_rctk cartographer.launch.py").value
        self.save_map_command = self.declare_parameter("save_map_command", "ros2 run nav2_map_server map_saver_cli -f").value

        self.map_state = "idle"
        self.map_id = ''
        self.pose_list = []

        self.map_command_subscription = self.create_subscription(String, 'map_command', self.map_command_callback, 10)
        self.get_map_files_service = self.create_service(Trigger, 'get_map_files', self.handle_get_map_files)
        # self.pose_list_service = self.create_service(PoseList, 'pose_list', self.handle_pose_list)

        self.map_state_pub = self.create_publisher(String, 'map_state', 10)
        self.timer = self.create_timer(1.0, self.publish_map_state)

    def publish_map_state(self):
        msg = String()
        msg.data = self.map_state
        self.map_state_pub.publish(msg)

    def launch_mapping(self):
        process = subprocess.Popen(self.start_mapping_command.split())
        self.mapping_process = process
        self.map_state = "mapping"
        self.publish_map_state()

    def launch_navigation(self, map_name):
        if self.map_state == "mapping":
            self.get_logger().warn("Terminate SLAM process before launching Navigation")
            self.wait_for_terminate(self.mapping_process, False)
        elif self.map_state == "navigation":
            self.get_logger().warn("Terminate old Navigation process before launching Navigation")
            self.wait_for_terminate(self.navigation_process, False)

        command = "ros2 launch " + self.navigation_command + " map:=" + self.foldername +'/'+ map_name + ".yaml"
        self.get_logger().info("Launch Navigation process with command: " + command)
        
        process = subprocess.Popen(command.split())
        self.navigation_process = process
        self.map_state = "navigation"
        self.map_id = map_name
        self.publish_map_state()
    
    def map_command_callback(self,msg):
        command = msg.data.split()

        if command[0] == "start":
            if self.map_state == "mapping":
                self.get_logger().warn("Already in mapping status")
                return
            
            self.get_logger().info("Received command to start mapping")
            if self.map_state == "navigation":
                self.get_logger().warn("Terminate Navigation process before mapping")
                self.wait_for_terminate(self.navigation_process, False)

            # 启动SLAM建图
            t = threading.Thread(target=self.launch_mapping)
            t.start()
            
        elif command[0] == "stop":
            if self.map_state == "mapping":
                self.get_logger().info("Received command to stop mapping")
                self.wait_for_terminate(self.mapping_process)
                self.get_logger().warn("Mapping process terminated cleanly")
            elif self.map_state == "navigation":
                self.get_logger().warn("Received command to stop navigation")
                self.wait_for_terminate(self.navigation_process)
                self.get_logger().warn("Navigation process terminated cleanly")
            else:
                self.get_logger().warn("Already Idle")
        elif command[0] == "save":
            if self.map_state == "mapping":
                mapPath = self.foldername+'/'+command[1]
                saveMapCommand = self.save_map_command.split()
                saveMapCommand.append(mapPath)

                # 保存地图
                subprocess.call(saveMapCommand)
                self.get_logger().info("Map saved")

                # 启动AMCL节点
                t = threading.Thread(target=self.launch_navigation(command[1]))
                t.start()
            else:
                self.get_logger().warn("Not mapping yet")
        elif command[0] == "load":
            # 启动AMCL节点
            self.get_logger().info("Load map file: " + command[1])
            t = threading.Thread(target=self.launch_navigation(command[1]))
            t.start()
    
    def wait_for_terminate(self, process, publish=True):
        parent = psutil.Process(process.pid)
        for child in parent.children(recursive=True):  # or parent.children() for recursive=False
            child.kill()
        process.terminate()
        self.map_state = "terminating"
        self.publish_map_state()
        process.wait()
        if (publish):
            self.map_state = "idle"
            self.publish_map_state()

    def handle_get_map_files(self, request, response):
        try:
            map_files = [f[:-4] for f in os.listdir(self.foldername) if f.endswith('.pgm')]
            response.success = True
            response.message = ','.join(map_files)
        except OSError as e:
            print(f"Error: {e}")
            response.success = False
            response.message = str(e)
        return response

    def on_shutdown(self):
        if self.map_state == "mapping":
            os.kill(self.mapping_process.pid, signal.SIGINT)
        elif self.map_state == "navigation":
            os.kill(self.navigation_process.pid, signal.SIGINT)
    
def main(args=None):
    rclpy.init(args=args)

    node = MappingNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        if node.map_state == "mapping":
            node.get_logger().info('Interrupted by user, shutting down mapping process')
        elif node.map_state == "navigation":
            node.get_logger().info('Interrupted by user, shutting down navigation process')
        node.on_shutdown()
    finally:
        if rclpy.ok():  # 检查ROS2的上下文是否已经关闭
            node.destroy_node()
            rclpy.shutdown()

if __name__ == "__main__":
    main()
