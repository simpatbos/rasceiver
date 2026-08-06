#include <iostream>
#include <gpiod.hpp>
#include <thread>
#include <chrono>

#define A_LINE 24
#define B_LINE 23

int main() {
	gpiod::chip chip = gpiod::chip("/dev/gpiochip0");

  gpiod::line_settings line_settings = gpiod::line_settings();
	line_settings.set_direction(gpiod::line::direction::INPUT);
	line_settings.set_edge_detection(gpiod::line::edge::BOTH);
	line_settings.set_bias(gpiod::line::bias::PULL_UP);
	line_settings.set_active_low(false);
	line_settings.set_debounce_period(std::chrono::milliseconds(5));

	gpiod::line_config line_config = gpiod::line_config();
	line_config.add_line_settings({A_LINE, B_LINE}, line_settings);

	auto request = chip.prepare_request()
		.set_line_config(line_config)
		.set_consumer("rasceiver")
		.do_request();

	gpiod::edge_event_buffer buffer(10);

	std::cout << "Waiting..." << std::endl;

	bool aState = 0;
	while (true) {
		if (request.wait_edge_events(std::chrono::seconds(10))) {
				request.read_edge_events(buffer);

				for (const auto& event : buffer) {
					if (event.type() == gpiod::edge_event::event_type::RISING_EDGE) {
						if (event.line_offset() == A_LINE) {
							aState = 1;
					  }
						if (event.line_offset() == B_LINE) {
							if (aState) std::cout << "B ROT 1" << std::endl;
							if (!aState) std::cout << "B ROT 2" << std::endl;
					  }
				  }
					if (event.type() == gpiod::edge_event::event_type::FALLING_EDGE) {
            if (event.line_offset() == A_LINE) {
							aState = 0;
					  }
				  } 
			  }
		}
		else {
			std::cout << "rotarty encoder still listening..." << std::endl;	
		}
  }
	return 0;
}
