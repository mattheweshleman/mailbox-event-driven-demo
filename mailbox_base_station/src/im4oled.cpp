#include "im4oled.h"
#include "mbed.h"

#define BTN_SAMPLES 4

/*
 * Constructor
 */
Im4Oled::Im4Oled(PinName pinOK, PinName pinStar, PinName pinUp, PinName pinDown)
    : btnOK(pinOK, PullUp), btnStar(pinStar, PullUp), btnUp(pinUp, PullUp), btnDown(pinDown, PullUp) {


    // reset all the flags and counters
    memset(&arrButtons, 0, sizeof(arrButtons));
    memset(&arrBtnFalling, 0, sizeof(arrBtnFalling));
    memset(&arrBtnFlags, 0, sizeof(arrBtnFlags));

    // Read pins every 10ms
    _ticker.attach(this, &Im4Oled::_sample, 0.01);
}

void Im4Oled::_sample() {
    uint16_t i;
    uint8_t val[4];

    val[0] = btnOK.read();
    val[1] = btnStar.read();
    val[2] = btnUp.read();
    val[3] = btnDown.read();

    for(i=0; i<4; i++) {
        //If current button pressed
        if(val[i] == 0) {
            if(arrButtons[i] < BTN_SAMPLES) {
                arrButtons[i]++;
            }

            //Button is pressed down
            if(arrButtons[i] == BTN_SAMPLES) {
                if(arrBtnFlags[i].flags.bit.fallingLatch == 0) {
                    arrBtnFlags[i].flags.bit.fallingLatch = 1;
                    arrBtnFalling[i]++;
                }
            }
        }
        else {
            if (arrButtons[i] > 0) {
                arrButtons[i]--;
            }

            //Button is up
            if(arrButtons[i] == 0) {
                //Reset fallingLatch
                arrBtnFlags[i].flags.bit.fallingLatch = 0;
            }
        }
    }
}


/**
 * Return the debounced value OK button
 */
int Im4Oled::getOkBtn(void) {
    //return btnOK.read();
    return arrButtons[0] == BTN_SAMPLES;
}

/**
 * Return the debounced value Star button
 */
int Im4Oled::getStarBtn(void) {
    return arrButtons[1] == BTN_SAMPLES;
}


/**
 * Return the debounced value Up button
 */
int Im4Oled::getUpBtn(void) {
    return arrButtons[2] == BTN_SAMPLES;
}

/**
 * Return the debounced value Down button
 */
int Im4Oled::getDownBtn(void) {
    return arrButtons[3] == BTN_SAMPLES;
}

/**
 * Return number of times the OK button was pressed
 */
int Im4Oled::getBtnFalling(uint16_t btnID) {
    if(arrBtnFalling[btnID]!=0) {
        arrBtnFalling[btnID]--;
        return arrBtnFalling[btnID]+1;
    }
    return 0;
}

/**
 * Return number of times the OK button was pressed
 */
int Im4Oled::getOkBtnFalling(void) {
    return getBtnFalling(0);
}

/**
 * Return number of times the Star button was pressed
 */
int Im4Oled::getStarBtnFalling(void) {
    return getBtnFalling(1);
}

/**
 * Return number of times the Up button was pressed
 */
int Im4Oled::getUpBtnFalling(void) {
    return getBtnFalling(2);
}

/**
 * Return number of times the Down button was pressed
 */
int Im4Oled::getDownBtnFalling(void) {
    return getBtnFalling(3);
}


