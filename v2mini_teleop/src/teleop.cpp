#include <geometry_msgs/Twist.h>
#include <sensor_msgs/JointState.h>
#include <tf/transform_broadcaster.h>
#include <ros/ros.h>
#include <stdlib.h>
#include <stdio.h>

#include "v2mini_teleop/teleop_controller.h"

using v2mini_teleop::ROBOT_VEL;

int main(int argc, char ** argv) {

	ros::init(argc, argv, "teleop");
	ros::NodeHandle n;

	ros::Publisher base_pub = n.advertise<geometry_msgs::Twist>("base_cmds", 1);
	ros::Publisher torso_pub = n.advertise<geometry_msgs::Twist>("torso_cmds", 1);
	ros::Publisher joint_pub = n.advertise<sensor_msgs::JointState>("joint_states", 1);
	tf::TransformBroadcaster broadcaster;

	geometry_msgs::TransformStamped odom_trans;
	sensor_msgs::JointState joint_state;
    odom_trans.header.frame_id = "odom";
    odom_trans.child_frame_id = "base_link";

    const double degree = M_PI/180;

    // robot state
    double tilt = 0, tinc = degree, swivel=0, angle=0, height=0, hinc=0.005;

	// Get type of control from param (auto or remote)
	std::string controller_type;
	ros::NodeHandle pnh("~");

	// Controlled by either keyboard or gamepad
	pnh.getParam("controller", controller_type);

	// Use keyboard controller as default (no arg).
	if (controller_type == "")
	{
		controller_type = "keyboard";
	}

	printf("Controller: %s\n", controller_type.c_str());

	float* cmds = NULL;
	bool quit = false;

	v2mini_teleop::TeleopController controller;

	SDL_Event event;

	ros::Rate loop_rate(50);

	while(ros::ok() && !controller.checkQuitStatus() && !quit)
	{

		geometry_msgs::Twist base_cmds;
		geometry_msgs::Twist torso_cmds;

		//update joint_state
		joint_state.header.stamp = ros::Time::now();
		joint_state.name.resize(3);
		joint_state.position.resize(3);
		joint_state.name[0] ="swivel";
		joint_state.position[0] = swivel;
		joint_state.name[1] ="tilt";
		joint_state.position[1] = tilt;
		joint_state.name[2] ="periscope";
		joint_state.position[2] = height;

		// update transform
		// (moving in a circle with radius=2)
		odom_trans.header.stamp = ros::Time::now();
		odom_trans.transform.translation.x = cos(angle)*2;
		odom_trans.transform.translation.y = sin(angle)*2;
		odom_trans.transform.translation.z = .7;
		odom_trans.transform.rotation = tf::createQuaternionMsgFromYaw(angle+M_PI/2);

		joint_pub.publish(joint_state);
		broadcaster.sendTransform(odom_trans);

		// Create new robot state
		tilt += tinc;
		if (tilt<-.5 || tilt>0) tinc *= -1;
		height += hinc;
		if (height>.2 || height<0) hinc *= -1;
		swivel += degree;
		angle += degree/4;

		while(SDL_PollEvent(&event) != 0)
		{

			if (event.type == SDL_QUIT)
			{
				quit = true;
			}

			// getKeyCmds by default (also checks 'esc' button)
			cmds = controller.getKeyCmds();

			if (controller_type == "gamepad")
			{
				// overwrite the pointer if gamecontroller is used
				cmds = controller.getGamepadCmds();
			}

		}

		base_cmds.linear.x = cmds[v2mini_teleop::BASE_VELX];
		base_cmds.linear.y = cmds[v2mini_teleop::BASE_VELY];
		base_cmds.angular.z = cmds[v2mini_teleop::BASE_VELZ];

		torso_cmds.linear.x = cmds[v2mini_teleop::FACE];
		torso_cmds.linear.y = cmds[v2mini_teleop::TORSO_VEL];

		//publish the movement commands
		base_pub.publish(base_cmds);
		torso_pub.publish(torso_cmds);

		loop_rate.sleep();
	}

	cmds = NULL;

	return 0;
}