#!/bin/bash
sleeptime=3
experimet_duration=400
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
# #=========================================== SERVER 0
task0_0=$(./create_task)
./set_task_param $task0_0 200 200 1 99 
echo "task id" $task0_0

task0_1=$(./create_task)
./set_task_param $task0_1 400 400 1 98
echo "task id" $task0_1

server0=$(./create_server 1 0) #1 = CBS
./set_server_param $server0 100 100 39 0 0
echo "server id" $server0
#attaching
./attach_task $server0 $task0_0 0
./attach_task $server0 $task0_1 0
# #=========================================== SERVER 0


#=========================================== SERVER 1
server1=$(./create_server 1 0) #1 = CBS
./set_server_param $server1 10 10 6 0 0
echo "server id" $server1

#======================== SERVER 1_0 ===============
task1_0_0=$(./create_task)
./set_task_param $task1_0_0 40 40 4 97
echo "task id" $task1_0_0

task1_0_1=$(./create_task)
./set_task_param $task1_0_1 200 200 1 96
echo "task id" $task1_0_1

server1_0=$(./create_server 1 0) #1 = CBS
./set_server_param $server1_0 20 20 3 0 0
echo "server id" $server1_0
#attaching
./attach_task $server1_0 $task1_0_0 0
./attach_task $server1_0 $task1_0_1 0
./attach_server $server1 $server1_0
#======================== SERVER 1_0 ===============
#======================== SERVER 1_1 ===============
task1_1_0=$(./create_task)
./set_task_param $task1_1_0 250 250 1 95
echo "task id" $task1_1_0

task1_1_1=$(./create_task)
./set_task_param $task1_1_1 200 200 1 94
echo "task id" $task1_1_1

server1_1=$(./create_server 0 0) #1 = CBS
./set_server_param $server1_1 100 100 12 0 0
echo "server id" $server1_1
#attaching
./attach_task $server1_1 $task1_1_0 0
./attach_task $server1_1 $task1_1_1 0
./attach_server $server1 $server1_1
#======================== SERVER 1_1 ===============
#======================== SERVER 1_2 ===============
task1_2_0=$(./create_task)
./set_task_param $task1_2_0 250 250 1 93
echo "task id" $task1_2_0

task1_2_1=$(./create_task)
./set_task_param $task1_2_1 150 150 1 92
echo "task id" $task1_2_1

server1_2=$(./create_server 0 0) #1 = CBS
./set_server_param $server1_2 75 75 9 0 0
echo "server id" $server1_2
#attaching
./attach_task $server1_2 $task1_2_0 0
./attach_task $server1_2 $task1_2_1 0
./attach_server $server1 $server1_2
#======================== SERVER 1_2 ===============
dmesg -c

echo "Press Enter to create the tasks:"
read create
clear
#======================== Server 0 ==================
./rt_task1 $task0_0 $experimet_duration &
./rt_task1 $task0_1 $experimet_duration &

#======================== Server 1_0 ==================
#echo $task1_0_0$'\n'$task1_0_0 > mplayer_config.txt
#mplayer skyfall.mov &
#./ColorSlider $task1_0_1 $experimet_duration 2 &
./rt_task1 $task1_0_0 $experimet_duration &
./rt_task1 $task1_0_1 $experimet_duration &
#======================== Server 1_1 ==================
#./ColorSlider $task1_1_0 $experimet_duration 1 &
#./ColorSlider $task1_1_1 $experimet_duration 5 &
./rt_task1 $task1_1_0 $experimet_duration &
./rt_task1 $task1_1_1 $experimet_duration &
#======================== Server 1_2 ==================
# ./ColorSlider $task1_2_0 $experimet_duration 5 &
# ./ColorSlider $task1_2_1 $experimet_duration 2 &
./rt_task1 $task1_2_0 $experimet_duration &
./rt_task1 $task1_2_1 $experimet_duration &
dmesg -c
sleep 1

echo "Press Enter to run"
read create
./run


for i in {1..60}
do
  sleep 1
  echo "---------------------------------------------------------------------------------------------------------------------------------"
  echo "-----------------------------------------------------$i seconds------------------------------------------------------------------"
  echo "---------------------------------------------------------------------------------------------------------------------------------"
done

 ./stop
 exit
# ./set_task_param $task1_1_0 200 200 1 96
# ./set_server_param $server_cbs0 50 50 5 1 1

for i in {20..120}
do
  sleep 1
  echo "---------------------------------------------------------------------------------------------------------------------------------"
  echo "-----------------------------------------------------$i seconds------------------------------------------------------------------"
  echo "---------------------------------------------------------------------------------------------------------------------------------"
done

./stop

exit



