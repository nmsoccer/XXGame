#!/usr/bin/python

import sys
import getopt
import subprocess

ID_STR = "1.1.0.0"

#cmd_str=""


print "execute " , sys.argv[0] , "..."
opts,args = getopt.getopt(sys.argv[1:] , "hsd")


for op , value in opts:
	if op == "-s":
		print "it is create..."
		cmd_str = "./create_bus "
		cmd_str += ID_STR
		subprocess.Popen(cmd_str , shell=True)
	elif op == "-d":
		print "it is delete..."
		cmd_str = "./delete_bus "
		cmd_str += ID_STR
		subprocess.Popen(cmd_str , shell=True)
	elif op == "-h":
		print "------help menu------"
		print "-s: create bus " + ID_STR
		print "-d: delete bus " + ID_STR
		print "-h: help"
	else:
		print "~~~~"

sys.exit()

#judge argument

