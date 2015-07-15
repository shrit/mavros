/**
 * @brief Offboard control test
 * @file offboard_control.h
 * @author Nuno Marques <n.marques21@hotmail.com>
 * @author Andre Nguyen <andre-phu-van.nguyen@polymtl.ca>
 *
 * @addtogroup sitl_test
 * @{
 */
/*
 * Copyright 2015 Nuno Marques, Andre Nguyen.
 *
 * This file is part of the mavros package and subject to the license terms
 * in the top-level LICENSE file of the mavros repository.
 * https://github.com/mavlink/mavros/tree/master/LICENSE.md
 */

#include <sitl_test/sitl_test.h>
#include <sitl_test/test_type.h>
#include <eigen_conversions/eigen_msg.h>

#include <geometry_msgs/Point.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/TwistStamped.h>

namespace testtype {
/**
 * @brief Offboard controller tester
 *
 * Tests offboard position, velocity and acceleration control
 *
 */

typedef enum {
	POSITION,
	VELOCITY,
	ACCELERATION
} control_mode;

typedef enum {
	SQUARE,
	CIRCLE,
	EIGHT,
	ELLIPSE
} path_shape;

class OffboardControl {
public:
	OffboardControl() :
		nh_sp("~"),
		local_pos_sp_pub(nh_sp.advertise<geometry_msgs::PoseStamped>("/mavros/setpoint_position/local", 10)),
		vel_sp_pub(nh_sp.advertise<geometry_msgs::TwistStamped>("/mavros/setpoint_velocity/cmd_vel", 10)),
		local_pos_sub(nh_sp.subscribe("/mavros/local_position/local", 10, &OffboardControl::local_pos_cb, this))
	{ };

	void init() {
		/**
		 * @brief Setpoint control mode selector
		 *
		 * Available modes:
		 * - position
		 * - velocity
		 * - acceleration
		 */
		nh_sp.param<std::string>("mode", mode_, "position");

		/**
		 * @brief Setpoint path shape selector
		 *
		 * Available shapes:
		 * - square
		 * - circle
		 * - eight
		 * - ellipse (in 3D space)
		 */
		nh_sp.param<std::string>("shape", shape_, "square");

		if (mode_ == "position")
			mode = POSITION;
		else if (mode_ == "velocity")
			mode = VELOCITY;
		else if (mode_ == "acceleration")
			mode = ACCELERATION;
		else {
			ROS_ERROR_NAMED("sitl_test", "Control mode: wrong/unexistant control mode name %s", mode_.c_str());
			return;
		}

		if (shape_ == "square")
			shape = SQUARE;
		else if (shape_ == "circle")
			shape = CIRCLE;
		else if (shape_ == "eight")
			shape = EIGHT;
		else if (shape_ == "ellipse")
			shape = ELLIPSE;
		else {
			ROS_ERROR_NAMED("sitl_test", "Path shape: wrong/unexistant path shape name %s", shape_.c_str());
			return;
		}
	}

	/* -*- main routine -*- */

	void spin(int argc, char *argv[]) {
		init();
		ros::Rate loop_rate(10);
		ROS_INFO("SITL Test: Offboard control test running!");

		if (mode == POSITION) {
			ROS_INFO("Position control mode selected.");
		}
		else if (mode == VELOCITY) {
			ROS_INFO("Velocity control mode selected.");
		}
		else if (mode == ACCELERATION) {
			ROS_INFO("Acceleration control mode selected.");
			ROS_ERROR_NAMED("sitl_test", "Control mode: acceleration control mode not supported in PX4 current Firmware.");
			/**
			 * @todo: lacks firmware support, for now
			 */
			return;
		}

		if (shape == SQUARE) {
			ROS_INFO("Test option: square-shaped path...");
			square_path_motion(loop_rate, mode);
		}
		else if (shape == CIRCLE) {
			ROS_INFO("Test option: circle-shaped path...");
			circle_path_motion(loop_rate, mode);
		}
		else if (shape == EIGHT) {
			ROS_INFO("Test option: eight-shaped path...");
			eight_path_motion(loop_rate, mode);
		}
		else if (shape == ELLIPSE) {
			ROS_INFO("Test option: ellipse-shaped path...");
			ellipse_path_motion(loop_rate, mode);
		}
	}

private:
	control_mode mode;
	path_shape shape;

	ros::NodeHandle nh_sp;
	ros::Publisher local_pos_sp_pub, vel_sp_pub;
	ros::Subscriber local_pos_sub;

	std::string mode_, shape_;

	geometry_msgs::PoseStamped localpos, ps;
	geometry_msgs::TwistStamped vs;

	Eigen::Vector3d current;

	/* -*- helper functions -*- */

	/**
	 * @brief Defines single position setpoint
	 */
	Eigen::Vector3d pos_setpoint(){
		/** @todo Give possibility to user define amplitude of movement (square corners coordinates)*/
		return Eigen::Vector3d(2.0f, 2.0f, 1.0f);	// meters
	}

	/**
	 * @brief Defines circle path
	 */
	Eigen::Vector3d circle_shape(int angle){
		/** @todo Give possibility to user define amplitude of movement (circle radius)*/
		double r = 5.0f;	// 5 meters radius

		return Eigen::Vector3d(r * cos(angle * M_PI / 180.0f),
				r * sin(angle * M_PI / 180.0f),
				1.0f);;
	}

	/**
	 * @brief Defines Gerono lemniscate path
	 */
	Eigen::Vector3d eight_shape(int angle){
		/** @todo Give possibility to user define amplitude of movement (vertical tangent size)*/
		double a = 5.0f;	// vertical tangent with 5 meters size

		return Eigen::Vector3d(a * cos(angle * M_PI / 180.0f),
				a * sin(angle * M_PI / 180.0f) * cos(angle * M_PI / 180.0f),
				1.0f);;
	}

	/**
	 * @brief Defines ellipse path
	 */
	Eigen::Vector3d ellipse_shape(int angle){
		/** @todo Give possibility to user define amplitude of movement (tangent sizes)*/
		double a = 5.0f;	// major axis
		double b = 2.0f;	// minor axis

		// rotation around y-axis
		return Eigen::Vector3d(a * cos(angle * M_PI / 180.0f),
				0.0f,
				2.5f + b * sin(angle * M_PI / 180.0f));;
	}

	/**
	 * @brief Square path motion routine
	 */
	void square_path_motion(ros::Rate loop_rate, control_mode mode){
		uint8_t pos_target = 1;

		ROS_INFO("Testing...");

		while (ros::ok()) {
			wait_and_move(ps);

			// motion routine
			switch (pos_target) {
			case 1:
				tf::pointEigenToMsg(pos_setpoint(), ps.pose.position);
				break;
			case 2:
				tf::pointEigenToMsg(Eigen::Vector3d(-pos_setpoint().x(),
							pos_setpoint().y(),
							pos_setpoint().z()),
						ps.pose.position);
				break;
			case 3:
				tf::pointEigenToMsg(Eigen::Vector3d(-pos_setpoint().x(),
							-pos_setpoint().y(),
							pos_setpoint().z()),
						ps.pose.position);
				break;
			case 4:
				tf::pointEigenToMsg(Eigen::Vector3d( pos_setpoint().x(),
							-pos_setpoint().y(),
							pos_setpoint().z()),
						ps.pose.position);
				break;
			case 5:
				tf::pointEigenToMsg(pos_setpoint(), ps.pose.position);
				break;
			default:
				break;
			}

			if (pos_target == 6) {
				ROS_INFO("Test complete!");
				ros::shutdown();
			}
			else
				++pos_target;

			loop_rate.sleep();
			ros::spinOnce();
		}
	}

	/**
	 * @brief Circle path motion routine
	 */
	void circle_path_motion(ros::Rate loop_rate, control_mode mode){
		ROS_INFO("Testing...");

		while (ros::ok()) {
			tf::pointMsgToEigen(localpos.pose.position, current);

			// starting point
			if (mode == POSITION) {
				tf::pointEigenToMsg(Eigen::Vector3d(5.0f, 0.0f, 1.0f), ps.pose.position);
				local_pos_sp_pub.publish(ps);
			}
			else if (mode == VELOCITY) {
				tf::vectorEigenToMsg(Eigen::Vector3d(5.0f - current.x(), -current.y(), 1.0f - current.z()), vs.twist.linear);
				vel_sp_pub.publish(vs);
			}
			else if (mode == ACCELERATION) {
				// TODO
				return;
			}

			wait_and_move(ps);

			// motion routine
			for (int theta = 0; theta <= 360; theta++) {
				tf::pointMsgToEigen(localpos.pose.position, current);

				if (mode == POSITION) {
					tf::pointEigenToMsg(circle_shape(theta), ps.pose.position);
					local_pos_sp_pub.publish(ps);
				}
				else if (mode == VELOCITY) {
					tf::vectorEigenToMsg(circle_shape(theta) - current, vs.twist.linear);
					vel_sp_pub.publish(vs);
				}
				else if (mode == ACCELERATION) {
					// TODO
					return;
				}
				if (theta == 360) {
					ROS_INFO("Test complete!");
					ros::shutdown();
				}
				loop_rate.sleep();
				ros::spinOnce();
			}
		}
	}

	/**
	 * @brief Eight path motion routine
	 */
	void eight_path_motion(ros::Rate loop_rate, control_mode mode){
		ROS_INFO("Testing...");

		while (ros::ok()) {
			tf::pointMsgToEigen(localpos.pose.position, current);

			// starting point
			if (mode == POSITION) {
				tf::pointEigenToMsg(Eigen::Vector3d(0.0f, 0.0f, 1.0f), ps.pose.position);
				local_pos_sp_pub.publish(ps);
			}
			else if (mode == VELOCITY) {
				tf::vectorEigenToMsg(Eigen::Vector3d(-current.x(), -current.y(), 1.0f - current.z()), vs.twist.linear);
				vel_sp_pub.publish(vs);
			}
			else if (mode == ACCELERATION) {
				// TODO
				return;
			}

			wait_and_move(ps);

			// motion routine
			for (int theta = -180; theta <= 180; theta++) {
				tf::pointMsgToEigen(localpos.pose.position, current);

				if (mode == POSITION) {
					tf::pointEigenToMsg(eight_shape(theta), ps.pose.position);
					local_pos_sp_pub.publish(ps);
				}
				else if (mode == VELOCITY) {
					tf::vectorEigenToMsg(eight_shape(theta) - current, vs.twist.linear);
					vel_sp_pub.publish(vs);
				}
				else if (mode == ACCELERATION) {
					// TODO
					return;
				}
				if (theta == 180) {
					ROS_INFO("Test complete!");
					ros::shutdown();
				}
				loop_rate.sleep();
				ros::spinOnce();
			}
		}
	}

	/**
	 * @brief Ellipse path motion routine
	 */
	void ellipse_path_motion(ros::Rate loop_rate, control_mode mode){
		ROS_INFO("Testing...");

		while (ros::ok()) {
			tf::pointMsgToEigen(localpos.pose.position, current);

			// starting point
			if (mode == POSITION) {
				tf::pointEigenToMsg(Eigen::Vector3d(0.0f, 0.0f, 2.5f), ps.pose.position);
				local_pos_sp_pub.publish(ps);
			}
			else if (mode == VELOCITY) {
				// This one gets some strange behavior, maybe due to overshoot on velocity controller
				// TODO: find a way to limit the velocity between points (probably using ros::Rate)
				tf::vectorEigenToMsg(Eigen::Vector3d(-current.x(), -current.y(), 2.5f - current.z()), vs.twist.linear);
				vel_sp_pub.publish(vs);
			}
			else if (mode == ACCELERATION) {
				// TODO
				return;
			}

			wait_and_move(ps);

			// motion routine
			for (int theta = 0; theta <= 360; theta++) {
				tf::pointMsgToEigen(localpos.pose.position, current);

				if (mode == POSITION) {
					tf::pointEigenToMsg(ellipse_shape(theta), ps.pose.position);
					local_pos_sp_pub.publish(ps);
				}
				else if (mode == VELOCITY) {
					tf::vectorEigenToMsg(ellipse_shape(theta) - current, vs.twist.linear);
					vel_sp_pub.publish(vs);
				}
				else if (mode == ACCELERATION) {
					// TODO
					return;
				}
				if (theta == 360) {
					ROS_INFO("Test complete!");
					ros::shutdown();
				}
				loop_rate.sleep();
				ros::spinOnce();
			}
		}
	}

	/**
	 * @brief Defines the accepted threshold to the destination/target position
	 * before moving to the next setpoint.
	 */
	void wait_and_move(geometry_msgs::PoseStamped target){
		ros::Rate loop_rate(10);

		bool stop = false;
		double distance;

		Eigen::Vector3d dest;

		while (ros::ok() && !stop) {
			tf::pointMsgToEigen(target.pose.position, dest);
			tf::pointMsgToEigen(localpos.pose.position, current);

			distance = sqrt((dest - current).x() * (dest - current).x() +
					(dest - current).y() * (dest - current).y() +
					(dest - current).z() * (dest - current).z());

			if (distance <= 0.1f)	/** @todo Add gaussian threshold */
				stop = true;

			if (mode == POSITION) {
				local_pos_sp_pub.publish(target);
			}
			else if (mode == VELOCITY) {
				tf::vectorEigenToMsg(dest - current, vs.twist.linear);
				vel_sp_pub.publish(vs);
			}
			else if (mode == ACCELERATION) {
				// TODO
				return;
			}
			loop_rate.sleep();
			ros::spinOnce();
		}
	}

	/* -*- callbacks -*- */

	void local_pos_cb(const geometry_msgs::PoseStampedConstPtr& msg){
		localpos = *msg;
	}
};
};	// namespace testype
