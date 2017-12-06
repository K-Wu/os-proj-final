obj-m :=scpd.o

test_read  := test_read.o 
test_write := test_write.o
test_read.o := test_read.c
test_write.o := test_write.c

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
