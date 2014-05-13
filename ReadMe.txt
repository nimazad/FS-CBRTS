1. Go to tasks directory
	cd tasks/
2. Run my_start.sh
	sudo ./my_start.sh

The script will first try to stop real-time tasks if are running under AdHierSched, so if it is the first time that the script is running it will fail stopping previous tasks and you will get:
	Error: failed to access the module!
	res: -1 stop!

which is fine!

-	Then the module will be compiled and inserted.
-	Afterwards the sample system will be created.
-	Thereafter, the tasks will be attached to the module.
-	By pressing Enter the system will run!


ColorSlider.c is an image processing application. It requires OpenCV packages. It can be used for tests if the required packages are installed.

