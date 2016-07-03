/*
 * main_queue.h
 *
 *  Created on: Apr 28, 2016
 *      Author: Matthew
 */

#ifndef MAIN_QUEUE_H_
#define MAIN_QUEUE_H_

#include "SensorStatemachine.h"

/**
 * InjectEvent()
 *   Inject a SensorEvent into the main
 *   event queue. This method
 *   is for normal thread context usage.
 */
void InjectEvent(SensorEvent event);

#endif /* MAIN_QUEUE_H_ */
