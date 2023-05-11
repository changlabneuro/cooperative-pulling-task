#include "led.hpp"
#include "ni.hpp"
#include <cassert>

namespace {

void do_trigger(om::led::LEDSync* sync, om::TimePoint curr_t) {
	om::ni::write_analog_pulse(sync->ni_channel_index, sync->pulse_high_voltage, sync->pulse_duration_s);
	sync->sync_time_points.push_back(om::elapsed_time(sync->t0, curr_t));
}

}	//	anon

void om::led::initialize(LEDSync* sync, om::TimePoint t0, int channel_index) {
	assert(!sync->initialized);
	sync->t0 = t0;
	sync->initialized = true;
	sync->ni_channel_index = channel_index;
}

void om::led::trigger(LEDSync* sync) {
	assert(sync->initialized);
	do_trigger(sync, now());
}

void om::led::update(LEDSync* sync) {
	assert(sync->initialized);
	auto curr_t = now();
	if (!sync->has_last_sync_time || 
			elapsed_time(sync->last_sync_time, curr_t) >= sync->sync_interval_s) {
		sync->has_last_sync_time = true;
		sync->last_sync_time = curr_t;
		do_trigger(sync, curr_t);
	}
}