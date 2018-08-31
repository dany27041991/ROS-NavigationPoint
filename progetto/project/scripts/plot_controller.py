#!/usr/bin/env python

import rospy
import rosnode
import numpy as np
import matplotlib.pyplot as plt
import signal
import sys
from project.msg import *
from std_msgs.msg import *
from constants import *

x_values = []
y_values = []
theta_values = []
iterations = []
w_values = []
time_values = []

x_array = []
y_array = []
theta_array = []

def distance(x1, y1, x2, y2):
	return (np.sqrt(np.power(x2 - x1, 2) + np.power(y2 - y1, 2)))
	
def get_theta(x1, y1, x2, y2):
	return (np.arctan2((y2 - y1), (x2 - x1)))
	
def control_law(theta1, theta2):
	w = K * (theta2 - theta1)
	return w
	
def try_again():
	global xdes
	global ydes
	
	print

	line = raw_input("what do you want to start? ( s = simulator, e = experiment )  or q if you want to quit -> ")
    			
	if line in ['s', 'S']:
		pub = rospy.Publisher('/input_topic', Input, queue_size = 1)
    		frequency = rospy.Rate(rate)
		xdes, ydes = compute_initial_try_again()
		sub = rospy.Subscriber('/position_topic', Position, callback)
	elif line in ['e', 'E']:
		print "experiment starting ..."
		pub_vicon = rospy.Publisher('SenderTopic', Input, queue_size = 1000)
		frequency = rospy.Rate(rate)
		x_des = input("set destination x: ")
		y_des = input("set destination y: ")
		time = start
		sub_vicon = rospy.Subscriber('ViconTopic', ViconMsg, callback_vicon, (pub_vicon))
	elif line in ['q', 'Q']:
		print "closing program ..."
		rospy.signal_shutdown("closing controller")
	else:
		print "wrong character"
		try_again()
	
def show_plot(x, y, theta, w, t):
	print
	
	print "len x: " + str(len(x_array))
	print "len y: " + str(len(y_array))
    		
    	if len(x_array) != len(y_array):
    		x_array.pop()	
    	
    	plt.subplot(2, 1, 1)
    	plt.plot(x, y, color='blue')
	plt.axis([-20, 20, -20, 20])
	plt.xlabel("x")
	plt.ylabel("y")
	plt.title("Motion of the robot")
	plt.grid(True)
	plt.quiver(x, y, 1, 1, color='red', angles=theta, pivot='tip', width=0.01)
	
	plt.subplot(2, 1, 2)
	plt.plot(t, w, color='blue')
	plt.axis([-0.5, 20, -2, 2])
	plt.xlabel("t")
	plt.ylabel("w")
	plt.title("w(t)")
	plt.grid(True)

	plt.show()

	#try_again()
	
	rospy.signal_shutdown("closing controller")
			
def compute_initial():
	x = input("set destination x: ")
	y = input("set destination y: ")
	
	dis = distance(init_x, init_y, x, y)
	theta = get_theta(init_x, init_y, x, y)
	
	theta_test = np.rad2deg(theta)
	if theta_test < -180 or theta_test > 180:
		s = int(theta_test / 180)
		th = theta_test - (s * 180)
		new_th = np.deg2rad(th)
	else:
		new_th = np.deg2rad(theta_test)
	
	w = control_law(init_theta, new_th)
	
	
	if w < -1:
		w = -1
		print "w = " + str(w)
	elif w > 1:
		w = 1
		print "w = " + str(w)
	else:
		print "w = " + str(w)
	
	time = start
	
	msg = Input()
	msg.w = w
	msg.v = v
	
	while pub.get_num_connections() != 1:
    		frequency.sleep()
    	
	pub.publish(msg)
	
	init_theta_deg = np.rad2deg(init_theta)
	
	print "initial time: " + str(time)
	
	with open("./src/project/scripts/log.txt", "w") as f:
		f.write(str(init_x) + " " + str(init_y) + " " + str(init_theta) + "\n")
	f.close()
	
	return x, y
	print
	
def callback_vicon(data, pub_vicon):
	print
	frequency.sleep()
	
	if not hasattr(callback_vicon, "msg"):
		callback_vicon.msg = ViconMsg()
		
	callback_vicon.msg = data
	
	x = callback_vicon.msg.p1 / 1000
	y = callback_vicon.msg.p2 / 1000
	
	theta = np.arccos(callback_vicon.msg.r11)
	
	print "x = " + str(x) + ", y = " + str(y)
	print "theta = " + str(theta)
	
	x_array.append(x)
	y_array.append(y)
	theta_array.append(theta)
	
	i = next(numIter)
	print "iteration: " + str(i)
	iterations.append(i)
	
	curr_time = i * delta
	print "current time: " + str(curr_time)
	time_values.append(curr_time)
	
	if len(x_array) > 1 and len(y_array) > 1 and len(theta_array) > 1:
		dis = distance(x_array[-1], y_array[-1], x_des, y_des)
		print "distance = " + str(dis)
		
		if dis < dist_des:
			print "target reached"
			theta_des = get_theta(x_array[-1], y_array[-1], x_des, y_des)
			w = control_law(theta_array[-1], theta_des)
			
			if w < -1.5:
				w = -1.5
			elif w > 1.5:
				w = 1.5
				
			print "w = " + str(w)
			
			w_values.append(w)
			
			new_msg = Input()
			new_msg.w = w
			new_msg.v = v_stop
	    		
			pub_vicon.publish(new_msg)
			
			rospy.signal_shutdown("closing controller")
		else:
			theta_des = get_theta(x_array[-1], y_array[-1], x_des, y_des)
			w = control_law(theta_array[-1], theta_des)
			w_values.append(w)
			
			if w < -1.5:
				w = -1.5
			elif w > 1.5:
				w = 1.5
			
			print "theta_des = " + str(theta_des)
			print "w = " + str(w)
		
			new_msg = Input()
			new_msg.w = w
			new_msg.v = v_exp
	    		
			pub_vicon.publish(new_msg)
			
		with open("./src/project/scripts/log_vicon.txt", "a") as f:
			f.write(str(x) + " " + str(y) + " " + str(w) + "\n")
		f.close()
		
def callback(data):
	print
	
	frequency.sleep()
	
	if not hasattr(callback, "curr_msg"):
		callback.curr_msg = Position()
		
	callback.curr_msg = data
	
	curr_x = callback.curr_msg.x
	curr_y = callback.curr_msg.y
	curr_theta = callback.curr_msg.theta
	
	compute(curr_x, curr_y, curr_theta)
	
def compute(x, y, theta):
	i = next(numIter)
	print "iteration: " + str(i)
	iterations.append(i)
	
	curr_time = i * delta
	print "current time: " + str(curr_time)
	time_values.append(curr_time)
	
	theta_deg = np.rad2deg(theta)
	print "current theta (deg) = " + str(theta_deg)
		
	x_values.append(x)
	y_values.append(y)
	theta_values.append(theta_deg)
	
	new_dis = distance(x, y, xdes, ydes)
	
	print "distance from the goal position: " + str(new_dis)

	theta_des = get_theta(x, y, xdes, ydes)
	
	theta_test = np.rad2deg(theta_des)
	if theta_test < -180 or theta_test > 180:
		s = int(theta_test / 180)
		th = theta_test - (s * 180)
		new_th = np.deg2rad(th)
	else:
		new_th = np.deg2rad(theta_test)
	
	new_w = control_law(theta, new_th)
	
	
	if new_w < -1:
		new_w = -1
		print "w = " + str(new_w)
	elif new_w > 1:
		new_w = 1
		print "w = " + str(new_w)
	else:
		print "w = " + str(new_w)
		
	w_values.append(new_w)
			
	print "the robot is in x = " + str(x) + ", y = " + str(y)

	if new_dis < dist_des:
	
		new_msg = Input()
		new_msg.w = new_w
		new_msg.v = v_stop
		
		while pub.get_num_connections() != 1:
    			frequency.sleep()
	
		pub.publish(new_msg)
		
		print "target reached"
	
		show_plot(x_values, y_values, theta_values, w_values, time_values)
		
		with open("./src/project/scripts/log.txt", "a") as f:
			f.write(str(x) + " " + str(y) + " " + str(theta_deg) + "\n")
		f.close()
	elif new_dis < dist_near:
		new_msg = Input()
		new_msg.w = new_w
		new_msg.v = v_near
		
		while pub.get_num_connections() != 1:
    			frequency.sleep()
	
		pub.publish(new_msg)
		
		with open("./src/project/scripts/log.txt", "a") as f:
			f.write(str(x) + " " + str(y) + " " + str(theta_deg) + "\n")
		f.close()
	else:
		new_msg = Input()
		new_msg.w = new_w
		new_msg.v = v
		
		while pub.get_num_connections() != 1:
    			frequency.sleep()
	
		pub.publish(new_msg)
		
		with open("./src/project/scripts/log.txt", "a") as f:
			f.write(str(x) + " " + str(y) + " " + str(theta_deg) + "\n")
		f.close()
	print
	
def trial(pub_trial):
	frequency.sleep()
	
	x = 0.1
	y = 0.0
	theta = 1.107
	
	xdes = 45
	ydes = 62
	
	dis = distance(x, y, xdes, ydes)
	theta_des = get_theta(x, y, xdes, ydes)
	w = control_law(theta, theta_des)
		
	print "distance = " + str(dis)
	print "theta_des = " + str(theta_des)
	print "w = " + str(w)
		
	new_msg = Input()
	new_msg.w = w
	new_msg.v = v
	
	while pub_trial.get_num_connections() != 1:
    		frequency.sleep()
    		
    	while True:	
		pub_trial.publish(new_msg)
		
def signal_handler(signal, frame):
        # show_plot da commentare se si effettua l'esperimento.
        show_plot(x_values, y_values, theta_values, w_values, time_values)
        rospy.signal_shutdown("closing controller")
        sys.exit(0)
	
if __name__ == '__main__':
        try:
		global xdes
		global ydes
        	print "controller starting ..."
        	print
        	
		signal.signal(signal.SIGINT, signal_handler)

		numIter = iter(range(1, num_max))
		iterations.append(0)
    		rospy.init_node('con', anonymous = True)
		
    		line = raw_input("what do you want to start? ( s = simulator, e = experiment )  or q if you want to quit -> ")
    		
		if line in ['s', 'S']:
			pub = rospy.Publisher('/input_topic', Input, queue_size = 1)
    			frequency = rospy.Rate(rate)
			xdes, ydes = compute_initial()
			sub = rospy.Subscriber('/position_topic', Position, callback)
		elif line in ['e', 'E']:
			print "experiment starting ..."
			pub_vicon = rospy.Publisher('SenderTopic', Input, queue_size = 1000)
			frequency = rospy.Rate(rate)
			x_des = input("set destination x: ")
			y_des = input("set destination y: ")
			time = start
			sub_vicon = rospy.Subscriber('ViconTopic', ViconMsg, callback_vicon, (pub_vicon))
		elif line in ['q', 'Q']:
			print "closing program ..."
			#print "just kidding"
			#pub_trial = rospy.Publisher('SenderTopic', Input, queue_size = 1000)
			#frequency = rospy.Rate(rate)
			#for i in range(5):
			#	trial(pub_trial)
			rospy.signal_shutdown("closing controller")
		else:
			print "wrong character"
			try_again()
			
		rospy.spin()
        except rospy.ROSInterruptException:
		print "error"
