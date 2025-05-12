#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>

#define DEVICE_NAME "/dev/tcs34725"
#define TCS34725_IOCTL_READ_C   _IOR('t', 1, uint16_t)
#define TCS34725_IOCTL_READ_R   _IOR('t', 2, uint16_t)
#define TCS34725_IOCTL_READ_G   _IOR('t', 3, uint16_t)
#define TCS34725_IOCTL_READ_B   _IOR('t', 4, uint16_t)
#define TCS34725_IOCTL_SET_GAIN _IOW('t', 5, uint8_t)
#define TCS34725_IOCTL_SET_ATIME _IOW('t', 6, uint8_t)

int main(void) {
    int fd = open(DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        perror("open");
        return errno;
    }

    uint8_t gain = 0x03, atime = 0xD5;
    if (ioctl(fd, TCS34725_IOCTL_SET_GAIN, &gain) < 0) perror("set gain");
    if (ioctl(fd, TCS34725_IOCTL_SET_ATIME, &atime) < 0) perror("set atime");

    while (1) {
        uint16_t c, r, g, b;
        if (ioctl(fd, TCS34725_IOCTL_READ_C, &c) < 0) perror("read C");
        if (ioctl(fd, TCS34725_IOCTL_READ_R, &r) < 0) perror("read R");
        if (ioctl(fd, TCS34725_IOCTL_READ_G, &g) < 0) perror("read G");
        if (ioctl(fd, TCS34725_IOCTL_READ_B, &b) < 0) perror("read B");

        if (c == 0) {
            printf("C=0, skipping normalization to avoid division by zero.\n");
        } else {
            uint8_t norm_r = (r * 255) / c;
            uint8_t norm_g = (g * 255) / c;
            uint8_t norm_b = (b * 255) / c;

            // Clamp values to 0–255
            if (norm_r > 255) norm_r = 255;
            if (norm_g > 255) norm_g = 255;
            if (norm_b > 255) norm_b = 255;

            printf("Normalized RGB: R=%u G=%u B=%u\n", norm_r, norm_g, norm_b);
        }

        usleep(250000);
    }

    close(fd);
    return 0;
}
