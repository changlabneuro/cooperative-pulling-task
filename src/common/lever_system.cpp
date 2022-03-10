#include "lever_system.hpp"
#include <cassert>

namespace om {

namespace {

using namespace lever;

LeverSystem::LocalInstance* find_local_instance(LeverSystem* sys, SerialLeverHandle handle) {
  for (auto& inst : sys->local_instances) {
    if (inst->handle == handle) {
      return inst.get();
    }
  }
  return nullptr;
}

LeverCommandData make_set_force_command(int force) {
  LeverCommandData result{};
  result.type = lever::LeverCommandType::SetForce;
  result.force = force;
  return result;
}

LeverCommandData make_open_port_command(std::string&& port) {
  LeverCommandData result{};
  result.type = lever::LeverCommandType::OpenPort;
  result.port = std::move(port);
  return result;
}

LeverCommandData make_close_port_command() {
  LeverCommandData result{};
  result.type = lever::LeverCommandType::ClosePort;
  return result;
}

bool process_remote_command(LeverSystem::RemoteInstance& remote, LeverCommandData&& data) {
  switch (data.type) {
    case LeverCommandType::SetForce: {
      remote.commanded_force = data.force.value();
      break;
    }
    case LeverCommandType::OpenPort: {
      remote = {};
      auto serial_res = om::make_context(
        data.port, om::default_baud_rate(), om::default_read_write_timeout());
      if (serial_res) {
        remote.serial_context = std::move(serial_res.value());
      } else {
        //  write error.
      }
      return true;
    }
    case LeverCommandType::ClosePort: {
      remote = {};
      return true;
    }
    default: {
      assert(false);
    }
  }
  return false;
}

void process_instance(LeverSystem* system, LeverSystem::RemoteInstance& remote,
                      LeverSystem::LocalInstance& local) {
  if (auto data = read(&local.command)) {
    if (process_remote_command(remote, std::move(data.value()))) {
      remote.need_send_update = true;
    }
  }

  if (is_open(remote.serial_context)) {
    remote.need_send_update = true;

    if (auto resp = om::set_force_grams(remote.serial_context, remote.commanded_force)) {
      remote.force = remote.commanded_force;
    } else {
      remote.force = std::nullopt;
    }

    if (auto state = om::read_state(remote.serial_context)) {
      remote.state = state.value();
    } else {
      remote.state = std::nullopt;
    }
  }

  if (remote.need_send_update) {
    LeverCommandData state{};
    state.type = LeverCommandType::ShareState;
    state.force = remote.force;
    state.state = remote.state;
    state.handle = local.handle;
    if (system->read_remote.maybe_write(state)) {
      remote.need_send_update = false;
    }
  }
}

void worker(LeverSystem* system) {
  while (system->keep_processing.load()) {
    for (int i = 0; i < int(system->remote_instances.size()); i++) {
      auto& remote = *system->remote_instances[i];
      auto& local = *system->local_instances[i];
      process_instance(system, remote, local);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

std::unique_ptr<LeverSystem::LocalInstance> make_local_instance(SerialLeverHandle handle) {
  auto result = std::make_unique<LeverSystem::LocalInstance>();
  result->handle = handle;
  return result;
}

std::unique_ptr<LeverSystem::RemoteInstance> make_remote_instance() {
  return std::make_unique<LeverSystem::RemoteInstance>();
}

} //  anon

std::vector<SerialLeverHandle> lever::initialize(LeverSystem* sys, int max_num_levers) {
  std::vector<SerialLeverHandle> result;

  for (int i = 0; i < max_num_levers; i++) {
    SerialLeverHandle handle{sys->instance_id++};
    sys->local_instances.emplace_back() = make_local_instance(handle);
    sys->remote_instances.emplace_back() = make_remote_instance();
    result.push_back(handle);
  }

  sys->keep_processing.store(true);
  sys->worker_thread = std::thread{[sys]() {
    worker(sys);
  }};

  return result;
}

void lever::terminate(LeverSystem* sys) {
  sys->keep_processing.store(false);
  if (sys->worker_thread.joinable()) {
    sys->worker_thread.join();
  }
  sys->local_instances.clear();
  sys->remote_instances.clear();
}

void lever::update(LeverSystem* system) {
  for (auto& inst : system->local_instances) {
    if (inst->command.awaiting_read) {
      (void) acknowledged(&inst->command);
    }

    if (inst->pending_open_port && !inst->command.awaiting_read) {
      auto data = make_open_port_command(std::move(inst->pending_open_port.value()));
      publish(&inst->command, std::move(data));
      inst->pending_open_port = std::nullopt;
    }

    if (inst->pending_close_port && !inst->command.awaiting_read) {
      publish(&inst->command, make_close_port_command());
      inst->pending_close_port = false;
    }

    if (inst->pending_canonical_force && !inst->command.awaiting_read) {
      auto data = make_set_force_command(inst->pending_canonical_force.value());
      publish(&inst->command, std::move(data));
      inst->pending_canonical_force = std::nullopt;
    }
  }

  const int num_read = system->read_remote.size();
  for (int i = 0; i < num_read; i++) {
    auto response = system->read_remote.read();
    if (response.type == LeverCommandType::ShareState) {
      if (auto* inst = find_local_instance(system, response.handle)) {
        inst->canonical_force = response.force;
        inst->state = response.state;
      }
    }
  }
}

void lever::set_force(LeverSystem* system, SerialLeverHandle instance, int grams) {
  if (auto* inst = find_local_instance(system, instance)) {
    inst->pending_canonical_force = grams;
    inst->commanded_force = grams;
  } else {
    assert(false);
  }
}

void lever::open_connection(LeverSystem* system, SerialLeverHandle handle,
                            const std::string& port) {
  if (auto* inst = find_local_instance(system, handle)) {
    inst->pending_open_port = port;
  } else {
    assert(false);
  }
}

void lever::close_connection(LeverSystem* system, SerialLeverHandle handle) {
  if (auto* inst = find_local_instance(system, handle)) {
    inst->pending_close_port = true;
  } else {
    assert(false);
  }
}

int lever::get_commanded_force(LeverSystem* system, SerialLeverHandle instance) {
  if (auto* inst = find_local_instance(system, instance)) {
    return inst->commanded_force;
  } else {
    assert(false);
    return 0;
  }
}

std::optional<int> lever::get_canonical_force(LeverSystem* system, SerialLeverHandle instance) {
  if (auto* inst = find_local_instance(system, instance)) {
    return inst->canonical_force;
  } else {
    assert(false);
    return std::nullopt;
  }
}

std::optional<LeverState> lever::get_state(LeverSystem* system, SerialLeverHandle instance) {
  if (auto* inst = find_local_instance(system, instance)) {
    return inst->state;
  } else {
    assert(false);
    return std::nullopt;
  }
}

}
