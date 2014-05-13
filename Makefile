ifneq ($(KERNELRELEASE),)
	obj-m := AdHierSched.o

# Otherwise we were called directly from the command line;
# invoke the kernel build system
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD	:= $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	@rm -f *.mod.* *.ko *.o .AdHierSched* modules.order Module.* *~
	@rm -fr .tmp*

endif
