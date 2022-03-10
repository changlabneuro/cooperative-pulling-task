#pragma once

#include <serial/serial.h>
#include <memory>
#include <optional>

namespace om {

struct SerialContext {
  std::unique_ptr<serial::Serial> instance{};
};

struct PortDescriptor {
  std::string port;
  std::string description;
};

std::optional<SerialContext> make_context(const std::string& port, uint32_t baud, uint32_t timeout);
std::optional<std::string> readline(const SerialContext& context);
std::vector<PortDescriptor> enumerate_ports();

inline bool is_open(const SerialContext& context) {
  return context.instance && context.instance->isOpen();
}

}