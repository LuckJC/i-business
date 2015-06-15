#KVERS = $(shell uname -r)
KVERS = /home/jovec/gq2440/linux-2.6.32.2

#Kernel modules
obj-m += virt_net.o

build: kernel_modules

kernel_modules :
	make -C /home/jovec/gq2440/linux-2.6.32.2 M=$(CURDIR) modules

clean:
	make -C /lib/modules/$(KVERS/build M=$(CURDIR) clean)
