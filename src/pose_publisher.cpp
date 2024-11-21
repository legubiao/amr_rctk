#include <rclcpp/rclcpp.hpp>
#include <tf2_ros/transform_listener.h>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <tf2/exceptions.h>
#include <tf2_ros/buffer.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <deque>
#include <nav_msgs/msg/path.hpp>
#include <amr_rctk/srv/frame_to_pose.hpp>

bool handle_frame_to_pose_request(
    const std::shared_ptr<amr_rctk::srv::FrameToPose::Request>& request,
    const std::shared_ptr<amr_rctk::srv::FrameToPose::Response>& response,
    const tf2_ros::Buffer& tfBuffer,
    const rclcpp::Node::SharedPtr& node)
{
    geometry_msgs::msg::TransformStamped transformStamped;
    try{
        transformStamped = tfBuffer.lookupTransform(request->frame1, request->frame2, rclcpp::Time(0));
    }
    catch (tf2::TransformException &ex) {
        RCLCPP_WARN(node->get_logger(), "%s", ex.what());
        return false;
    }

    response->pose.header.stamp = node->now();
    response->pose.header.frame_id = request->frame1;
    response->pose.pose.position.x = transformStamped.transform.translation.x;
    response->pose.pose.position.y = transformStamped.transform.translation.y;
    response->pose.pose.position.z = transformStamped.transform.translation.z;
    response->pose.pose.orientation = transformStamped.transform.rotation;

    return true;
}

int main(int argc, char** argv){
    rclcpp::init(argc, argv);

    auto node = std::make_shared<rclcpp::Node>("robot_pose_publisher");

    auto pose_pub = node->create_publisher<geometry_msgs::msg::PoseStamped>("/robot_pose", 10);
    auto path_pub = node->create_publisher<nav_msgs::msg::Path>("/robot_path", 10);

    tf2_ros::Buffer tfBuffer(node->get_clock());
    tf2_ros::TransformListener tfListener(tfBuffer);

    auto service = node->create_service<amr_rctk::srv::FrameToPose>("frame_to_pose",
        std::bind(&handle_frame_to_pose_request, std::placeholders::_1, std::placeholders::_2, std::ref(tfBuffer), node));


    bool print_error = node->declare_parameter<bool>("print_error", false);
    auto source_frame = node->declare_parameter<std::string>("source_frame", "base_link");

    std::deque<geometry_msgs::msg::PoseStamped> poses;
    nav_msgs::msg::Path path_msg;
    path_msg.header.frame_id = "map";
    int path_counter = 5;

    rclcpp::Rate rate(10.0);
    while (rclcpp::ok()){
        try{
            geometry_msgs::msg::TransformStamped transformStamped;
            transformStamped = tfBuffer.lookupTransform("map", source_frame, tf2::TimePointZero);
            geometry_msgs::msg::PoseStamped pose_msg;
            pose_msg.header.stamp = node->now();
            pose_msg.header.frame_id = "map";
            pose_msg.pose.position.x = transformStamped.transform.translation.x;
            pose_msg.pose.position.y = transformStamped.transform.translation.y;
            pose_msg.pose.position.z = transformStamped.transform.translation.z;
            pose_msg.pose.orientation = transformStamped.transform.rotation;

            pose_pub->publish(pose_msg);

            path_counter -= 1;
            if (path_counter == 0){
                path_counter = 5;
                poses.push_back(pose_msg);
                if (poses.size() > 100){
                    poses.pop_front();
                }
                path_msg.poses = std::vector(poses.begin(), poses.end());
                path_msg.header.stamp = node->now();
                path_msg.header.frame_id = "map";
                path_pub->publish(path_msg);
            }
        }
        catch (tf2::TransformException &ex) {
            if (print_error){
                RCLCPP_ERROR(node->get_logger(), "%s", ex.what());
            }
        }

        spin_some(node);
        rate.sleep();
    }
    rclcpp::shutdown();
    return 0;
};