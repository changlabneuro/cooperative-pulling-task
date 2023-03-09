#pragma once

#include "serial_lever.hpp"
#include "identifier.hpp"
#include <vector>

namespace om::lever {

struct SerialLeverHandle {
  OM_INTEGER_IDENTIFIER_EQUALITY(SerialLeverHandle, id)
  uint32_t id;
};

struct LeverSystem;

void initialize(LeverSystem* sys, int max_num_levers, SerialLeverHandle* levers);
void update(LeverSystem* system);
void terminate(LeverSystem* sys);

int num_remote_commands(LeverSystem* sys);
LeverSystem* get_global_lever_system();

void set_force(LeverSystem* system, SerialLeverHandle instance, int grams);
void set_direction(LeverSystem* system, SerialLeverHandle instance, SerialLeverDirection dir);
void open_connection(LeverSystem* system, SerialLeverHandle handle, const std::string& port);
bool is_pending_open(LeverSystem* system, SerialLeverHandle handle);
bool is_open(LeverSystem* system, SerialLeverHandle handle);
void close_connection(LeverSystem* system, SerialLeverHandle handle);

std::optional<int> get_canonical_force(LeverSystem* system, SerialLeverHandle instance);
int get_commanded_force(LeverSystem* system, SerialLeverHandle instance);
std::optional<LeverState> get_state(LeverSystem* system, SerialLeverHandle instance);
std::optional<SerialLeverDirection> get_canonical_direction(LeverSystem* system, SerialLeverHandle instance);
SerialLeverDirection get_commanded_direction(LeverSystem* system, SerialLeverHandle instance);

}