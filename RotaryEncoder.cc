#include <stdio.h>
#include <gpiod.h>

int main() {
	struct gpiod_chip *gpio = gpiod_chip_open("/dev/gpiochip0");
	if (!gpio) {
		perror("error opening chip");
		return 1;
	}
	struct gpiod_chip_info *info = gpiod_chip_get_info(gpio);	
	const char *name = gpiod_chip_info_get_label(info);
	printf(name);
	return 0;
}
