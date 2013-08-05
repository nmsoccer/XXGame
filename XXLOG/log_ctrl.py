#!/usr/bin/python

import sys
import getopt
import subprocess

ID_STR="1.1.0.0"

print "executing " , sys.argv[0] , "..."
opts , args= getopt.getopt(sys.argv[1:] , "hsd")

for opt , value in opts:
	if opt == "-h":
		print "------help menu------"
		print "-s: start log server " + ID_STR
		print "-d: shut down log server " + ID_STR
		print "-h: help menu"
	elif opt == "-s":
		print "start log server..."
		cmd_str = "./log_server "
		cmd_str += ID_STR
		subprocess.Popen(cmd_str , shell=True)
	elif opt == "-d":
		print "shut down log server..."
		cmd_str = "killall ./log_server"
		subprocess.Popen(cmd_str , shell=True)
sys.exit()

