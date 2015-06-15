KVERS = $(shell uname -r)

#Kernel modules
obj-m += globalfifo_driver.o
obj-m += globalfifo_device.o

build: kernel_modules

kernel_modules :
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules

clean:
	make -C /lib/modules/$(KVERS/build M=$(CURDIR) clean)
