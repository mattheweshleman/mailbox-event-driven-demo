#include "mbed.h"

typedef struct {
    union {
        struct {
            uint8_t fallingLatch    :1;
            uint8_t risingLatch     :1;
        } bit;
        uint8_t    Val;
    } flags;
} ButtonFlags;


class Im4Oled {
public:
    Im4Oled(PinName pinOK, PinName pinStar, PinName pinUp, PinName pinDown);

    int getOkBtn();
    int getStarBtn();
    int getUpBtn();
    int getDownBtn();

//    int getOkBtnRissing();
//    int getStarBtnRissing();
//    int getUpBtnRissing();
//    int getDownBtnRissing();

    int getBtnFalling(uint16_t btnID);

    int getOkBtnFalling();
    int getStarBtnFalling();
    int getUpBtnFalling();
    int getDownBtnFalling();

private :
    // objects
    Ticker      _ticker;

    DigitalIn   btnOK;
    DigitalIn   btnStar;
    DigitalIn   btnUp;
    DigitalIn   btnDown;

    uint8_t     arrButtons[4];      //OK, Star, Up, Down
    uint8_t     arrBtnFalling[4];   //OK, Star, Up, Down
    ButtonFlags arrBtnFlags[4];

    // function to take a sample, and update flags
    void _sample(void);
};
