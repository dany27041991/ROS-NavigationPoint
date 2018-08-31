#include "../lib/viconlib/DataStreamClient.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "ros/ros.h"
#include "project/ViconMsg.h"

#include <cstdio>
#define output_stream if(!LogFile.empty()) ; else std::cout

/****GLOBAL VARIABLE***/
bool keepRunning = true; // take the central while alive
float GlobalTranslationX;
float GlobalTranslationY;
float GlobalTranslationZ;
float GlobalRotationX;
float GlobalRotationY;
float GlobalRotationZ;
std::string RobotName;
std::string SegmentName;
std::string SubjectName;
std::string LogFile = "";

project::ViconMsg msgV;

using namespace ViconDataStreamSDK::CPP;

namespace
{
	std::string Adapt(const bool i_Value)
	{
		return i_Value ? "True" : "False";
	}
}

void signal_handler_fn(int dummy)
{
	keepRunning = false;
	std::cout << "Catched" << std::endl;
}

int main(int argc, char* argv[])
{

/*******************************************************************************/
/*                              VICON STUFFS                                   */
/*******************************************************************************/

	/* Process the input argument to set the RobotName */
	if(argc!=2) // ask for parameter
	{
		std::cout << "Error: Please insert the second parameter, i.e. GreenLantern" << std::endl;
		return 0;
	}
	else
	{
		if(strcmp(argv[1], "GreenLantern") == 0) // strcmp return 0 if strings are equal
		{
			RobotName = "GreenLantern";
			std::cout << "Robot Name is GreenLantern." << std::endl;
		}
		else
		{
			std::cout << "Error: Unexpected value for the second parameter, use 'GreenLantern'" << std::endl;
			return 0;
		}
	} // endif on argc

	SubjectName = RobotName;
	SegmentName = RobotName;

	// IP address of Vicon Server
	std::string HostName = "10.0.0.1:801";

	// Create a new Vicon Client
	Client MyClient;

	// Try to connect to the Server
	std::cout << "Connecting to " << HostName << " ..." << std::flush;
	while(!MyClient.IsConnected().Connected)
	{
		// Direct connection
		std::cout << "No Connection " << std::flush;
		MyClient.Connect(HostName);
		std::cout << ".";
		sleep(1);
	}

	// When connection is up, enable some different data types
	MyClient.EnableSegmentData();
	MyClient.EnableMarkerData();
	MyClient.EnableUnlabeledMarkerData();
	MyClient.EnableDeviceData();

	std::cout << "Connected! "<< std::endl;
	sleep(1);

	Output_GetSegmentGlobalTranslation GlobalTranslation;
	Output_GetSegmentGlobalRotationMatrix _Output_GetSegmentGlobalRotationMatrix;

/*******************************************************************************/
/*                                ROS STUFFS                                   */
/*******************************************************************************/
	ros::init(argc, argv, "vicon_server");
	ros::NodeHandle n;

	// This node will publish its messages onto ViconTopic               queue_size
	ros::Publisher vicon_pub = n.advertise<project::ViconMsg>("ViconTopic", 1000);

	// This node will publish messages at 100 Hz
	ros::Rate loop_rate(100);

	std::cout <<"Starting Connection to ROS Environment."<< std::endl;
	sleep(3);

	signal(SIGINT, signal_handler_fn);
	std::cout <<"Ready ..."<< std::endl;

	//Print data on file
	FILE *f;
	f = fopen("position.txt", "w");

/*******************************************************************************/
/*                                   CORE                                      */
/*******************************************************************************/
	while(keepRunning)
	{
		// Get a frame
		while(MyClient.GetFrame().Result != Result::Success)
		{
			// Sleep a little so that we don't lumber the CPU with a busy poll
			std::cout << "GetFrame() Unsuccessfully, sleeping ..." << std::endl;
			sleep(0.5);
			std::cout << "!";
		}

		// Get the global segment translation
		GlobalTranslation = MyClient.GetSegmentGlobalTranslation(SubjectName, SegmentName);

		// Get the global segment rotation as a matrix
		_Output_GetSegmentGlobalRotationMatrix = MyClient.GetSegmentGlobalRotationMatrix(SubjectName, SegmentName);

		GlobalTranslationX=GlobalTranslation.Translation[0];
		GlobalTranslationY=GlobalTranslation.Translation[1];
		GlobalTranslationZ=GlobalTranslation.Translation[2];

		// Save p1, p2, p3
		msgV.p1 = GlobalTranslationX;
		msgV.p2 = GlobalTranslationY;
		msgV.p3 = GlobalTranslationZ;

		// Save Rotation Matrix
		msgV.r11 = _Output_GetSegmentGlobalRotationMatrix.Rotation[0];
		msgV.r12 = _Output_GetSegmentGlobalRotationMatrix.Rotation[1];
		msgV.r13 = _Output_GetSegmentGlobalRotationMatrix.Rotation[2];
		msgV.r21 = _Output_GetSegmentGlobalRotationMatrix.Rotation[3];
		msgV.r22 = _Output_GetSegmentGlobalRotationMatrix.Rotation[4];
	 	msgV.r23 = _Output_GetSegmentGlobalRotationMatrix.Rotation[5];
 		msgV.r31 = _Output_GetSegmentGlobalRotationMatrix.Rotation[6];
		msgV.r32 = _Output_GetSegmentGlobalRotationMatrix.Rotation[7];
		msgV.r33 = _Output_GetSegmentGlobalRotationMatrix.Rotation[8];

		fprintf(f, "%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", GlobalTranslationX, GlobalTranslationY, GlobalTranslationZ, msgV.r11, msgV.r12, msgV.r13, msgV.r21, msgV.r22, msgV.r23, msgV.r31, msgV.r32, msgV.r33);
		
		// publish the message onto ViconTopic
		vicon_pub.publish(msgV);

		loop_rate.sleep();

	} // end while

	MyClient.DisableSegmentData();
	MyClient.DisableMarkerData();
	MyClient.DisableUnlabeledMarkerData();
	MyClient.DisableDeviceData();
	MyClient.Disconnect();
	fclose(f);
	sleep(1);

	return 0;
}
