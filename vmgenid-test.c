#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/acpi.h>
#include <acpi/acpi_bus.h>
#include <acpi/acpi_drivers.h>

MODULE_AUTHOR("Ben Warren <ben@skyportsystems.com>");
MODULE_DESCRIPTION("Test Driver for Windows VM Generation ID");
MODULE_LICENSE("GPL");

static int vmgenid_add(struct acpi_device *device);
static int vmgenid_remove(struct acpi_device *device);
static void vmgenid_notify(struct acpi_device *device, u32 event);

static u64 vmgenid_addr;
static int num_notices;
static u8 guid_val[16];

static struct kobject *vmgenid_kobj;

static const struct acpi_device_id vmgenid_device_ids[] = {
    { "QEMUVGID", 0}, /* QEMU */
    { "XEN0000", 0},  /* Xen */
    { "", 0},
};
MODULE_DEVICE_TABLE(acpi, vmgenid_device_ids);

static ssize_t notices_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    return sprintf(buf, "%d\n", num_notices);
}
 
static ssize_t guid_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
    return sprintf(buf, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
            guid_val[0], guid_val[1], guid_val[2], guid_val[3],
            guid_val[4], guid_val[5], guid_val[6], guid_val[7],
            guid_val[8], guid_val[9], guid_val[10], guid_val[11],
            guid_val[12], guid_val[13], guid_val[14], guid_val[15]);
}

static struct kobj_attribute notices_attribute = __ATTR(notices, S_IRUGO, notices_show, NULL);
static struct kobj_attribute guid_attribute = __ATTR(guid, S_IRUGO, guid_show, NULL);
 
static struct attribute *attrs [] =
{
    &notices_attribute.attr,
    &guid_attribute.attr,
    NULL,
};
 
static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct acpi_driver vmgenid_driver = {
    .name  = "VM Generation ID",
    .class = "Windows",
    .ids   = vmgenid_device_ids,
    .ops   = {
        .add    = vmgenid_add,
        .remove = vmgenid_remove,
        .notify = vmgenid_notify,
    },
    .owner = THIS_MODULE,
};

static int read_vmgenid_guid(void)
{
    u8 *guid;
    int i;

    if (vmgenid_addr == 0) {
        printk(KERN_ERR "ADDR not initialized\n");
        return -EINVAL;
    }

    guid = ioremap_nocache(vmgenid_addr, 16);
    if (!guid) {
        printk(KERN_ERR "Unable to map memory for access\n");
        return -EINVAL;
    }
    for (i = 0; i < 16; i++) {
        guid_val[i] = ioread8(guid + i);
    }
    iounmap(guid);
    return 0;
}

static int vmgenid_enable(acpi_handle handle)
{
    acpi_status result;
    struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
    union acpi_object *obj;

    if (acpi_has_method(handle, "ADDR")) {
        printk(KERN_INFO "VMGENID ADDR method found\n");
        result = acpi_evaluate_object(handle, "ADDR", NULL, &buffer);
        if (ACPI_FAILURE(result))
            return -EINVAL;
        
        obj = buffer.pointer;
        if (!obj || obj->type != ACPI_TYPE_PACKAGE || obj->package.count != 2 ) {
            printk(KERN_INFO "Invalid ADDR data\n");
            return -EINVAL;
        }
        vmgenid_addr = obj->package.elements[0].integer.value +
            (obj->package.elements[1].integer.value << 32);
        return read_vmgenid_guid();
    }

    return 0;
}

static int vmgenid_add(struct acpi_device *device)
{
    return vmgenid_enable(device->handle);
}

static int __init vmgenid_init(void)
{
    if (acpi_bus_register_driver(&vmgenid_driver) < 0) {
        ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
                              "Error registering VMGENID\n"));
        printk(KERN_INFO "Unable to register VMGENID\n");
        return -ENODEV;
    }
    vmgenid_kobj = kobject_create_and_add("vmgenid", acpi_kobj);
    if (sysfs_create_group(vmgenid_kobj, &attr_group) < 0) {
        kobject_put(vmgenid_kobj);
    }
    printk(KERN_INFO "VMGENID module registered\n");
    return 0;
}

static int vmgenid_remove(struct acpi_device *device)
{
    return 0;
}

static void vmgenid_notify(struct acpi_device *device, u32 event)
{
    printk(KERN_INFO "Received a VMGENID ACPI notification.  Event = %u\n", event);
    num_notices++;
    read_vmgenid_guid();
}

static void __exit vmgenid_exit(void)
{
    acpi_bus_unregister_driver(&vmgenid_driver);
    sysfs_remove_group(vmgenid_kobj, &attr_group);
    kobject_put(vmgenid_kobj);
}

module_init(vmgenid_init);
module_exit(vmgenid_exit);
