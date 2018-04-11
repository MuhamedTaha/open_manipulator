/*******************************************************************************
* Copyright 2016 ROBOTIS CO., LTD.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/* Authors: Taehun Lim (Darby) */

#include "open_manipulator_position_ctrl/gripper_controller.h"

using namespace open_manipulator;

GripperController::GripperController()
    :using_gazebo_(false),
     robot_name_(""),
     palm_num_(2),
     is_moving_(false)
{
  // Init parameter
  nh_.getParam("gazebo", using_gazebo_);
  nh_.getParam("robot_name", robot_name_);
  nh_.getParam("gripper_dxl_id", gripper_dxl_id_);

  gripper_.name = "gripper";
  gripper_.dxl_id = gripper_dxl_id_;

  planned_path_info_.waypoints = 10;
  planned_path_info_.planned_path_positions = Eigen::MatrixXd::Zero(planned_path_info_.waypoints, 2);

  move_group = new moveit::planning_interface::MoveGroupInterface("gripper");

  initPublisher(using_gazebo_);
  initSubscriber(using_gazebo_);
}

GripperController::~GripperController()
{
  ros::shutdown();
  return;
}

void GripperController::initPublisher(bool using_gazebo)
{
  if (using_gazebo)
  {
    ROS_INFO("SET Gazebo Simulation Mode(Gripper)");

    // if (robot_name_.find("tb3") != std::string::npos)
    // {
    //   // open_manipulator chain with tb3
    //   gazebo_gripper_position_pub_[LEFT_PALM]  = nh_.advertise<std_msgs::Float64>("/grip_joint_position/command", 10);
    //   gazebo_gripper_position_pub_[RIGHT_PALM] = nh_.advertise<std_msgs::Float64>("/grip_joint_sub_position/command", 10);
    // }
    // else
    // {
      gazebo_gripper_position_pub_[LEFT_PALM]  = nh_.advertise<std_msgs::Float64>(robot_name_ + "/grip_joint_position/command", 10);
      gazebo_gripper_position_pub_[RIGHT_PALM] = nh_.advertise<std_msgs::Float64>(robot_name_ + "/grip_joint_sub_position/command", 10);
    // }
  }

  gripper_onoff_pub_ = nh_.advertise<open_manipulator_msgs::JointPose>(robot_name_ + "/gripper_pose", 10);
}

void GripperController::initSubscriber(bool using_gazebo)
{
  if (using_gazebo)
  {
    gazebo_present_joint_position_sub_ = nh_.subscribe(robot_name_ + "/joint_states", 10,
                                                       &GripperController::gazeboPresentJointPositionMsgCallback, this);
  }

  gripper_pose_sub_ = nh_.subscribe(robot_name_ + "/gripper_pose", 10,
                                    &GripperController::targetGripperPoseMsgCallback, this);

  if (robot_name_.find("tb3") != std::string::npos)
  {
    gripper_onoff_sub_ = nh_.subscribe(robot_name_ + "/gripper", 10,
                                       &GripperController::gripperPositionMsgCallback, this);
  }
  else
  {
    gripper_onoff_sub_ = nh_.subscribe(robot_name_ + "/gripper", 10,
                                       &GripperController::gripperPositionMsgCallback, this);
  }

  display_planned_path_sub_ = nh_.subscribe("/move_group/display_planned_path", 10,
                                            &GripperController::displayPlannedPathMsgCallback, this);
}

void GripperController::gazeboPresentJointPositionMsgCallback(const sensor_msgs::JointState::ConstPtr &msg)
{
  //TODO
}

void GripperController::targetGripperPoseMsgCallback(const open_manipulator_msgs::JointPose::ConstPtr &msg)
{
  ros::AsyncSpinner spinner(1);
  spinner.start();

  const robot_state::JointModelGroup *joint_model_group = move_group->getCurrentState()->getJointModelGroup("gripper");

  moveit::core::RobotStatePtr current_state = move_group->getCurrentState();

  std::vector<double> joint_group_positions;
  current_state->copyJointGroupPositions(joint_model_group, joint_group_positions);

  for (uint8_t index = 0; index < palm_num_; index++)
  {
    joint_group_positions[index] = msg->position[0];
    joint_group_positions[index] = msg->position[0];
  }

  move_group->setJointValueTarget(joint_group_positions);

  move_group->setMaxVelocityScalingFactor(0.3);
  move_group->setMaxAccelerationScalingFactor(0.01);

  moveit::planning_interface::MoveGroupInterface::Plan my_plan;

  if (is_moving_ == false)
  {
    bool success = (move_group->plan(my_plan) == moveit::planning_interface::MoveItErrorCode::SUCCESS);

    if (success) move_group->move();
    else ROS_WARN("Planning (joint space goal) is FAILED");
  }
  else
    ROS_WARN("ROBOT IS WORKING");

  spinner.stop();
}

void GripperController::gripperPositionMsgCallback(const std_msgs::String::ConstPtr &msg)
{
  open_manipulator_msgs::JointPose grip_pose;

  if (msg->data == "grip_on")
  {
    grip_pose.position.push_back(GRIP_ON); 

    gripper_onoff_pub_.publish(grip_pose);
  }
  else if (msg->data == "grip_off")
  {
    grip_pose.position.push_back(GRIP_OFF); 

    gripper_onoff_pub_.publish(grip_pose);
  }
    else if (msg->data == "neutral")
  {
    grip_pose.position.push_back(NEUTRAL); 

    gripper_onoff_pub_.publish(grip_pose);
  }
  else
  {
    ROS_ERROR("If you want to grip or release something, publish 'grip_on' or 'grip_off'");
  }
}

void GripperController::displayPlannedPathMsgCallback(const moveit_msgs::DisplayTrajectory::ConstPtr &msg)
{
  // Can find 'grip'
  if (msg->trajectory[0].joint_trajectory.joint_names[0].find("grip") != std::string::npos)
  {
    ROS_INFO("Get Gripper Planned Path");
    uint8_t gripper_num = 2;

    planned_path_info_.waypoints = msg->trajectory[0].joint_trajectory.points.size();

    planned_path_info_.planned_path_positions.resize(planned_path_info_.waypoints, gripper_num);

    for (uint16_t point_num = 0; point_num < planned_path_info_.waypoints; point_num++)
    {
      for (uint8_t num = 0; num < gripper_num; num++)
      {
        float joint_position = msg->trajectory[0].joint_trajectory.points[point_num].positions[num];

        planned_path_info_.planned_path_positions.coeffRef(point_num , num) = joint_position;
      }
    }

    all_time_steps_ = planned_path_info_.waypoints - 1;

    ros::WallDuration sleep_time(0.5);
    sleep_time.sleep();

    is_moving_  = true; 
  }
}

void GripperController::process(void)
{
  static uint16_t step_cnt = 0;
  std_msgs::Float64 goal_joint_position;

  if (is_moving_)
  {
    if (using_gazebo_)
    {
      goal_joint_position.data = planned_path_info_.planned_path_positions(step_cnt, 0);
      gazebo_gripper_position_pub_[LEFT_PALM].publish(goal_joint_position);

      goal_joint_position.data = planned_path_info_.planned_path_positions(step_cnt, 1);
      gazebo_gripper_position_pub_[RIGHT_PALM].publish(goal_joint_position);
    }

    if (step_cnt >= all_time_steps_)
    {
      is_moving_ = false;
      step_cnt   = 0;

      ROS_INFO("Complete Execution");
    }
    else
    {
      step_cnt++;
    }
  }
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "gripper_controller_for_OpenManipulator");

  ros::WallDuration sleep_time(3.0);
  sleep_time.sleep();

  GripperController controller;

  ros::Rate loop_rate(ITERATION_FREQUENCY);

  while (ros::ok())
  {
    controller.process();

    ros::spinOnce();
    loop_rate.sleep();
  }

  return 0;
}