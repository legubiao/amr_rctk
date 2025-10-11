#include <rclcpp/rclcpp.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/path.hpp>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Matrix3x3.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <cmath>
#include <vector>

class PatrolNode : public rclcpp::Node
{
public:
    PatrolNode() : Node("patrol_node"), current_waypoint_index_(0), patrol_active_(false), 
                   at_position_(false)
    {
        // 声明参数
        this->declare_parameter<std::vector<double>>("waypoints_x", std::vector<double>());
        this->declare_parameter<std::vector<double>>("waypoints_y", std::vector<double>());
        this->declare_parameter<std::vector<double>>("waypoints_yaw", std::vector<double>());
        this->declare_parameter<bool>("repeat_patrol", true);
        this->declare_parameter<double>("goal_tolerance", 0.2);
        this->declare_parameter<double>("angular_tolerance", 0.1);
        this->declare_parameter<double>("linear_velocity", 0.3);
        this->declare_parameter<double>("angular_velocity", 0.5);
        this->declare_parameter<double>("kp_linear", 1.0);
        this->declare_parameter<double>("kp_angular", 2.0);
        this->declare_parameter<double>("max_linear_vel", 0.5);
        this->declare_parameter<double>("max_angular_vel", 1.0);

        // 获取参数
        std::vector<double> waypoints_x = this->get_parameter("waypoints_x").as_double_array();
        std::vector<double> waypoints_y = this->get_parameter("waypoints_y").as_double_array();
        std::vector<double> waypoints_yaw = this->get_parameter("waypoints_yaw").as_double_array();
        repeat_patrol_ = this->get_parameter("repeat_patrol").as_bool();
        goal_tolerance_ = this->get_parameter("goal_tolerance").as_double();
        angular_tolerance_ = this->get_parameter("angular_tolerance").as_double();
        linear_velocity_ = this->get_parameter("linear_velocity").as_double();
        angular_velocity_ = this->get_parameter("angular_velocity").as_double();
        kp_linear_ = this->get_parameter("kp_linear").as_double();
        kp_angular_ = this->get_parameter("kp_angular").as_double();
        max_linear_vel_ = this->get_parameter("max_linear_vel").as_double();
        max_angular_vel_ = this->get_parameter("max_angular_vel").as_double();

        // 检查巡逻点是否有效
        if (waypoints_x.size() != waypoints_y.size())
        {
            RCLCPP_ERROR(this->get_logger(), "巡逻点X和Y坐标数量不匹配！");
            rclcpp::shutdown();
            return;
        }

        if (waypoints_x.empty())
        {
            RCLCPP_ERROR(this->get_logger(), "没有设置巡逻点！");
            rclcpp::shutdown();
            return;
        }

        // 如果没有提供yaw角度，默认为0
        if (waypoints_yaw.empty())
        {
            waypoints_yaw.resize(waypoints_x.size(), 0.0);
            RCLCPP_INFO(this->get_logger(), "未设置目标角度，默认使用0度");
        }
        else if (waypoints_yaw.size() != waypoints_x.size())
        {
            RCLCPP_ERROR(this->get_logger(), "巡逻点角度数量与坐标数量不匹配！");
            rclcpp::shutdown();
            return;
        }

        // 初始化巡逻点列表
        for (size_t i = 0; i < waypoints_x.size(); ++i)
        {
            geometry_msgs::msg::Pose waypoint;
            waypoint.position.x = waypoints_x[i];
            waypoint.position.y = waypoints_y[i];
            waypoint.position.z = 0.0;
            
            // 将yaw角度转换为四元数
            tf2::Quaternion q;
            q.setRPY(0, 0, waypoints_yaw[i]);
            waypoint.orientation = tf2::toMsg(q);
            
            waypoints_.push_back(waypoint);
        }

        RCLCPP_INFO(this->get_logger(), "加载了 %zu 个巡逻点", waypoints_.size());
        RCLCPP_INFO(this->get_logger(), "重复巡逻: %s", repeat_patrol_ ? "是" : "否");

        // 创建订阅和发布
        odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
            "/odom", 10,
            std::bind(&PatrolNode::odomCallback, this, std::placeholders::_1));

        cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);
        
        // 发布当前目标点和路径用于可视化
        goal_pub_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("/patrol_goal", 10);
        path_pub_ = this->create_publisher<nav_msgs::msg::Path>("/patrol_path", 10);

        // 发布巡逻路径
        publishPatrolPath();

        // 创建控制定时器
        control_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&PatrolNode::controlLoop, this));

        RCLCPP_INFO(this->get_logger(), "巡逻节点已启动");
    }

private:
    void odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
    {
        current_pose_ = msg->pose.pose;
        
        // 如果还没有激活巡逻，激活它
        if (!patrol_active_ && !waypoints_.empty())
        {
            patrol_active_ = true;
            RCLCPP_INFO(this->get_logger(), "开始巡逻，目标点: %zu", current_waypoint_index_);
        }
    }

    void controlLoop()
    {
        if (!patrol_active_ || waypoints_.empty())
        {
            return;
        }

        // 获取当前目标点
        const auto& target = waypoints_[current_waypoint_index_];

        // 从四元数获取当前朝向
        tf2::Quaternion current_q(
            current_pose_.orientation.x,
            current_pose_.orientation.y,
            current_pose_.orientation.z,
            current_pose_.orientation.w);
        tf2::Matrix3x3 current_m(current_q);
        double roll, pitch, current_yaw;
        current_m.getRPY(roll, pitch, current_yaw);

        // 获取目标朝向
        tf2::Quaternion target_q(
            target.orientation.x,
            target.orientation.y,
            target.orientation.z,
            target.orientation.w);
        tf2::Matrix3x3 target_m(target_q);
        double target_roll, target_pitch, target_yaw;
        target_m.getRPY(target_roll, target_pitch, target_yaw);

        // 发布当前目标点用于可视化
        publishCurrentGoal(target);

        // 创建速度命令
        geometry_msgs::msg::Twist cmd_vel;

        if (!at_position_)
        {
            // 阶段1: 到达目标位置
            double dx = target.position.x - current_pose_.position.x;
            double dy = target.position.y - current_pose_.position.y;
            double distance = std::sqrt(dx * dx + dy * dy);
            double move_angle = std::atan2(dy, dx);
            double angle_diff = normalizeAngle(move_angle - current_yaw);

            // 检查是否到达目标位置
            if (distance < goal_tolerance_)
            {
                // 到达位置，停止并准备调整姿态
                cmd_vel.linear.x = 0.0;
                cmd_vel.angular.z = 0.0;
                cmd_vel_pub_->publish(cmd_vel);
                
                at_position_ = true;
                RCLCPP_INFO(this->get_logger(), "到达巡逻点 %zu 位置 (%.2f, %.2f)，开始调整姿态", 
                            current_waypoint_index_, target.position.x, target.position.y);
                return;
            }

            // 如果角度差较大，先转向
            if (std::abs(angle_diff) > angular_tolerance_)
            {
                // 只进行旋转
                cmd_vel.linear.x = 0.0;
                cmd_vel.angular.z = std::clamp(kp_angular_ * angle_diff, 
                                              -max_angular_vel_, max_angular_vel_);
            }
            else
            {
                // 前进并调整角度
                cmd_vel.linear.x = std::clamp(kp_linear_ * distance, 
                                             0.0, max_linear_vel_);
                cmd_vel.angular.z = std::clamp(kp_angular_ * angle_diff, 
                                              -max_angular_vel_, max_angular_vel_);
            }
        }
        else
        {
            // 阶段2: 调整到目标姿态
            double yaw_diff = normalizeAngle(target_yaw - current_yaw);

            // 检查是否到达目标姿态
            if (std::abs(yaw_diff) < angular_tolerance_)
            {
                // 完成此巡逻点，停止
                cmd_vel.linear.x = 0.0;
                cmd_vel.angular.z = 0.0;
                cmd_vel_pub_->publish(cmd_vel);

                double target_yaw_deg = target_yaw * 180.0 / M_PI;
                RCLCPP_INFO(this->get_logger(), "完成巡逻点 %zu 姿态调整 (yaw: %.1f°)", 
                            current_waypoint_index_, target_yaw_deg);

                // 重置状态标志，移动到下一个目标点
                at_position_ = false;
                current_waypoint_index_++;

                // 检查是否完成所有巡逻点
                if (current_waypoint_index_ >= waypoints_.size())
                {
                    if (repeat_patrol_)
                    {
                        // 重新开始巡逻
                        current_waypoint_index_ = 0;
                        RCLCPP_INFO(this->get_logger(), "重新开始巡逻");
                    }
                    else
                    {
                        // 结束巡逻
                        patrol_active_ = false;
                        RCLCPP_INFO(this->get_logger(), "巡逻完成，节点停止");
                        return;
                    }
                }
                else
                {
                    RCLCPP_INFO(this->get_logger(), "前往下一个巡逻点: %zu", current_waypoint_index_);
                }
                return;
            }

            // 旋转到目标姿态
            cmd_vel.linear.x = 0.0;
            cmd_vel.angular.z = std::clamp(kp_angular_ * yaw_diff, 
                                          -max_angular_vel_, max_angular_vel_);
        }

        cmd_vel_pub_->publish(cmd_vel);
    }

    void publishCurrentGoal(const geometry_msgs::msg::Pose& target)
    {
        geometry_msgs::msg::PoseStamped goal_msg;
        goal_msg.header.stamp = this->now();
        goal_msg.header.frame_id = "odom";
        goal_msg.pose = target;
        goal_pub_->publish(goal_msg);
    }

    void publishPatrolPath()
    {
        nav_msgs::msg::Path path_msg;
        path_msg.header.stamp = this->now();
        path_msg.header.frame_id = "odom";

        for (const auto& waypoint : waypoints_)
        {
            geometry_msgs::msg::PoseStamped pose;
            pose.header = path_msg.header;
            pose.pose = waypoint;
            path_msg.poses.push_back(pose);
        }

        path_pub_->publish(path_msg);
    }

    double normalizeAngle(double angle)
    {
        while (angle > M_PI) angle -= 2.0 * M_PI;
        while (angle < -M_PI) angle += 2.0 * M_PI;
        return angle;
    }

    // 成员变量
    rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr goal_pub_;
    rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr path_pub_;
    rclcpp::TimerBase::SharedPtr control_timer_;

    geometry_msgs::msg::Pose current_pose_;
    std::vector<geometry_msgs::msg::Pose> waypoints_;
    size_t current_waypoint_index_;
    bool patrol_active_;
    bool at_position_;  // 标记是否已到达位置，正在调整姿态
    bool repeat_patrol_;

    // 控制参数
    double goal_tolerance_;
    double angular_tolerance_;
    double linear_velocity_;
    double angular_velocity_;
    double kp_linear_;
    double kp_angular_;
    double max_linear_vel_;
    double max_angular_vel_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<PatrolNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}

