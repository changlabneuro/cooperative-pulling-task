#include "serial.hpp"
#include <cstdio>

namespace om {

std::vector<PortDescriptor> enumerate_ports() {
  std::vector<PortDescriptor> result;
  for (auto& port : serial::list_ports()) {
    PortDescriptor desc{};
    desc.port = port.port;
    desc.description = port.description;
    result.push_back(std::move(desc));
  }
  return result;
}

std::optional<SerialContext> make_context(const std::string& port, uint32_t baud, uint32_t timeout) {
  SerialContext result;

  try {
    result.instance = std::make_unique<serial::Serial>(
      port, baud, serial::Timeout::simpleTimeout(timeout));

  } catch (...) {
    printf("Failed to create instance.\n");
    return std::nullopt;
  }

  if (!result.instance->isOpen()) {
    printf("Failed to open port.\n");
    return std::nullopt;
  } else {
    result.instance->setTimeout(serial::Timeout::max(), timeout, 0, timeout, 0);
  }

  return result;
}

std::optional<std::string> readline(const SerialContext& context) {
  try {
    return context.instance->readline();
  } catch (...) {
    printf("Failed to read line.\n");
    return std::nullopt;
  }
}

}
