#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
//#include <asm/types.h>
#include <linux/hiddev.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "crc8.h"

#define close_return(x) do { close(fd); return x; } while(0);

int main(void) {
    int fd, rc;

    if ((fd = open("/dev/usb/hiddev0", O_RDONLY)) <= 0) {
        perror("open");
        return 1;
    }

    struct hiddev_devinfo device_info;

    rc = ioctl(fd, HIDIOCGDEVINFO, &device_info);

    if (rc < 0) {
        perror("ioctl");
        close_return(1);
    }

    printf("VID: 0x%04hX  PID: 0x%04hX  version: 0x%04hX\n", device_info.vendor, device_info.product, device_info.version);
    printf("Applications: %i\n", device_info.num_applications);
    printf("Bus: %d Devnum: %d Ifnum: %d\n", device_info.busnum, device_info.devnum, device_info.ifnum);

    struct hiddev_string_descriptor hstring_man, hstring_prod;

    hstring_man.index = 1;
    hstring_man.value[0] = 0;

    hstring_prod.index = 2;
    hstring_prod.value[0] = 0;

    rc = ioctl(fd, HIDIOCGSTRING, &hstring_man);

    if (rc < 0) {
        perror("ioctl");
        close_return(1);
    }

    rc = ioctl(fd, HIDIOCGSTRING, &hstring_prod);

    if (rc < 0) {
        perror("ioctl");
        close_return(1);
    }

    printf("Manufacturer: %s\nProduct: %s\n", hstring_man.value, hstring_prod.value);
    
    if (strcmp(hstring_man.value, "usb@6bez10.info") != 0 || strcmp(hstring_prod.value, "DS18B20 v1") != 0) {
        fprintf(stderr, "Your device is not supported by this software\n");
        close_return(1);
    }

    size_t n = 9;
    unsigned char report[9];
    unsigned char crc;

    struct hiddev_report_info rep_info_i;
	struct hiddev_usage_ref_multi ref_multi_i;

    rep_info_i.report_type = HID_REPORT_TYPE_FEATURE;
	rep_info_i.report_id = HID_REPORT_ID_FIRST;
	rep_info_i.num_fields = 1;

    ref_multi_i.uref.report_type = HID_REPORT_TYPE_FEATURE;
	ref_multi_i.uref.report_id = HID_REPORT_ID_FIRST;
	ref_multi_i.uref.field_index = 0;
	ref_multi_i.uref.usage_index = 0;
	ref_multi_i.num_values = n;

    size_t i;
    short temp;

    while (1) {
        rc = ioctl(fd, HIDIOCGREPORT, &rep_info_i);

        if (rc < 0) {
            perror("ioctl");
            close_return(1);
        }

        rc = ioctl(fd, HIDIOCGUSAGES, &ref_multi_i);

        if (rc < 0) {
            perror("ioctl");
            close_return(1);
        }

        for (i = 0; i < n; ++i) {
            printf("%02X ", report[i] = ref_multi_i.values[i]);
        }

        crc = crc8(report, 9);

        printf("(CRC %s)\n", (crc == 0) ? "OK" : "ERROR");

        if (crc) {
            close_return(1);
        }

        temp = ((short)report[1] << 8) | report[0];
        printf("Temperature: %.04f*C\n", (float)temp / 16);
        
        sleep(1);
    }

    close(fd);
    return 0;
}
