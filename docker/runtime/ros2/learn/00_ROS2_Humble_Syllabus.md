# ROS 2 Humble Practical Curriculum (Ubuntu 20.04 Docker)

Welcome to your structured ROS 2 Humble learning journey. This curriculum is designed to take you from a complete beginner to a developer capable of deploying algorithms onto simulated or physical robots using Docker.

---

## 🛠️ Module 1: ROS 2 Core Concepts & CLI Tools

### Objectives
* Master the foundational architecture of ROS 2 (Nodes, Topics, Messages).
* Learn to navigate, debug, and manipulate running robot systems via the Command Line Interface (CLI).

### Section 1: Understanding Nodes & Topics (Turtlesim Exploration)
1. **Node Concept**: Single-purpose executable blocks (e.g., sensor driver, path planner).
2. **Topic Concept**: Unidirectional data pipelines utilizing a Publisher-Subscriber pattern.
3. **Hands-on Verification**:
   * Open 3 terminals attached to your Docker container.
   * **Terminal 1 (Simulation)**: `ros2 run turtlesim turtlesim_node`
   * **Terminal 2 (Teleoperation)**: `ros2 run turtlesim turtle_teleop_key`
   * **Terminal 3 (Inspection)**:
     ```bash
     ros2 node list
     ros2 topic list
     ros2 topic info /turtle1/cmd_vel
     ros2 topic echo /turtle1/cmd_vel
     ros2 topic pub --once /turtle1/cmd_vel geometry_msgs/msg/Twist "{linear: {x: 2.0, y: 0.0, z: 0.0}, angular: {x: 0.0, y: 0.0, z: 1.8}}"
     ```



### Section 2: Active Telemetry & Graphical Analysis
1. **Inspecting Message Types**: Discovering the underlying data structures.
   ```bash
   ros2 topic type /turtle1/cmd_vel
   ros2 interface show geometry_msgs/msg/Twist
   ```
2. **Dynamic Computation Graph**: Visualizing how data flows between active nodes.
   ```bash
   ros2 run rqt_graph rqt_graph
   ```
   *Action*: Observe the graphic mapping `/teleop_turtle` publishing `geometry_msgs/msg/Twist` directly to `/turtlesim`.

---

## 📁 Module 2: Workspaces, Packages, and Python Nodes

### Objectives
* Set up a persistent development environment.
* Write, compile, and execute a custom Python node that programmatically drives a robot.

### Section 1: Creating a ROS 2 Workspace
1. Move to the shared directory configured in your `docker-compose.yml` file to ensure code persistence:
   ```bash
   cd /home/ros/workspace
   mkdir -p ros2_ws/src
   cd ros2_ws
   ```
2. Build the empty workspace to verify the compiler (`colcon`):
   ```bash
   colcon build
   ```
3. Source your new workspace environment overlay:
   ```bash
   source install/setup.bash
   ```

### Section 2: Building your First Python Publisher Package
1. **Generate the Package Blueprint**:
   ```bash
   cd src
   ros2 pkg create --build-type ament_python my_robot_controller --dependencies rclpy geometry_msgs
   ```
2. **Write the Autonomous Controller Code**:
   Create a script named `draw_circle.py` inside `ros2_ws/src/my_robot_controller/my_robot_controller/` and paste the following Python implementation:

   ```python
   #!/usr/bin/env python3
   import rclpy
   from rclpy.node import Node
   from geometry_msgs.msg import Twist

   class CircleDrawerNode(Node):
      def __init__(self):
         super().__init__('circle_drawer')
         self.publisher_ = self.create_publisher(Twist, '/turtle1/cmd_vel', 10)
         self.timer = self.create_timer(0.5, self.timer_callback)
         self.get_logger().info('Circle Drawer Node has been started!')

      def timer_callback(self):
         msg = Twist()
         msg.linear.x = 2.0   # Forward velocity
         msg.angular.z = 1.0  # Turning velocity
         self.publisher_.publish(msg)

   class CircleDrawerNodeAdvance(Node):
      def __init__(self):
         super().__init__('circle_drawer')
         self.publisher_ = self.create_publisher(Twist, '/turtle1/cmd_vel', 10)
         self.timer = self.create_timer(0.5, self.timer_callback)
         self.direction = 1.0
         self.cnt_change_dir = 0
         self.get_logger().info('Circle Drawer Node has been started!')

      def timer_callback(self):
         msg = Twist()
         msg.linear.x = 0.8
         msg.angular.z = 1.0 * self.direction
         self.publisher_.publish(msg)
         self.cnt_change_dir += 1
         if self.cnt_change_dir > 20:
               self.direction *= -1.0
               self.cnt_change_dir = 0

   def main(args=None):
      rclpy.init(args=args)
      node = CircleDrawerNodeAdvance()
      rclpy.spin(node)
      rclpy.shutdown()

   if __name__ == '__main__':
      main()
   ```

3. **Configure Registration (`setup.py`)**:
   Add your script entry point within the `entry_points` dictionary inside `setup.py`:
   ```python
   'console_scripts': [
       'draw_circle_node = my_robot_controller.draw_circle:main'
   ],
   ```

4. **Compile and Execute**:
   ```bash
   cd /home/ros/workspace/ros2_ws
   colcon build --packages-select my_robot_controller
   source install/setup.bash
   ros2 run my_robot_controller draw_circle_node
   ```
   *Action*: Watch your turtlesim window as the turtle begins moving in a continuous loop without keyboard inputs.

---

## 🔌 Module 3: Hardware Communication & Custom Services

### Objectives
* Connect physical microcontrollers (e.g., Arduino/ESP32) to Docker ROS 2.
* Learn the Request-Response paradigm via ROS 2 Services.

### Section 1: Serial Interface Passthrough (Micro-ROS/Serial)
1. Ensure your physical serial device is plugged into the host computer (e.g., mapped to `/dev/ttyUSB0` or `/dev/ttyACM0`).
2. Inside your Docker shell, verify the device is exposed correctly:
   ```bash
   ls -l /dev/ttyUSB*
   ```
3. Use a standard ROS 2 executable or generic Python serial node to verify data parsing straight from raw hardware lines.

### Section 2: Implementing Services (Clients and Servers)
1. **Concept**: Unlike continuous Topics, Services operate on a synchronous Request-Response protocol (e.g., triggered state changes, clear maps, spawn objects).
2. **Interacting via CLI**:
   ```bash
   ros2 service list
   ros2 service type /spawn
   ros2 service call /spawn turtlesim/srv/Spawn "{x: 5.0, y: 5.0, theta: 0.0, name: 'turtle2'}"
   ```
   *Action*: Note that a second turtle appears exactly at the coordinates specified by your request payload.

---

## 🤖 Module 4: Advanced 3D Simulation (URDF & Gazebo)

### Objectives
* Create descriptive, multi-joint virtual robot frames.
* Bind physical dynamics (mass, friction, inertia) to view models in real-time within physics environments.

### Section 1: Creating a Robot Description File (URDF)
1. Define a basic Differential Drive base structure using Unified Robot Description Format (URDF) XML schemas.
2. Formulate components such as structural rigid `links` (chassis, wheels) and kinematic constraints through `joints` (continuous or fixed).

### Section 2: GPU-Accelerated Gazebo Ignition Execution
1. Run the hardware-accelerated robot spawn command inside your workspace environment:
   ```bash
   ros2 launch gazebo_ros gazebo.launch.py
   ```
2. Verify GPU resource rendering inside the container:
   * Look closely at frame rate performance.
   * If configured correctly through the `nvidia-container-toolkit`, shadows, ray-traced sensors, and camera data will compute natively through your NVIDIA card without overloading the CPU.

---

## 📈 Module 5: Navigation 2 (Nav2) & SLAM

### Objectives
* Map out an unknown room using sensor data.
* Enable automated obstacle avoidance and waypoint tracking.

### Section 1: Simultaneous Localization and Mapping (SLAM)
1. Launch an active LiDAR node or a virtual simulation clone inside a structured environment.
2. Execute the mapping algorithm node:
   ```bash
   ros2 launch slam_toolbox online_async_launch.py
   ```
3. Open `rviz2`, add a `Map` display plugin, and configure the target topic to `/map`. Move the robot manually to see a clean 2D layout populate your interface.

### Section 2: Setting up Autonomous Navigation Pipelines
1. Boot the Nav2 stack parameters optimized for differential chassis configurations.
2. Utilize the **"Nav2 Goal"** visual marker plugin inside RViz2.
3. Click anywhere on the map grid to generate a real-time goal orientation vector. Watch the global/local costmaps immediately calculate dynamic collision trajectories to steer your robot safely to the target.
