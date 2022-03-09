#include "common/serial.hpp"
#include "common/serial_lever.hpp"
#include <serial/serial.h>
#include <string>

int main(int, char**) {
  auto ports = om::enumerate_ports();
  for (auto& port : ports) {
    printf("Port: %s\n", port.port.c_str());
  }

  const std::string port = "COM3";
  const uint32_t baud = 9600;
  if (auto context = om::make_context(port, baud, 250)) {
    if (auto state = om::read_state(context.value())) {
      auto& lever = state.value();
      const auto state_str = om::to_string(state.value(), "\n");
      printf("Ok: %s", state_str.c_str());
    } else {
      printf("Failed to read state.\n");
      return 0;
    }
  } else {
    printf("Failed to make context.\n");
    return 0;
  }

  return 0;
}

