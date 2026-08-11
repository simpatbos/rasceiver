#include <iostream>
#include <gpiod.hpp>
#include <chrono>
#include <functional>
#include <thread>

#define PIN1 (23)
#define PIN2 (24)

int main() {
	gpiod::chip chip = gpiod::chip("/dev/gpiochip0");

	gpiod::line_settings line_settings = gpiod::line_settings();
  line_settings.set_direction(gpiod::line::direction::OUTPUT);
	line_settings.set_drive(gpiod::line::drive::PUSH_PULL);

  gpiod::line_config line_config = gpiod::line_config();
  line_config.add_line_settings({PIN1,PIN2}, line_settings);

  auto request = chip.prepare_request()
    .set_line_config(line_config)
    .set_consumer("rasceiver")
    .do_request();

  request.set_value(PIN1, gpiod::line::value::INACTIVE);
  request.set_value(PIN2, gpiod::line::value::INACTIVE);
	while (true) {
		request.set_value(PIN1, gpiod::line::value::ACTIVE);
		std::cout << "1 ACTIVE" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		request.set_value(PIN1, gpiod::line::value::INACTIVE);
		std::cout << "1 INACTIVE" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(2));

		request.set_value(PIN2, gpiod::line::value::ACTIVE);
		std::cout << "2 ACTIVE" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		request.set_value(PIN2, gpiod::line::value::INACTIVE);
		std::cout << "2 INACTIVE" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(2));
  }

}
