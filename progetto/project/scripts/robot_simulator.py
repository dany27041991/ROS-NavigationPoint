#!/usr/bin/env python

import rospy
import rosnode
import time
import numpy as np
import matplotlib.pyplot as plt
from scipy.integrate import odeint
from project.msg import *
from std_msgs.msg import *
from constants import *

w = 0
v = 0
theta_box = 0
x_box = 0
y_box = 0
time = start
theta_var = 0

def model_theta(theta, t):
	dot_theta = w
	return dot_theta
	
def model_x(x, t):
	dot_x = v * np.cos(theta_var)
	return dot_x

def model_y(y, t):
	dot_y = v * np.sin(theta_var)
	return dot_y

def compute(w, v):
	global theta_box
	global x_box
	global y_box
	global time
	global theta_var
	
	print "time = " + str(time)
	
	print "integrating dot_theta, dot_x, dot_y. the results are: ..."
	
	t = np.linspace(time, time+delta, 2)
	
	theta = odeint(model_theta, theta_box, t)
	print "theta (rad) = " + str(theta[-1])
	theta_var = theta[-1]
	curr_theta = theta[-1]
	theta_box = curr_theta
	
	x = odeint(model_x, x_box, t)
	print "x = " + str(x[-1])
	curr_x = x[-1]
	x_box = curr_x
	
	y = odeint(model_y, y_box, t)
	print "y = " + str(y[-1])
	curr_y = y[-1]
	y_box = curr_y
	
	time += delta
	print "current time = " + str(time)
	
	theta_deg = np.rad2deg(curr_theta)

	print "theta (deg) = " + str(theta_deg)
	print "v = " + str(v)

	return curr_x, curr_y, curr_theta

def callback(data, pub):
	global w
	global v
	
	print
	
	frequency.sleep()
	
	Vr = 0
	Vl = 0
	
	if not hasattr(callback, "msg"):
		callback.msg = Input()
		
	callback.msg = data
	
	w = callback.msg.w
	v = callback.msg.v
	
	print "w = " + str(w)
	
	x, y, theta = compute(w, v)
	
	if theta == 0:
		if v == 0:
			Vr = 0
			Vl = 0
		else:
			Vr = v
			Vl = v
	elif theta < 0:
		if v == 0:
			Vr = 0
			Vl = 0
		else:
			Vr = -v
			Vl = v
	else:
		if v == 0:
			Vr = 0
			Vl = 0
		else:
			Vr = v
			Vl = -v
		
	print "Vr = " + str(Vr) + "\tVl = " + str(Vl)
	
	new_msg = Position()
	new_msg.x = x
	new_msg.y = y
	new_msg.theta = theta
	
	while pub.get_num_connections() != 1:
    		frequency.sleep()
    	
	pub.publish(new_msg)
	
def callback_vicon(data):
	print
	
	frequency.sleep()
	
	if not hasattr(callback_vicon, "msg"):
		callback_vicon.msg = Input()
		
	callback_vicon.msg = data
	
	w = callback_vicon.msg.w
	v = callback_vicon.msg.v
	
	print "w = " + str(w)
	print "v = " + str(v)

if __name__ == '__main__':
	try:	
		print "simulator starting ..."
		print

    		rospy.init_node('sim', anonymous = True)
	
		pub = rospy.Publisher('/position_topic', Position, queue_size = 1)
    		frequency = rospy.Rate(rate)
    		
    		sub = rospy.Subscriber('/input_topic', Input, callback, (pub))
    		sub_vicon = rospy.Subscriber('SenderTopic', Input, callback_vicon)

		rospy.spin()
	except rospy.ROSInterruptException:
		print "error"
