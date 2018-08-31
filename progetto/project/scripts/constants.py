#!/usr/bin/env python

start = 0.0
delta = 0.01 # should be 0.01
rate = 100 # should be 100
dist_near = 0.5
dist_des = 0.1
num_max = 100000

# linear and angular speed, gain - Input()
v_exp = 0.112
v = 0.16 # modificato
v_near = 0.08
v_stop = 0
K = 1

# init current position - Position()
init_x = 0
init_y = 0
init_theta = 0
