#!/bin/bash
sleeptime=3
experimet_duration=1500
dmesg -c
clear
./stop
echo "=====***===== stopping hsf module finished!"

cd library
make clean
make
echo "=====***=====make api functions finished!"

userinc="/usr/lib/AdHierSched"
# make include directories for AdHierSched
if [ ! -d $userinc ]; then
	mkdir -p $userinc
fi
# copy header files.
cp -f ./libhsf.a $userinc/libAdHierSched.a
chmod 644 $userinc/libAdHierSched.a
userincold="/usr/lib/resch"
cp -f ./libhsf.a $userincold/libresch.a
chmod 644 $userincold/libresch.a
echo "=====***=====copying libAdHierSched finished!"

cd ..
make clean
make
echo "=====***=====make task files finished!"
sleep $sleeptime

clear
cd ..
cd script
./uninstall
echo "=====***=====removing hsf module finished!"
sleep $sleeptime
clear
./install
echo "=====***===== installing hsf module finished!"
cd ..
cd tasks

#(queue_type, server_type);
echo "Press Enter to create the system: "
read create
clear

# =========================	APP 0 ==============================
app0=$(./create_app 1) #
./set_app_param $app0 50 20 222 1
# -------------------------	APP 0 ------------------------------
# =========================	SERVER 0============================
server0=$(./create_server) 
#		id   period 	deadline budget	priority proc_id
./set_server_param $server0 500 500 500 0 1
./attach_server_to_app $server0 $app0
# -------------------------	Server 0 ----------------------------
# =========================	Task 0===============================
task0_0=$(./create_task)
./set_task_param $task0_0 20 20 4 98
./attach_task_to_app $task0_0 $app0
./rt_edge_detector $task0_0 $experimet_duration 1 &
#./rt_task1 $task0_0 $experimet_duration 10 &
# -------------------------	Task 0 ------------------------------



echo "Press Enter to run servers for the solo task"
read release
dmesg -c
clear

./run
exit
