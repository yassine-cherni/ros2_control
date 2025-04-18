// Copyright 2022 PAL Robotics S.L.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "hardware_interface/component_parser.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "pluginlib/class_loader.hpp"
#include "ros2_control_test_assets/descriptions.hpp"
#include "transmission_interface/simple_transmission.hpp"
#include "transmission_interface/transmission_loader.hpp"

using testing::DoubleNear;
using testing::SizeIs;

// Floating-point value comparison threshold
const double EPS = 1e-5;

class TransmissionPluginLoader
{
public:
  std::shared_ptr<transmission_interface::TransmissionLoader> create(const std::string & type)
  {
    try
    {
      return class_loader_.createUniqueInstance(type);
    }
    catch (std::exception & ex)
    {
      std::cerr << ex.what() << std::endl;
      return std::shared_ptr<transmission_interface::TransmissionLoader>();
    }
  }

private:
  // must keep it alive because instance destroyers need it
  pluginlib::ClassLoader<transmission_interface::TransmissionLoader> class_loader_ = {
    "transmission_interface", "transmission_interface::TransmissionLoader"};
};

TEST(SimpleTransmissionLoaderTest, FullSpec)
{
  // Parse transmission info

  std::string urdf_to_test = std::string(ros2_control_test_assets::urdf_head) +
                             R"(
    <ros2_control name="RRBotModularJoint1" type="actuator">
      <hardware>
        <plugin>ros2_control_demo_hardware/VelocityActuatorHardware</plugin>
        <param name="example_param_write_for_sec">1.23</param>
        <param name="example_param_read_for_sec">3</param>
      </hardware>
      <joint name="joint1">
        <command_interface name="velocity">
          <param name="min">-1</param>
          <param name="max">1</param>
        </command_interface>
        <state_interface name="velocity"/>
      </joint>
      <transmission name="transmission1">
        <plugin>transmission_interface/SimpleTransmission</plugin>
        <joint name="joint1" role="joint1">
          <mechanical_reduction>325.949</mechanical_reduction>
        </joint>
      </transmission>
    </ros2_control>
    <ros2_control name="RRBotModularJoint2" type="actuator">
      <hardware>
        <plugin>ros2_control_demo_hardware/VelocityActuatorHardware</plugin>
        <param name="example_param_write_for_sec">1.23</param>
        <param name="example_param_read_for_sec">3</param>
      </hardware>
      <joint name="joint2">
        <command_interface name="velocity">
          <param name="min">-1</param>
          <param name="max">1</param>
        </command_interface>
        <state_interface name="velocity"/>
      </joint>
    </ros2_control>
    <ros2_control name="RRBotModularPositionSensorJoint1" type="sensor">
      <hardware>
        <plugin>ros2_control_demo_hardware/PositionSensorHardware</plugin>
        <param name="example_param_read_for_sec">2</param>
      </hardware>
      <joint name="joint1">
        <state_interface name="position"/>
      </joint>
    </ros2_control>
    <ros2_control name="RRBotModularPositionSensorJoint2" type="sensor">
      <hardware>
        <plugin>ros2_control_demo_hardware/PositionSensorHardware</plugin>
        <param name="example_param_read_for_sec">2</param>
      </hardware>
      <joint name="joint2">
        <state_interface name="position"/>
      </joint>
    </ros2_control>
  </robot>
  )";

  std::vector<hardware_interface::HardwareInfo> infos =
    hardware_interface::parse_control_resources_from_urdf(urdf_to_test);
  ASSERT_THAT(infos[0].transmissions, SizeIs(1));

  // Transmission loader
  TransmissionPluginLoader loader;
  std::shared_ptr<transmission_interface::TransmissionLoader> transmission_loader =
    loader.create(infos[0].transmissions[0].type);
  ASSERT_TRUE(nullptr != transmission_loader);

  std::shared_ptr<transmission_interface::Transmission> transmission;
  const hardware_interface::TransmissionInfo & info = infos[0].transmissions[0];
  transmission = transmission_loader->load(info);
  ASSERT_TRUE(nullptr != transmission);
  ASSERT_STREQ(infos[0].transmissions[0].joints[0].role.c_str(), "joint1");

  // Validate transmission
  transmission_interface::SimpleTransmission * simple_transmission =
    dynamic_cast<transmission_interface::SimpleTransmission *>(transmission.get());
  ASSERT_TRUE(nullptr != simple_transmission);
  EXPECT_THAT(325.949, DoubleNear(simple_transmission->get_actuator_reduction(), EPS));
  EXPECT_THAT(0.0, DoubleNear(simple_transmission->get_joint_offset(), EPS));
}

TEST(SimpleTransmissionLoaderTest, only_mech_red_specified)
{
  std::string urdf_to_test = std::string(ros2_control_test_assets::urdf_head) + R"(
  <ros2_control name="MinimalSpec" type="actuator">
    <joint name="joint1">
      <command_interface name="velocity">
        <param name="min">-1</param>
        <param name="max">1</param>
      </command_interface>
      <state_interface name="velocity"/>
    </joint>
    <transmission name="transmission1">
      <plugin>transmission_interface/SimpleTransmission</plugin>
      <joint name="joint1" role="joint1">
        <mechanical_reduction>50</mechanical_reduction>
      </joint>
    </transmission>
  </ros2_control>
</robot>
)";
  // Parse transmission info
  std::vector<hardware_interface::HardwareInfo> infos =
    hardware_interface::parse_control_resources_from_urdf(urdf_to_test);
  ASSERT_THAT(infos[0].transmissions, SizeIs(1));

  // Transmission loader
  TransmissionPluginLoader loader;
  std::shared_ptr<transmission_interface::TransmissionLoader> transmission_loader =
    loader.create(infos[0].transmissions[0].type);
  ASSERT_TRUE(nullptr != transmission_loader);

  std::shared_ptr<transmission_interface::Transmission> transmission;
  const hardware_interface::TransmissionInfo & info = infos[0].transmissions[0];
  transmission = transmission_loader->load(info);
  ASSERT_TRUE(nullptr != transmission);

  // Validate transmission
  transmission_interface::SimpleTransmission * simple_transmission =
    dynamic_cast<transmission_interface::SimpleTransmission *>(transmission.get());
  ASSERT_TRUE(nullptr != simple_transmission);
  EXPECT_THAT(50.0, DoubleNear(simple_transmission->get_actuator_reduction(), EPS));
  EXPECT_THAT(0.0, DoubleNear(simple_transmission->get_joint_offset(), EPS));
}

TEST(SimpleTransmissionLoaderTest, offset_and_mech_red_not_specified)
{
  std::string urdf_to_test = std::string(ros2_control_test_assets::urdf_head) + R"(
  <ros2_control name="InvalidSpec" type="actuator">
      <joint name="joint1">
        <command_interface name="velocity">
          <param name="min">-1</param>
          <param name="max">1</param>
        </command_interface>
        <state_interface name="velocity"/>
      </joint>
      <transmission name="transmission1">
        <plugin>transmission_interface/SimpleTransmission</plugin>
        <joint name="joint1" role="joint1">
          <!-- Unspecified element -->
        </joint>
      </transmission>
  </ros2_control>
  </robot>
)";
  // Parse transmission info
  std::vector<hardware_interface::HardwareInfo> infos =
    hardware_interface::parse_control_resources_from_urdf(urdf_to_test);
  ASSERT_THAT(infos[0].transmissions, SizeIs(1));
  // Transmission loader
  TransmissionPluginLoader loader;
  std::shared_ptr<transmission_interface::TransmissionLoader> transmission_loader;
  transmission_loader = loader.create(infos[0].transmissions[0].type);
  ASSERT_TRUE(nullptr != transmission_loader);
  std::shared_ptr<transmission_interface::Transmission> transmission = nullptr;
  const auto trans = infos[0].transmissions[0];
  transmission = transmission_loader->load(trans);
  ASSERT_TRUE(nullptr != transmission);
  transmission_interface::SimpleTransmission * simple_transmission =
    dynamic_cast<transmission_interface::SimpleTransmission *>(transmission.get());
  ASSERT_TRUE(nullptr != simple_transmission);
  EXPECT_THAT(1.0, DoubleNear(simple_transmission->get_actuator_reduction(), EPS));
  EXPECT_THAT(0.0, DoubleNear(simple_transmission->get_joint_offset(), EPS));
}

TEST(SimpleTransmissionLoaderTest, mechanical_reduction_not_a_number)
{
  std::string urdf_to_test = std::string(ros2_control_test_assets::urdf_head) + R"(
  <ros2_control name="InvalidSpec" type="actuator">
      <joint name="joint2">
        <command_interface name="velocity">
          <param name="min">-1</param>
          <param name="max">1</param>
        </command_interface>
        <state_interface name="velocity"/>
      </joint>
      <transmission name="transmission2">
        <plugin>transmission_interface/SimpleTransmission</plugin>
        <joint name="joint2" role="joint1">
          <mechanical_reduction>fifty</mechanical_reduction> <!-- Not a number -->
        </joint>
      </transmission>
  </ros2_control>
  </robot>
)";
  // Parse transmission info
  std::vector<hardware_interface::HardwareInfo> infos =
    hardware_interface::parse_control_resources_from_urdf(urdf_to_test);
  ASSERT_THAT(infos[0].transmissions, SizeIs(1));
  // Transmission loader
  TransmissionPluginLoader loader;
  std::shared_ptr<transmission_interface::TransmissionLoader> transmission_loader;
  transmission_loader = loader.create(infos[0].transmissions[0].type);
  ASSERT_TRUE(nullptr != transmission_loader);
  std::shared_ptr<transmission_interface::Transmission> transmission = nullptr;
  const auto trans = infos[0].transmissions[0];
  transmission = transmission_loader->load(trans);
  ASSERT_TRUE(nullptr != transmission);
  transmission_interface::SimpleTransmission * simple_transmission =
    dynamic_cast<transmission_interface::SimpleTransmission *>(transmission.get());
  ASSERT_TRUE(nullptr != simple_transmission);
  // default kicks in for ill-defined values
  EXPECT_THAT(1.0, DoubleNear(simple_transmission->get_actuator_reduction(), EPS));
}

TEST(SimpleTransmissionLoaderTest, offset_ill_defined)
{
  std::string urdf_to_test = std::string(ros2_control_test_assets::urdf_head) + R"(
  <ros2_control name="InvalidSpec" type="actuator">
      <joint name="joint3">
        <command_interface name="velocity">
          <param name="min">-1</param>
          <param name="max">1</param>
        </command_interface>
        <state_interface name="velocity"/>
      </joint>
      <transmission name="transmission3">
        <plugin>transmission_interface/SimpleTransmission</plugin>
        <joint name="joint3" role="joint1">
          <offset>three</offset> <!-- Not a number -->
          <mechanical_reduction>50</mechanical_reduction>
        </joint>
      </transmission>
  </ros2_control>
  </robot>
)";
  // Parse transmission info
  std::vector<hardware_interface::HardwareInfo> infos =
    hardware_interface::parse_control_resources_from_urdf(urdf_to_test);
  ASSERT_THAT(infos[0].transmissions, SizeIs(1));
  // Transmission loader
  TransmissionPluginLoader loader;
  std::shared_ptr<transmission_interface::TransmissionLoader> transmission_loader;
  transmission_loader = loader.create(infos[0].transmissions[0].type);
  ASSERT_TRUE(nullptr != transmission_loader);
  std::shared_ptr<transmission_interface::Transmission> transmission = nullptr;
  const auto trans = infos[0].transmissions[0];
  transmission = transmission_loader->load(trans);
  ASSERT_TRUE(nullptr != transmission);
  transmission_interface::SimpleTransmission * simple_transmission =
    dynamic_cast<transmission_interface::SimpleTransmission *>(transmission.get());
  ASSERT_TRUE(nullptr != simple_transmission);
  // default kicks in for ill-defined values
  EXPECT_THAT(0.0, DoubleNear(simple_transmission->get_joint_offset(), EPS));
  EXPECT_THAT(50.0, DoubleNear(simple_transmission->get_actuator_reduction(), EPS));
}

TEST(SimpleTransmissionLoaderTest, mech_red_invalid_value)
{
  std::string urdf_to_test = std::string(ros2_control_test_assets::urdf_head) + R"(
  <ros2_control name="InvalidSpec" type="actuator">
      <joint name="joint3">
        <command_interface name="velocity">
          <param name="min">-1</param>
          <param name="max">1</param>
        </command_interface>
        <state_interface name="velocity"/>
      </joint>
      <transmission name="transmission4">
        <plugin>transmission_interface/SimpleTransmission</plugin>
        <joint name="joint3" role="joint1">
          <mechanical_reduction>0</mechanical_reduction>           <!-- Invalid value -->
        </joint>
      </transmission>
  </ros2_control>
  </robot>
)";
  // Parse transmission info
  std::vector<hardware_interface::HardwareInfo> infos =
    hardware_interface::parse_control_resources_from_urdf(urdf_to_test);
  ASSERT_THAT(infos[0].transmissions, SizeIs(1));
  // Transmission loader
  TransmissionPluginLoader loader;
  std::shared_ptr<transmission_interface::TransmissionLoader> transmission_loader;
  transmission_loader = loader.create(infos[0].transmissions[0].type);
  ASSERT_TRUE(nullptr != transmission_loader);
  std::shared_ptr<transmission_interface::Transmission> transmission = nullptr;
  const auto trans = infos[0].transmissions[0];
  transmission = transmission_loader->load(trans);
  ASSERT_TRUE(nullptr == transmission);
}
