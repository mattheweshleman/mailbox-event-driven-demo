/*
 * SensorStatemachine.h
 *
 *  Created on: Apr 24, 2016
 *      Author: Matthew Eshleman
 */

#ifndef SENSORSTATEMACHINE_H_
#define SENSORSTATEMACHINE_H_


enum class SensorEvent: char
{
	ENTER_STATE,  //internal use only
	EXIT_STATE,   //internal use only
	RADIO_CREATED,
	TIMEOUT_10_SEC,
	MSG_SENT,
	MSG_SENT_ERROR,
	ACK_RXD_OK,
	EXCEEDED_RETRIES
};


/**
 * class SensorStatemachine
 *   simple flat statemachine controlling overall
 *   mailbox sensor behavior
 */
class SensorStatemachine {
public:
	SensorStatemachine();
	virtual ~SensorStatemachine();

	void Init();

	void ProcessEvent(const SensorEvent event);

private:

	/*
	 * Check this out... http://www.gotw.ca/gotw/057.htm
	 *   For background on the below.
	 */
	struct SmHandler_;
	typedef SmHandler_ (*Handler)(SensorStatemachine*, const SensorEvent);
	struct SmHandler_
	{
		SmHandler_( Handler pp ) : p( pp ) { }
		operator Handler() { return p; }
		Handler p;
	};

	static SmHandler_ InitRadio(SensorStatemachine* me, const SensorEvent event);
	static SmHandler_ SendMsg(SensorStatemachine* me, const SensorEvent event);
	static SmHandler_ ReceiveAck(SensorStatemachine* me, const SensorEvent event);
	static SmHandler_ DeepSleep(SensorStatemachine* me, const SensorEvent event);

	Handler mCurrent;
};

#endif /* SENSORSTATEMACHINE_H_ */
