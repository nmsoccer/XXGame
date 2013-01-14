#!/bin/bash

#start log_server
cd XXLOG
echo "enter"  $PWD "ready to start log server..."
./log_server &
cd ..

#create bus
cd XXBUS
echo "enter" $PWD "ready to create bus..."
./create_bus 1.1.x.x
cd ..

#start connect_server
cd XXCONNECT
echo "enter" $PWD "ready to start connect_server..."
./connect_server 1.1.x.x &
cd ..

#start logic_server
cd XXLOGIC
echo "enter" $PWD "ready to start logic_server..."
./logic_server 1.1.x.x &
cd ..
