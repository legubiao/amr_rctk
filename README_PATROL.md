# 巡逻节点使用说明

## 目录

- [功能描述](#功能描述)
- [主要特性](#主要特性)
- [快速开始](#快速开始)
  - [最快速启动](#最快速启动使用默认配置)
  - [角度快速参考](#角度快速参考)
  - [监控运行状态](#监控运行状态)
- [文件说明](#文件说明)
- [详细编译说明](#详细编译说明)
- [使用方法](#使用方法)
- [配置参数说明](#配置参数说明)
- [话题接口](#话题接口)
- [在RViz中可视化](#在rviz中可视化)
- [工作原理](#工作原理)
- [示例场景](#示例场景)
- [调试技巧](#调试技巧)
- [注意事项](#注意事项)
- [故障排除](#故障排除)
- [未来改进方向](#未来改进方向)

---

## 功能描述

`patrol_node` 是一个自主巡逻节点，可以控制移动机器人按照预设的关键点列表进行巡逻。节点通过订阅里程计(odom)数据获取当前位置，并发布速度命令(cmd_vel)来控制机器人移动。

## 主要特性

- ✅ 订阅 `/odom` 话题获取机器人当前位姿
- ✅ 发布 `/cmd_vel` 话题控制机器人运动
- ✅ 支持多个巡逻点的顺序执行
- ✅ 支持指定每个巡逻点的目标姿态（朝向角度）
- ✅ 两阶段控制：先到达位置，再调整姿态
- ✅ 支持循环巡逻或单次巡逻模式
- ✅ 基于PID控制的平滑运动
- ✅ 可视化巡逻路径和当前目标点
- ✅ 可配置的速度和容差参数

## 快速开始

### 最快速启动（使用默认配置）

```bash
# 1. 编译
cd /home/biao/ros2_ws
colcon build --packages-select amr_rctk
source install/setup.bash

# 2. 启动巡逻节点（自动打开RViz可视化）
ros2 launch amr_rctk patrol.launch.py
```

默认配置：正方形路径巡逻，每个角朝向不同方向，自动启动RViz显示路径。

**不需要RViz？**
```bash
ros2 launch amr_rctk patrol.launch.py use_rviz:=false
```

### 角度快速参考

| 朝向 | 弧度 | 度数 |
|------|------|------|
| 东 (→) | 0.00 | 0° |
| 北 (↑) | 1.57 | 90° |
| 西 (←) | 3.14 | 180° |
| 南 (↓) | -1.57 | -90° |

**提示**: 不设置yaw参数时，所有点默认朝向东(0度)

### 监控运行状态

```bash
# 查看节点日志
ros2 topic echo /rosout | grep patrol

# 监控速度命令
ros2 topic echo /cmd_vel

# 查看当前目标点
ros2 topic echo /patrol_goal

# 检查odom话题
ros2 topic hz /odom
```

---

## 文件说明

- `src/patrol_node.cpp` - 巡逻节点主程序
- `launch/patrol.launch.py` - 启动文件
- `config/patrol/patrol_points.yaml` - 默认配置文件
- `config/patrol/patrol_examples.yaml` - 多种场景示例配置
- `config/patrol/patrol.rviz` - RViz可视化配置

## 详细编译说明

在ROS2工作空间中编译：

```bash
cd /home/biao/ros2_ws
colcon build --packages-select amr_rctk
source install/setup.bash
```

## 使用方法

### 1. 修改配置文件

编辑 `config/patrol/patrol_points.yaml` 文件，设置巡逻点坐标：

```yaml
patrol_node:
  ros__parameters:
    # 巡逻点X坐标列表（单位：米）
    waypoints_x: [0.0, 2.0, 2.0, 0.0]
    
    # 巡逻点Y坐标列表（单位：米）
    waypoints_y: [0.0, 0.0, 2.0, 2.0]
    
    # 巡逻点目标朝向（单位：弧度，可选）
    # 0.0=向东, 1.57=向北, 3.14=向西, -1.57=向南
    waypoints_yaw: [0.0, 1.57, 3.14, -1.57]
    
    # 是否重复巡逻
    repeat_patrol: true
```

### 2. 启动巡逻节点

使用默认配置文件启动（自动打开RViz）：

```bash
ros2 launch amr_rctk patrol.launch.py
```

使用自定义配置文件：

```bash
ros2 launch amr_rctk patrol.launch.py config_file:=/path/to/your/config.yaml
```

不启动RViz：

```bash
ros2 launch amr_rctk patrol.launch.py use_rviz:=false
```

使用自定义RViz配置：

```bash
ros2 launch amr_rctk patrol.launch.py rviz_config:=/path/to/your.rviz
```

### 3. 直接运行节点（使用命令行参数）

```bash
ros2 run amr_rctk patrol_node --ros-args \
  -p waypoints_x:=[0.0,2.0,2.0,0.0] \
  -p waypoints_y:=[0.0,0.0,2.0,2.0] \
  -p repeat_patrol:=true \
  -p goal_tolerance:=0.2 \
  -p max_linear_vel:=0.5
```

## 配置参数说明

### 参数列表

| 参数名称 | 类型 | 默认值 | 说明 |
|---------|------|--------|------|
| `waypoints_x` | double[] | [] | 巡逻点X坐标列表（米） |
| `waypoints_y` | double[] | [] | 巡逻点Y坐标列表（米） |
| `waypoints_yaw` | double[] | [] | 巡逻点目标朝向列表（弧度，可选，默认为0） |
| `repeat_patrol` | bool | true | 是否循环巡逻（true=循环，false=单次） |
| `goal_tolerance` | double | 0.2 | 到达目标点的距离容差（米） |
| `angular_tolerance` | double | 0.1 | 角度容差（弧度） |
| `linear_velocity` | double | 0.3 | 参考线速度（米/秒） |
| `angular_velocity` | double | 0.5 | 参考角速度（弧度/秒） |
| `kp_linear` | double | 1.0 | 线速度比例控制增益 |
| `kp_angular` | double | 2.0 | 角速度比例控制增益 |
| `max_linear_vel` | double | 0.5 | 最大线速度限制（米/秒） |
| `max_angular_vel` | double | 1.0 | 最大角速度限制（弧度/秒） |

### 常用参数调整示例

```yaml
# 速度控制
max_linear_vel: 0.5      # 最大前进速度 (米/秒)
max_angular_vel: 1.0     # 最大旋转速度 (弧度/秒)

# 精度控制
goal_tolerance: 0.2      # 位置到达容差 (米)
angular_tolerance: 0.1   # 角度到达容差 (弧度，约5.7度)

# PID增益（影响运动平滑度）
kp_linear: 1.0           # 线速度比例增益（越大加速越快）
kp_angular: 2.0          # 角速度比例增益（越大转向越快）
```

**调整建议**:
- 机器人运动不稳定？→ 降低`kp_linear`和`kp_angular`
- 机器人太慢？→ 增大`max_linear_vel`和`max_angular_vel`
- 无法到达目标点？→ 增大`goal_tolerance`
- 姿态调整不精确？→ 减小`angular_tolerance`

## 话题接口

### 订阅的话题

- `/odom` (nav_msgs/Odometry) - 机器人里程计数据

### 发布的话题

- `/cmd_vel` (geometry_msgs/Twist) - 速度控制命令
- `/patrol_goal` (geometry_msgs/PoseStamped) - 当前目标点（用于可视化）
- `/patrol_path` (nav_msgs/Path) - 完整巡逻路径（用于可视化）

## 在RViz中可视化

### 自动启动（推荐）

使用launch文件启动时，RViz会自动打开并加载预配置的可视化设置：

```bash
ros2 launch amr_rctk patrol.launch.py
```

RViz配置文件位于：`config/patrol/patrol.rviz`

### 手动配置

如果需要手动添加显示项：

1. **Path** - 订阅 `/patrol_path` 显示完整巡逻路径
2. **PoseStamped** - 订阅 `/patrol_goal` 显示当前目标点
3. **Odometry** - 订阅 `/odom` 显示机器人当前位置

## 工作原理

1. **初始化**: 从参数中加载巡逻点列表（位置+姿态）
2. **等待定位**: 等待第一个odom消息，确认机器人已定位
3. **两阶段控制**: 
   - **阶段1 - 到达位置**:
     - 计算到目标位置的距离和方向角
     - 如果角度差较大，先原地旋转对准目标方向
     - 角度对准后，向目标点前进并实时调整方向
     - 到达位置（距离<容差）后，进入阶段2
   - **阶段2 - 调整姿态**:
     - 计算当前朝向与目标朝向的角度差
     - 原地旋转到目标朝向
     - 到达目标姿态（角度差<容差）后，完成当前巡逻点
4. **点位切换**: 当完成目标姿态调整后，切换到下一个巡逻点
5. **循环/结束**: 
   - 如果 `repeat_patrol=true`，回到第一个点继续巡逻
   - 如果 `repeat_patrol=false`，停止机器人并结束

## 示例场景

查看 `config/patrol/patrol_examples.yaml` 获取更多配置示例（7种不同场景）

### 场景1: 正方形巡逻（带姿态控制）

```yaml
waypoints_x: [0.0, 2.0, 2.0, 0.0]
waypoints_y: [0.0, 0.0, 2.0, 2.0]
waypoints_yaw: [0.0, 1.57, 3.14, -1.57]  # 每个点朝向不同方向
repeat_patrol: true
```

机器人将按照正方形路径巡逻，在每个顶点调整到指定朝向：
- (0,0) 朝向东 (0°)
- (2,0) 朝向北 (90°)
- (2,2) 朝向西 (180°)
- (0,2) 朝向南 (-90°)

### 场景2: 多点检查（单次，无姿态要求）

```yaml
waypoints_x: [1.0, 3.0, 5.0, 3.0, 1.0]
waypoints_y: [0.0, 1.0, 0.0, -1.0, 0.0]
# waypoints_yaw 未设置，默认所有点朝向0度
repeat_patrol: false
```

机器人访问5个检查点后返回起点并停止，不进行特定姿态调整。

### 场景3: 定点监控

```yaml
waypoints_x: [2.0, 5.0, 8.0]
waypoints_y: [1.0, 3.0, 1.5]
waypoints_yaw: [0.785, -0.785, 3.14]  # 指向特定监控方向
repeat_patrol: true
```

机器人在3个监控点循环巡逻，每到一个点都调整到预设的监控角度。

**更多示例**: 
- 简单往返巡逻
- 安保监控巡逻
- 精确慢速巡逻
- 快速大范围巡逻
- 等，详见 `config/patrol/patrol_examples.yaml`

## 调试技巧

1. **查看节点日志**:
   ```bash
   ros2 topic echo /rosout | grep patrol_node
   ```

2. **监控速度命令**:
   ```bash
   ros2 topic echo /cmd_vel
   ```

3. **检查当前目标点**:
   ```bash
   ros2 topic echo /patrol_goal
   ```

## 注意事项

- ⚠️ 确保巡逻点X、Y坐标数量相同
- ⚠️ 如果设置waypoints_yaw，其数量必须与X、Y坐标数量相同（或不设置，默认为0）
- ⚠️ yaw角度单位为弧度：0=东，π/2=北，π=西，-π/2=南
- ⚠️ 巡逻点坐标基于odom坐标系
- ⚠️ 机器人会先到达位置，然后再调整到目标姿态（两阶段控制）
- ⚠️ 调整速度和增益参数以适应不同的机器人
- ⚠️ 本节点不包含障碍物避障功能，需要配合其他避障系统使用
- ⚠️ 在真实机器人上测试前，建议先在仿真环境中验证

## 故障排除

### 快速诊断

| 问题 | 可能原因 | 解决方案 |
|------|---------|---------|
| 机器人不移动 | 未接收到odom数据 | `ros2 topic hz /odom` 检查话题<br>`ros2 topic list \| grep odom` |
| 运动不稳定/抖动 | PID参数过大 | 降低 `kp_linear` 和 `kp_angular`<br>降低最大速度限制 |
| 无法到达目标点 | 容差设置过小 | 增大 `goal_tolerance` 值 |
| 姿态调整不准确 | 角度容差过大 | 减小 `angular_tolerance` 值 |
| 速度太慢 | 速度限制过低 | 增大 `max_linear_vel` 和 `max_angular_vel` |
| 转弯太急/太缓 | 角速度控制不当 | 调整 `kp_angular` 和 `max_angular_vel` |

### 详细调试步骤

**问题1: 机器人不移动**
```bash
# 检查odom话题是否存在
ros2 topic list | grep odom

# 检查odom发布频率
ros2 topic hz /odom

# 检查巡逻节点是否运行
ros2 node list | grep patrol

# 查看节点日志
ros2 topic echo /rosout | grep patrol
```

**问题2: 机器人运动不稳定**
```yaml
# 在配置文件中降低参数
kp_linear: 0.5      # 从1.0降到0.5
kp_angular: 1.0     # 从2.0降到1.0
max_linear_vel: 0.3 # 从0.5降到0.3
```

**问题3: 无法到达目标点**
```yaml
# 增大容差
goal_tolerance: 0.3      # 从0.2增加到0.3
angular_tolerance: 0.15  # 从0.1增加到0.15
```

## 未来改进方向

- [ ] 添加障碍物避障功能
- [ ] 支持从文件加载更复杂的路径
- [ ] 添加服务接口动态修改巡逻点
- [ ] 支持指定每个点的停留时间
- [ ] 添加更复杂的路径跟踪算法（如Pure Pursuit）
- [ ] 支持速度规划和加减速控制
- [x] ~~支持目标点姿态控制~~ ✅ 已完成

## 许可证

Apache-2.0

## 作者

amr_rctk 开发团队

