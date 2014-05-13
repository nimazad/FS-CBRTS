#!/bin/bash
sleeptime=3
experimet_duration=10
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
./set_app_param $app0  800 200 100 0
# -------------------------	APP 0 ------------------------------

# =========================	Task 0===============================
task0_0=$(./create_task)
./set_task_param $task0_0 800 800 4 98
./attach_task_to_app $task0_0 $app0
./rt_task1 $task0_0 $experimet_duration 1 &

# -------------------------	Task 0 ------------------------------
# =========================	Task 1===============================
task0_1=$(./create_task)
./set_task_param $task0_1 800 800 4 97
./attach_task_to_app $task0_1 $app0
./rt_task1 $task0_1 $experimet_duration 1 &
# -------------------------	Task 1 ------------------------------
# =========================	Task 2===============================
task0_2=$(./create_task)
./set_task_param $task0_2 800 800 4 96
./attach_task_to_app $task0_2 $app0
./rt_task1 $task0_2 $experimet_duration 5 &
# -------------------------	Task 2 ------------------------------

# =========================	APP 1 ==============================
app1=$(./create_app 1) #
./set_app_param $app1 800 300 100 1
# -------------------------	APP 1 ------------------------------
# =========================	Task 3===============================
task1_0=$(./create_task)
./set_task_param $task1_0 800 800 4 95
./attach_task_to_app $task1_0 $app1
./video_decoder $task1_0 $experimet_duration 2 &
# -------------------------	Task 3 ------------------------------
# =========================	Task 4===============================
task1_1=$(./create_task)
./set_task_param $task1_1 800 800 4 97
./attach_task_to_app $task1_1 $app1
./rt_task1 $task1_1 $experimet_duration 1 &
# -------------------------	Task 4 ------------------------------
# =========================	Task 5===============================
task0_ctrl=$(./create_task)
./set_task_param $task0_ctrl 800 800 4 94
./attach_task_to_app $task0_ctrl $app1
./lqr_ctrl $task0_ctrl $experimet_duration $app0 &
# -------------------------	Task 5 ------------------------------


# # =========================	APP ctrl ============================
# appCtrl=$(./create_app 1) #
# ./set_app_param $appCtrl 	200 2500 5 0
# # -------------------------	APP 0 ------------------------------
# # =========================	Task 6===============================
# task0_ctrl=$(./create_task)
# ./set_task_param $task0_ctrl 200 200 4 96
# ./attach_task_to_app $task0_ctrl $appCtrl
# ./lqr_ctrl $task0_ctrl $experimet_duration 0 &
# # -------------------------	Task 6 ------------------------------
# # =========================	APP ctrl ============================


echo "Press Enter to run manager"
read release
dmesg -c
clear
./manager

echo "Press Enter to run applications"
read release
dmesg -c
clear
./run

exit



# =========================	APP 0 ==============================
app0=$(./create_app 1) #
./set_app_param $app0 500 200 120 0
# -------------------------	APP 0 ------------------------------

# =========================	Task 0===============================
task0_0=$(./create_task)
./set_task_param $task0_0 300 300 4 98
./attach_task_to_app $task0_0 $app0
./rt_task1 $task0_0 $experimet_duration 5 &

# -------------------------	Task 0 ------------------------------
# =========================	Task 1===============================
task0_1=$(./create_task)
./set_task_param $task0_1 200 200 4 97
./attach_task_to_app $task0_1 $app0
./rt_task1 $task0_1 $experimet_duration 15 &
# -------------------------	Task 1 ------------------------------

# =========================	Task 2===============================
task0_2=$(./create_task)
./set_task_param $task0_2 200 200 4 96
./attach_task_to_app $task0_2 $app0
./rt_task1 $task0_2 $experimet_duration 15 &
# -------------------------	Task 2 ------------------------------


# =========================	APP 1 ==============================
app1=$(./create_app 1) #
./set_app_param $app1 600 300 60 1
# -------------------------	APP 1 ------------------------------
# =========================	Task 3===============================
task1_0=$(./create_task)
./set_task_param $task1_0 250 250 4 95
./attach_task_to_app $task1_0 $app1
./video_decoder $task1_0 $experimet_duration 0 &
# -------------------------	Task 3 ------------------------------
# =========================	Task 3===============================
# controller
#task1_1=$(./create_task)
#./set_task_param $task1_1 300 300 4 94
#./attach_task_to_app $task1_1 $app1
#./lqr_ctrl $task1_1 $experimet_duration $app0 &
# -------------------------	Task 3 ------------------------------
