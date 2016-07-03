/*
 * RadioManager.h
 *
 *  Created on: May 28, 2016
 *      Author: Matthew Eshleman
 */

#ifndef RADIOMANAGER_H_
#define RADIOMANAGER_H_

namespace RadioManager
{
   typedef void (*RadioEventHandler)(void);

   /**
    *  Init()
    *    Initialize the RadioManager.
    *
    *  @param sensorMsgReceived: function callback
    *         pointer. This function is called
    *         when a fully verified SensorMsg
    *         has been received.
    */
   void Init(RadioEventHandler sensorMsgReceived);
};

#endif /* RADIOMANAGER_H_ */
