This module is useful for testing a hypervisor's implementation of the Microsoft "VM Generation ID" feature.

It currently works with Xen and KVM/QEMU.  I've been unable to make it work with Hyper-V because Microsoft used a non-ACPI-spec-compliant HID value ("Hyper_V_Gen_Counter_V1").  The ACPI spec states that this should be a maximum of 9 characters, and Linux strictly enforces it.  I haven't yet added support for VMWare, although that should be trivial.

The feature is specified here:
http://go.microsoft.com/fwlink/p/?LinkID=260709


# Building:

* Make sure you have Linux headers installed
* Run make:
```
[admin@playground src]$ make
make -C /lib/modules/4.1.6-1-ARCH/build M=/home/admin/src modules
make[1]: Entering directory '/usr/lib/modules/4.1.6-1-ARCH/build'

  CC [M]  /home/admin/src/vmgenid-test.o
  Building modules, stage 2.
  MODPOST 1 modules
  CC      /home/admin/src/vmgenid-test.mod.o
  LD [M]  /home/admin/src/vmgenid-test.ko
make[1]: Leaving directory '/usr/lib/modules/4.1.6-1-ARCH/build'
```
# Installing

* Install the kernel module.  This must be run as root:
```
[admin@playground src]$ sudo insmod vmgenid-test.ko
```
* Check that the module is running
```
[admin@playground src]$ lsmod | grep vmgenid
vmgenid_test           16384  0
```
* Check that the VMGENID data was found in ACPI tables
```
[admin@playground src]$ dmesg | tail -2
[48852.399674] VMGENID ADDR method found
[48852.399939] VMGENID module registered
```
# Using

* See the current value of the VM Generation ID GUID
```
[admin@playground src]$ cat /sys/firmware/acpi/vmgenid/guid 
fecb9643-d2c9-d6bf-2aa8-61818f4c1235
```

* See how many VM Generation ID ACPI notify events have been received.  Note that this is the number since the kernel module was installed, not necessarily overall
```
[admin@playground src]$ cat /sys/firmware/acpi/vmgenid/notices 
0
```
