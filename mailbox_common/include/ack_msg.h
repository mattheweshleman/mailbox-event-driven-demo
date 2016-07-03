
#ifndef ACK_MSG_H_
#define ACK_MSG_H_

#include <stdint.h>
#include <type_traits>

/*
 * The AckMsg that the base station
 * sends to the Sensor to Ack the
 * SensorMsg.
 */
struct AckMsg {

   AckMsg() :
      hdr_id{'E','S','H','L'},
      msg_id(2)
   {

   }

   const int8_t hdr_id[4];
   const uint8_t msg_id;
} __attribute__((packed));

/**
 * Confirm that the compiler
 * packed the message as expected.
 */
static_assert( sizeof(AckMsg) == 5, "should have been 5");

#endif
