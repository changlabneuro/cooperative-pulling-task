#pragma once

#include "identifier.hpp"
#include <string>

namespace om::pump {

enum class RateUnits {
  mLPerHour
};

enum class VolumeUnits {
  mL,
};

struct PumpState {
  bool connection_open;
  int address;
  float syringe_diameter;
  int rate;
  pump::RateUnits rate_units;
  float volume;
  pump::VolumeUnits volume_units;
};

struct PumpHandle {
  OM_INTEGER_IDENTIFIER_EQUALITY(PumpHandle, index)
  uint32_t index;
};

void initialize_pump_system(std::string port, int num_pumps);
void terminate_pump_system();

int num_initialized_pumps();
pump::PumpHandle ith_pump(int i);

PumpState read_desired_pump_state(PumpHandle pump);
PumpState read_canonical_pump_state(PumpHandle pump);

void set_dispensed_volume(PumpHandle pump, float vol, VolumeUnits units);
void set_pump_rate(PumpHandle pump, int rate, RateUnits units);
void set_address(PumpHandle pump, int address);
void set_address_rate_volume(PumpHandle pump, PumpState state);
void run_dispense_program(PumpHandle pump);
void stop_dispense_program(PumpHandle pump);

void submit_commands();

}