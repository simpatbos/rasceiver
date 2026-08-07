#include <iostream>
#include <gpiod.hpp>
#include <chrono>
#include <functional>
#include <thread>

class RotaryEncoder {
  public:
    RotaryEncoder(std::string chip_path, 
        const unsigned int a_line, 
        const unsigned int b_line, 
        const unsigned int c_line, 
        const std::function<void()>&cw_cb, 
        const std::function<void()>&ccw_cb, 
        const std::function<void()>&button_cb
        ): _chip(chip_path), 
          _a_line(a_line), 
          _b_line(b_line), 
          _c_line(c_line), 
          _cw_cb(cw_cb), 
          _ccw_cb(ccw_cb), 
          _button_cb(button_cb) {

    } 

    ~RotaryEncoder() {
      if (_rotation_thread.joinable()) {
        _rotation_thread.join();
      }
      if (_button_thread.joinable()) {
        _button_thread.join();
      }
    }

    void listen_rotation() {
      _rotation_thread = std::thread(&RotaryEncoder::_listen_rotation, this);
    }

    void listen_button() {
      _button_thread = std::thread(&RotaryEncoder::_listen_button, this);
    }

  private:
    std::thread _rotation_thread;
    std::thread _button_thread;

    gpiod::chip _chip; // chip to read from
    const unsigned int _a_line; // a line
    const unsigned int _b_line; // b line
    const unsigned int _c_line; // button line
    const std::function<void()> _cw_cb; // on cw rotation
    const std::function<void()> _ccw_cb; // on ccw rotation
    const std::function<void()> _button_cb; // on button press

    // given [prev_state][cur_state]
    // did it move cw (-1) or ccw (1)
    const int _state_mappings [4][4]= {
      {0, -1, 1, 0}, // prev = 0
      {1, 0, 0, -1}, // prev = 1
      {-1, 0, 0, 1}, // prev = 2
      {0, 1, -1, 0}, // prev = 3
    };
    
    void _listen_rotation() {
      gpiod::line_settings line_settings = gpiod::line_settings();
      line_settings.set_direction(gpiod::line::direction::INPUT);
      line_settings.set_edge_detection(gpiod::line::edge::BOTH);
      line_settings.set_bias(gpiod::line::bias::PULL_UP);
      line_settings.set_active_low(true);

      gpiod::line_config line_config = gpiod::line_config();
      line_config.add_line_settings({_a_line, _b_line}, line_settings);

      auto request = _chip.prepare_request()
        .set_line_config(line_config)
        .set_consumer("rasceiver")
        .do_request();

      gpiod::edge_event_buffer buffer(32);

      int a_state = (request.get_value(_a_line) == gpiod::line::value::ACTIVE) ? 1  : 0;
      int b_state = (request.get_value(_b_line) == gpiod::line::value::ACTIVE) ? 1  : 0;
      int prev_state = (a_state << 1) | b_state;
      int step_count = 0;

      while (true) {
        if (request.wait_edge_events(std::chrono::milliseconds(10))) {
          request.read_edge_events(buffer);

          for (const auto& event : buffer) {
            int state;
            if (event.type() == gpiod::edge_event::event_type::RISING_EDGE) {
              if (event.line_offset() == _a_line) {
                state = prev_state | 0b10;
              }
              if (event.line_offset() == _b_line) {
                state = prev_state | 0b01;
              }
            }
            if (event.type() == gpiod::edge_event::event_type::FALLING_EDGE) {
              if (event.line_offset() == _a_line) {
                state = prev_state & 0b01;
              }  
              if (event.line_offset() == _b_line) {
                state = prev_state & 0b10;
              }  
            }
            step_count += _state_mappings[prev_state][state];
            if (step_count >= 4) {
              _ccw_cb();
              step_count = 0;
            }
            if (step_count <= -4) {
              _cw_cb();
              step_count = 0;
            }
            prev_state = state;
          }
        }
      }
    }

    void _listen_button() {
      gpiod::line_settings line_settings = gpiod::line_settings();
      line_settings.set_direction(gpiod::line::direction::INPUT);
      line_settings.set_edge_detection(gpiod::line::edge::RISING);
      line_settings.set_bias(gpiod::line::bias::PULL_UP);
      line_settings.set_active_low(true);
      line_settings.set_debounce_period(std::chrono::milliseconds(5));

      gpiod::line_config line_config = gpiod::line_config();
      line_config.add_line_settings(_c_line, line_settings);

      auto request = _chip.prepare_request()
        .set_line_config(line_config)
        .set_consumer("rasceiver")
        .do_request();

      gpiod::edge_event_buffer buffer(10);

      while (true) {
        if (request.wait_edge_events(std::chrono::milliseconds(10))) {
          request.read_edge_events(buffer);

          for (const auto& event : buffer) {
            if (event.type() == gpiod::edge_event::event_type::RISING_EDGE) {
              _button_cb();
            }
          }
        }
      }
    }
};