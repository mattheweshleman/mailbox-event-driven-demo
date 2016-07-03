
#ifndef SENSOR_MSG_H_
#define SENSOR_MSG_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <type_traits>

/**
 * SensorMsg: data structure
 *  representing the message
 *  sent by the mail box sensor
 *  to the base station.
 */
struct SensorMsg {

	SensorMsg() :
		hdr_id{'E','S','H','L'},
		msg_id(1),
		battery_mV(0),
		is_low(false)
	{
	}

	const int8_t hdr_id[4];
	const uint8_t msg_id;
	uint16_t battery_mV;
	bool is_low;
} __attribute__((packed));

/**
 * Various static asserts to confirm
 * the compiler packs the message as expected.
 */
static_assert( sizeof(bool) == 1, "bool is larger than a single byte!" );
static_assert( sizeof(SensorMsg) == 8, "should have been 8");

#endif
