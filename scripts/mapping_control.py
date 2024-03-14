#!/usr/bin/env python

import rospy
import subprocess
from std_msgs.msg import String

def command_callback(msg):
    global mapping_process
    global isMapping
    command = msg.data.split()

    if command[0] == "start_mapping":

        if isMapping:
            rospy.WARN("Already in mapping status")
        else:
            # 启动SLAM建图
            mapping_process = subprocess.Popen(start_mapping_command.split())
            rospy.loginfo("Received command to start mapping")
            isMapping = True

    elif command[0] == "save_map":
        if isMapping:
            mapPath = foldername+'/'+command[1]
            saveMapCommand = save_map_command.split()
            saveMapCommand.append(mapPath)
            print(saveMapCommand)
            # 保存地图
            subprocess.call(saveMapCommand)
            rospy.loginfo("Map saved")
            # 停止SLAM建图节点
            mapping_process.terminate()
            rospy.loginfo("SLAM mapping process terminated")
            isMapping = False
        else:
            rospy.WARN("Not mapping yet")

if __name__ == "__main__":
    rospy.init_node("mapping_node")
    
    foldername = rospy.get_param("~foldername", "/path/to/your/folder")
    start_mapping_command = rospy.get_param("~start_mapping_command", "roslaunch turtlebot3_slam turtlebot3_slam.launch slam_methods:=gmapping")
    save_map_command = rospy.get_param("~save_map_command", "rosrun map_server map_saver -f")
    isMapping = False

    rospy.Subscriber("map_command", String, command_callback)
    rospy.spin()
