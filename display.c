#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "font5x7.h"

#define DEVICE_NAME   "/dev/i2c-1"

#define SLAVE_ADDRESS 0x74

#define ENABLE_ADDRESS 0x00
#define COLOR_ADDRESS  0x24
#define BANK_ADDRESS   0xfd

#define BANK_CONFIG    0x0b

#define CONFIG_MODE       0x00
#define CONFIG_FRAME      0x01
#define CONFIG_AUDIO_SYNC 0x06
#define CONFIG_SHUTDOWN   0x0a

static int phat_fd = -1;
static unsigned char current_frame = 0;

static bool
phat_write_buffer (unsigned char address, unsigned char *data, unsigned int data_length)
{
    unsigned char buffer[data_length + 1];
    buffer[0] = address;
    for (unsigned int i = 0; i < data_length; i++)
        buffer[i + 1] = data[i];
    if (write (phat_fd, buffer, data_length + 1) < 0) {
        fprintf (stderr, "Failed to write to Scroll pHat HD: %s\n", strerror (errno));
        return false;
    }

    return true;
}

static bool
phat_write (unsigned char address, unsigned char value)
{
    return phat_write_buffer (address, &value, 1);
}

static bool
phat_set_bank (unsigned char bank)
{
    return phat_write (BANK_ADDRESS, bank);
}

static bool
phat_write_config (unsigned char address, unsigned char value)
{
    return phat_set_bank (BANK_CONFIG) &&
           phat_write (address, value);
}

static bool
phat_reset ()
{
    if (!phat_write_config (CONFIG_SHUTDOWN, 0))
        return false;

    usleep (10000);

    return phat_write_config (CONFIG_SHUTDOWN, 1);
}

static bool
phat_setup ()
{
    if (!phat_reset () ||
        !phat_write_config (CONFIG_FRAME, 0) ||
        !phat_write_config (CONFIG_MODE, 0x00) || // picture
        !phat_write_config (CONFIG_AUDIO_SYNC, 0))
        return false;

    unsigned char enable_buffer[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                      0xff };
    if (!phat_set_bank (1) ||
        !phat_write_buffer (ENABLE_ADDRESS, enable_buffer, 17))
        return false;
    if (!phat_set_bank (0) ||
        !phat_write_buffer (ENABLE_ADDRESS, enable_buffer, 17))
        return false;

    return true;
}

static bool
phat_write_picture (unsigned char frame, unsigned char *pixels)
{
    unsigned char color_buffer[144];

    for (int i = 0; i < 144; i++)
        color_buffer[i] = 0;
    for (int x = 0; x < 17; x++) {
        for (int y = 0; y < 7; y++) {
            unsigned char p = pixels[y * 17 + x];
            if (x <= 8)
                color_buffer[6 + (8 - x) * 16 - y] = p;
            else
                color_buffer[8 + (x - 9) * 16 + y] = p;
        }
    }

    if (!phat_set_bank (frame) ||
        !phat_write_buffer (COLOR_ADDRESS, color_buffer, 144) ||
        !phat_write_config (CONFIG_FRAME, frame))
        return false;

    return true;
}

static int
draw_string (unsigned char *picture, int x, const char *text)
{
    int offset = 0;

    for (const char *c = text; *c != '\0'; c++) {
        if (c != text)
            offset++;
        offset += draw_letter (picture, x + offset, *c);
    }

    return offset;
}

int
main (int argc, char **argv)
{
    phat_fd = open (DEVICE_NAME, O_RDWR);
    if (phat_fd < 0) {
        fprintf (stderr, "Failed to open I2C device: %s\n", strerror (errno));
        return EXIT_FAILURE;
    }

    if (ioctl (phat_fd, I2C_SLAVE, SLAVE_ADDRESS) < 0) {
        fprintf (stderr, "Failed to set I2C slave address: %s\n", strerror (errno));
        return EXIT_FAILURE;
    }

    if (!phat_setup ())
        return EXIT_FAILURE;

    for (int x = 0; ; x--) {
        unsigned char picture[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        int length = draw_string (picture, x + 17, "Ubuntu Core");
        if (x + 17 + length < 0)
            return EXIT_SUCCESS;         
        if (!phat_write_picture (current_frame, picture))
            return EXIT_FAILURE;
        current_frame ^= 1;

        usleep (100000);
    }

    return EXIT_SUCCESS;
}
