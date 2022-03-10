#pragma once

#include "ringbuffer.hpp"
#include "handshake.hpp"
#include "serial_lever.hpp"
#include "identifier.hpp"
#include <thread>
#include <vector>

namespace om::lever {

struct SerialLeverHandle {
  OM_INTEGER_IDENTIFIER_EQUALITY(SerialLeverHandle, id)
  uint32_t id;
};

enum class SerialLeverError {
  None = 0,
  FailedToOpen = 1,
};

enum class LeverMessageType {
  SetForce = 0,
  ShareState,
  OpenPort,
  ClosePort,
  PortStatus,
};

struct LeverMessageData {
  SerialLeverHandle handle;
  LeverMessageType type;
  std::optional<LeverState> state;
  std::optional<int> force;
  std::string port;
  bool is_open;
  SerialLeverError error;
};

struct LeverSystem {
  struct RemoteInstance {
    SerialContext serial_context;
    std::optional<LeverState> state;
    std::optional<int> force;
    int commanded_force{};
    bool need_send_state{};
    std::optional<SerialLeverError> open_response;
  };

  struct LocalInstance {
    SerialLeverHandle handle;
    std::optional<int> pending_canonical_force;
    std::optional<std::string> pending_open_port;
    bool pending_close_port{};
    int commanded_force{};
    std::optional<int> canonical_force;
    std::optional<LeverState> state;
    Handshake<LeverMessageData> message;

    bool awaiting_open{};
    bool is_open{};
  };

  std::thread worker_thread;
  std::atomic<bool> keep_processing{};

  std::vector<std::unique_ptr<LocalInstance>> local_instances;
  std::vector<std::unique_ptr<RemoteInstance>> remote_instances;
  RingBuffer<LeverMessageData, 8> read_remote;

  uint32_t instance_id{1};
};

std::vector<SerialLeverHandle> initialize(LeverSystem* sys, int max_num_levers);
void update(LeverSystem* system);
void terminate(LeverSystem* sys);

inline int num_remote_commands(LeverSystem* sys) {
  return sys->read_remote.size();
}

void set_force(LeverSystem* system, SerialLeverHandle instance, int grams);
void open_connection(LeverSystem* system, SerialLeverHandle handle, const std::string& port);
bool is_pending_open(LeverSystem* system, SerialLeverHandle handle);
bool is_open(LeverSystem* system, SerialLeverHandle handle);
void close_connection(LeverSystem* system, SerialLeverHandle handle);

std::optional<int> get_canonical_force(LeverSystem* system, SerialLeverHandle instance);
int get_commanded_force(LeverSystem* system, SerialLeverHandle instance);
std::optional<LeverState> get_state(LeverSystem* system, SerialLeverHandle instance);

}