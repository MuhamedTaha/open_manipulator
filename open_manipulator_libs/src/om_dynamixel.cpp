﻿/*******************************************************************************
* Copyright 2018 ROBOTIS CO., LTD.
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

#include "open_manipulator_libs/om_dynamixel.h"

using namespace OM_DYNAMIXEL;

void JointDynamixel::init(std::vector<uint8_t> actuator_id, const void *arg)
{
  std::string *get_arg_ = (std::string *)arg;

  bool result = JointDynamixel::initialize(actuator_id ,get_arg_[0], get_arg_[1]);

  if (result == false)
    return;
}

void JointDynamixel::setMode(std::vector<uint8_t> actuator_id, const void *arg)
{
  bool result = false;
  const char* log = NULL;

  std::string *get_arg_ = (std::string *)arg;

  if (get_arg_[0] == "position_mode" || get_arg_[0] == "current_based_position_mode")
  {
    result = JointDynamixel::setOperatingMode(actuator_id, get_arg_[0]);
    if (result == false)
      return;
  }
  else
  {
    result = JointDynamixel::writeProfileValue(actuator_id, get_arg_[0], std::stoi(get_arg_[1]));
    if (result == false)
      return;
  }

  result = JointDynamixel::setSDKHandler(actuator_id.at(0));
  if (result == false)
    return;
}

std::vector<uint8_t> JointDynamixel::getId()
{
  return dynamixel_.id;
}

void JointDynamixel::enable()
{
  const char* log = NULL;
  bool result = false;
  
  for (uint32_t index = 0; index < dynamixel_.num; index++)
  {
    result = dynamixel_workbench_->torqueOn(dynamixel_.id.at(index), &log);
    if (result == false)
    {
      printf("%s\n", log);
      return;
    }
  }
}

void JointDynamixel::disable()
{
  const char* log = NULL;
  bool result = false;
  
  for (uint32_t index = 0; index < dynamixel_.num; index++)
  {
    result = dynamixel_workbench_->torqueOff(dynamixel_.id.at(index), &log);
    if (result == false)
    {
      printf("%s\n", log);
      return;
    }
  }
}



bool JointDynamixel::sendJointActuatorValue(std::vector<uint8_t> actuator_id, std::vector<ROBOTIS_MANIPULATOR::Actuator> value_vector)
{
  bool result = false;
  std::vector<double> radian_vector;

  for(int index = 0; index < value_vector.size(); index++)
  {
    radian_vector.push_back(value_vector.at(index).value);
  }

  result = JointDynamixel::writeGoalPosition(actuator_id, radian_vector);
  if (result == false)
    return false;

  return true;
}

std::vector<ROBOTIS_MANIPULATOR::Actuator> JointDynamixel::receiveJointActuatorValue(std::vector<uint8_t> actuator_id)
{
  return JointDynamixel::receiveAllDynamixelValue(actuator_id);
}

//////////////////////////////////////////////////////////////////////////

bool JointDynamixel::initialize(std::vector<uint8_t> actuator_id, std::string dxl_device_name, std::string dxl_baud_rate)
{
  bool result = false;
  const char* log = NULL;

  dynamixel_.id = actuator_id;
  dynamixel_.num = actuator_id.size();

  dynamixel_workbench_ = new DynamixelWorkbench;

  result = dynamixel_workbench_->init(dxl_device_name.c_str(), std::stoi(dxl_baud_rate), &log);
  if (result == false)
  {
    printf("%s\n", log);
    return false;
  }    

  uint16_t get_model_number;
  for (uint8_t index = 0; index < dynamixel_.num; index++)
  {
    uint8_t id = dynamixel_.id.at(index);
    result = dynamixel_workbench_->ping(id, &get_model_number, &log);
    if (result == false)
    {
      printf("%s\n", log);
      printf("Please check your Dynamixel ID\n");
      return false;
    }
    else
    {
      printf("ID : %d, Model Name : %s\n", id, dynamixel_workbench_->getModelName(id));
    }
  }

  return true;
}

bool JointDynamixel::setOperatingMode(std::vector<uint8_t> actuator_id, std::string dynamixel_mode)
{
  const char* log = NULL;
  bool result = false;

  const uint32_t velocity = 0;
  const uint32_t acceleration = 0;
  const uint32_t current = 0;

  if (dynamixel_mode == "position_mode")
  {
    for (uint8_t num = 0; num < actuator_id.size(); num++)
    {
      result = dynamixel_workbench_->jointMode(actuator_id.at(num), velocity, acceleration, &log);
      if (result == false)
      {
        printf("%s\n", log);
        return false;
      }
    }
  }
  else if (dynamixel_mode == "current_based_position_mode")
  {
    for (uint8_t num = 0; num < actuator_id.size(); num++)
    {
      result = dynamixel_workbench_->CurrentBasedPositionMode(actuator_id.at(num), current, &log);
      if (result == false)
      {
        printf("%s\n", log);
        return false;
      }
    }
  }
  else
  {
    for (uint8_t num = 0; num < actuator_id.size(); num++)
    {
      result = dynamixel_workbench_->jointMode(actuator_id.at(num), velocity, acceleration, &log);
      if (result == false)
      {
        printf("%s\n", log);
        return false;
      }
    }
  }

  return true;
}

bool JointDynamixel::setSDKHandler(uint8_t actuator_id)
{
  bool result = false;
  const char* log = NULL;

  dynamixel_workbench_->addSyncWriteHandler(actuator_id, "Goal_Position", &log);

  result = dynamixel_workbench_->addSyncReadHandler(ADDR_PRESENT_CURRENT_2, 
                                                    (LENGTH_PRESENT_CURRENT_2 + LENGTH_PRESENT_VELOCITY_2 + LENGTH_PRESENT_POSITION_2), 
                                                    &log);
  if (result == false)
  {
    printf("%s\n", log);
    return false;
  }

  return true;
}

bool JointDynamixel::writeProfileValue(std::vector<uint8_t> actuator_id, std::string profile_mode, uint32_t value)
{
  const char* log = NULL;
  bool result = false;

  const char * char_profile_mode = profile_mode.c_str();

  for (uint8_t num = 0; num < actuator_id.size(); num++)
  {
    result = dynamixel_workbench_->writeRegister(actuator_id.at(num), char_profile_mode, value, &log);
    if (result == false)
    {
      printf("%s\n", log);
      return false;
    }
  }

  return true;
}

bool JointDynamixel::writeGoalPosition(std::vector<uint8_t> actuator_id, std::vector<double> radian_vector)
{
  bool result = false;
  const char* log = NULL;

  uint8_t id_array[actuator_id.size()];
  int32_t goal_position[actuator_id.size()];

  for (uint8_t index = 0; index < actuator_id.size(); index++)
  {
    id_array[index] = actuator_id.at(index);
    goal_position[index] = dynamixel_workbench_->convertRadian2Value(actuator_id.at(index), radian_vector.at(index));
  }

  result = dynamixel_workbench_->syncWrite(SYNC_WRITE_HANDLER_FOR_GOAL_POSITION, id_array, actuator_id.size(), goal_position, &log);
  if (result == false)
  {
    printf("%s\n", log);
    return false;
  }

  return true;
}

std::vector<ROBOTIS_MANIPULATOR::Actuator> JointDynamixel::receiveAllDynamixelValue(std::vector<uint8_t> actuator_id)
{
  bool result = false;
  const char* log = NULL;

  ROBOTIS_MANIPULATOR::Actuator value;
  std::vector<ROBOTIS_MANIPULATOR::Actuator> all_values;

  int32_t get_value[actuator_id.size()];

  uint8_t id_array[actuator_id.size()];
  for (uint8_t index = 0; index < actuator_id.size(); index++)
  {
    id_array[index] = actuator_id.at(index);
  }

  result = dynamixel_workbench_->syncRead(SYNC_READ_HANDLER_FOR_PRESENT_POSITION_VELOCITY_CURRENT, 
                                          id_array, 
                                          actuator_id.size(), 
                                          get_value, 
                                          &log);
  if (result == false)
  {
    printf("%s\n", log);
    return all_values; // Is it right..?
  }

  for (uint8_t index = 0; index < actuator_id.size(); index++)
  {
    value.value = dynamixel_workbench_->convertValue2Radian(actuator_id.at(index), get_value[index]);
    all_values.push_back(value);
  }

  return all_values;
}

//////////////////////////////////////tool actuator

void GripperDynamixel::init(uint8_t actuator_id, const void *arg)
{
  std::string *get_arg_ = (std::string *)arg;

  bool result = GripperDynamixel::initialize(actuator_id ,get_arg_[0], get_arg_[1]);

  if (result == false)
    return;
}

void GripperDynamixel::setMode(const void *arg)
{
  bool result = false;
  const char* log = NULL;

  std::string *get_arg_ = (std::string *)arg;

  if (get_arg_[0] == "position_mode" || get_arg_[0] == "current_based_position_mode")
  {
    result = GripperDynamixel::setOperatingMode(get_arg_[0]);
    if (result == false)
      return;
  }
  else
  {
    result = GripperDynamixel::writeProfileValue(get_arg_[0], std::stoi(get_arg_[1]));
    if (result == false)
      return;
  }

  result = GripperDynamixel::setSDKHandler();
  if (result == false)
    return;
}

uint8_t GripperDynamixel::getId()
{
  return dynamixel_.id.at(0);
}

void GripperDynamixel::enable()
{
  const char* log = NULL;
  bool result = false;
  
  result = dynamixel_workbench_->torqueOn(dynamixel_.id.at(0), &log);
  if (result == false)
  {
    printf("%s\n", log);
    return;
  }
}

void GripperDynamixel::disable()
{
  const char* log = NULL;
  bool result = false;
  
  result = dynamixel_workbench_->torqueOff(dynamixel_.id.at(0), &log);
  if (result == false)
  {
    printf("%s\n", log);
    return;
  }
}

bool GripperDynamixel::sendToolActuatorValue(double value)
{
  return GripperDynamixel::writeGoalPosition(value);
}

double GripperDynamixel::receiveToolActuatorValue()
{
  return GripperDynamixel::receiveDynamixelValue();
}

//////////////////////////////////////////////////////////////////////////

bool GripperDynamixel::initialize(uint8_t actuator_id, std::string dxl_device_name, std::string dxl_baud_rate)
{
  const char* log = NULL;
  bool result = false;

  dynamixel_.id.push_back(actuator_id);
  dynamixel_.num = 1;

  dynamixel_workbench_ = new DynamixelWorkbench;

  result = dynamixel_workbench_->init(dxl_device_name.c_str(), std::stoi(dxl_baud_rate), &log);
  if (result == false)
  {
    printf("%s\n", log);
    return false;
  }

  uint16_t get_model_number;
  result = dynamixel_workbench_->ping(dynamixel_.id.at(0), &get_model_number, &log);
  if (result == false)
  {
    printf("%s\n", log);
    return false;
  }

  return true;
}

bool GripperDynamixel::setOperatingMode(std::string dynamixel_mode)
{
  const char* log = NULL;
  bool result = false;

  const uint32_t velocity = 0;
  const uint32_t acceleration = 0;
  const uint32_t current = 0;

  if (dynamixel_mode == "position_mode")
  {
    result = dynamixel_workbench_->jointMode(dynamixel_.id.at(0), velocity, acceleration, &log);
    if (result == false)
    {
      printf("%s\n", log);
      return false;
    }
  }
  else if (dynamixel_mode == "current_based_position_mode")
  {
    result = dynamixel_workbench_->CurrentBasedPositionMode(dynamixel_.id.at(0), current, &log);
    if (result == false)
    {
      printf("%s\n", log);
      return false;
    }
  }
  else
  {
    result = dynamixel_workbench_->jointMode(dynamixel_.id.at(0), velocity, acceleration, &log);
    if (result == false)
    {
      printf("%s\n", log);
      return false;
    }
  }

  return true;
}

bool GripperDynamixel::writeProfileValue(std::string profile_mode, uint8_t value)
{
  const char* log = NULL;
  bool result = false;

  const char * char_profile_mode = profile_mode.c_str();

  result = dynamixel_workbench_->writeRegister(dynamixel_.id.at(0), char_profile_mode, value, &log);
  if (result == false)
  {
    printf("%s\n", log);
    return false;
  }

  return true;
}

bool GripperDynamixel::setSDKHandler()
{
  bool result = false;
  const char* log = NULL;

  dynamixel_workbench_->addSyncWriteHandler(dynamixel_.id.at(0), "Goal_Position", &log);

  result = dynamixel_workbench_->addSyncReadHandler(ADDR_PRESENT_CURRENT_2, 
                                                    (LENGTH_PRESENT_CURRENT_2 + LENGTH_PRESENT_VELOCITY_2 + LENGTH_PRESENT_POSITION_2), 
                                                    &log);
  if (result == false)
  {
    printf("%s\n", log);
    return false;
  }

  return true;
}

bool GripperDynamixel::writeGoalPosition(double radian)
{
  bool result = false;
  const char* log = NULL;

  int32_t goal_position = 0;

  goal_position = dynamixel_workbench_->convertRadian2Value(dynamixel_.id.at(0), radian);

  result = dynamixel_workbench_->syncWrite(SYNC_WRITE_HANDLER_FOR_GOAL_POSITION, &goal_position, &log);
  if (result == false)
  {
    printf("%s\n", log);
    return false;
  }

  return true;
}

double GripperDynamixel::receiveDynamixelValue()
{
  bool result = false;
  const char* log = NULL;

  int32_t get_value = 0;
  double value = 0.0;

  result = dynamixel_workbench_->syncRead(SYNC_READ_HANDLER_FOR_PRESENT_POSITION_VELOCITY_CURRENT, 
                                          &get_value, 
                                          &log);
  if (result == false)
  {
    printf("%s\n", log);
    return value; // Is it right..?
  }

  return dynamixel_workbench_->convertValue2Radian(dynamixel_.id.at(0), get_value);
}
