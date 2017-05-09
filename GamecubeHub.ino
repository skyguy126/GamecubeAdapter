#include "Joystick.h"

#define GC_PIN 2
#define GC_OUTPUT DDRD |= 0x02;
#define GC_INPUT DDRD &= ~0x02;
#define GC_HIGH PORTD |= 0x02;
#define GC_LOW PORTD &= ~0x02;
#define GC_QUERY (PIND & 0x02)

Joystick_ Joystick(
    JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
    12, 0,
    true, true, false,
    true, true, false,
    false, true,
    false, true, false
);

static int jXMin = 1023;
static int jXMax = 0;
static int jYMin = 1023;
static int jYMax = 0;
static int cXMin = 1023;
static int cXMax = 0;
static int cYMin = 1023;
static int cYMax = 0;
static int jLMin = 1023;
static int jLMax = 0;
static int jRMin = 1023;
static int jRMax = 0;

static unsigned char gcRaw[64] = {};
static struct {
    unsigned int bStart;
    unsigned int bY;
    unsigned int bX;
    unsigned int bB;
    unsigned int bA;
    unsigned int bL;
    unsigned int bR;
    unsigned int bZ;
    unsigned int dUp;
    unsigned int dDown;
    unsigned int dRight;
    unsigned int dLeft;
    int jX;
    int jY;
    int cX;
    int cY;
    int jL;
    int jR;
} gcStatus;

static int btoi(unsigned char *data, int start, int end) {
    int result = 0;
    int counter = end - start;
    for (int i = start; i < end + 1; i++) {
        result |= (gcRaw[i] << counter);
        counter--;
    }
    return result;
}

static void joystickSend() {
    Joystick.setButton(0, gcStatus.bStart);
    Joystick.setButton(1, gcStatus.bY);
    Joystick.setButton(2, gcStatus.bX);
    Joystick.setButton(3, gcStatus.bB);
    Joystick.setButton(4, gcStatus.bA);
    Joystick.setButton(5, gcStatus.bZ);
    Joystick.setButton(6, gcStatus.bL);
    Joystick.setButton(7, gcStatus.bR);
    Joystick.setButton(8, gcStatus.dUp);
    Joystick.setButton(9, gcStatus.dRight);
    Joystick.setButton(10, gcStatus.dDown);
    Joystick.setButton(11, gcStatus.dLeft);
    Joystick.setXAxis(gcStatus.jX);
    Joystick.setYAxis(jYMax - gcStatus.jY + jYMin);
    Joystick.setRxAxis(gcStatus.cX);
    Joystick.setRyAxis(gcStatus.cY);
    Joystick.setBrake(gcStatus.jL);
    Joystick.setThrottle(jRMax - gcStatus.jR + jRMin);
    Joystick.sendState();
}

static void gcInit() {
    unsigned char initialize = 0x00;
    gcSend(&initialize, 1);

    for (int x = 0; x < 64; x++) {
        if (!GC_QUERY)
            x = 0;
    }
}

static void gcConvert() {
    gcStatus.bStart = gcRaw[3];
    gcStatus.bY = gcRaw[4];
    gcStatus.bX = gcRaw[5];
    gcStatus.bB = gcRaw[6];
    gcStatus.bA = gcRaw[7];
    gcStatus.bL = gcRaw[9];
    gcStatus.bR = gcRaw[10];
    gcStatus.bZ = gcRaw[11];
    gcStatus.dUp = gcRaw[12];
    gcStatus.dDown = gcRaw[13];
    gcStatus.dRight = gcRaw[14];
    gcStatus.dLeft = gcRaw[15];
    gcStatus.jX = btoi(gcRaw, 16, 23);
    gcStatus.jY = btoi(gcRaw, 24, 31);
    gcStatus.cX = btoi(gcRaw, 32, 39);
    gcStatus.cY = btoi(gcRaw, 40, 47);
    gcStatus.jL = btoi(gcRaw, 48, 55);
    gcStatus.jR = btoi(gcRaw, 56, 63);
}

static int gcGet() {
    // need to read 8 bytes = 64 bits

    int bitCounter = 0;
    int timeout = 0;

    // wait for line to go low
    lowLoop : {
        if (GC_QUERY) {
            timeout++;
            if (timeout >= 64)
                return 0;

            goto lowLoop;
        } else {
            // line has gone low wait 2 us and sample
            asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                          "nop\nnop\nnop\nnop\nnop\n"
                          "nop\nnop\nnop\nnop\nnop\n"
                          "nop\nnop\nnop\nnop\nnop\n"
                          "nop\nnop\nnop\nnop\nnop\n"
                          "nop\nnop\nnop\nnop\nnop\n");
            goto analyze;
        }
    }

    analyze : {
        gcRaw[bitCounter] = (GC_QUERY >> 1);
        bitCounter++;

        if (bitCounter == 64)
            return 1;

        // wait for line to go high again before repeating
        timeout = 0;
        while (!GC_QUERY) {
            timeout++;
            if (timeout >= 64)
                return 0;
        }

        timeout = 0;
        goto lowLoop;
    }
}

static void gcSend(unsigned char *buffer, char length) {

    char bits;

    outer_loop: {

        bits = 8;

            inner_loop: {

                GC_LOW;

                if (*buffer >> 7) {
                    // bit is a 1, remain low 1us and high 3us
                    asm volatile ("nop\nnop\nnop\nnop\nnop\n");

                    GC_HIGH;

                    // only 2us to allow time for statements below
                    asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                                  "nop\nnop\nnop\nnop\nnop\n"
                                  "nop\nnop\nnop\nnop\nnop\n"
                                  "nop\nnop\nnop\nnop\nnop\n"
                                  "nop\nnop\nnop\nnop\nnop\n"
                                  "nop\nnop\nnop\nnop\nnop\n");

                } else {
                    // bit is a 0, remain low 3us and high 1us
                    asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                                  "nop\nnop\nnop\nnop\nnop\n"
                                  "nop\nnop\nnop\nnop\nnop\n"
                                  "nop\nnop\nnop\nnop\nnop\n"
                                  "nop\nnop\nnop\nnop\nnop\n"
                                  "nop\nnop\nnop\nnop\nnop\n"
                                  "nop\nnop\nnop\nnop\nnop\n"
                                  "nop\n");
                    GC_HIGH;
                }

                // line must return to low in 16 cycles

                --bits;
                if (bits != 0) {

                    // more delay to get exactly 16 cycles
                    asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                                  "nop\nnop\nnop\nnop\n");

                    // shift binary number to the left by 1 (to check next bit)
                    *buffer <<= 1;
                    goto inner_loop;
                }
            }

        // END INNER LOOP
        // Exactly 16 cycles are taken up here

        --length;
        if (length != 0) {
            ++buffer;
            goto outer_loop;
        }
    }

    // send stop bit (1)
    asm volatile ("nop\nnop\nnop\nnop\n");
    GC_LOW;

    asm volatile ("nop\nnop\nnop\nnop\nnop\n"
                  "nop\nnop\nnop\nnop\nnop\n"
                  "nop\n");
    GC_HIGH;
}

static void gcCalibrate() {

    for (int i = 0; i < 5000; i++) {

        unsigned char command[] = {0x40, 0x03, 0x01};

        noInterrupts();
        GC_OUTPUT;
        gcSend(command, 3);
        GC_INPUT;
        int status = gcGet();
        interrupts();

        if (!status) {
            Serial.println("failed callibration");
            continue;
        }

        gcConvert();

        jXMin = (gcStatus.jX < jXMin) ? gcStatus.jX : jXMin;
        jXMax = (gcStatus.jX > jXMax) ? gcStatus.jX : jXMax;

        jYMin = (gcStatus.jY < jYMin) ? gcStatus.jY : jYMin;
        jYMax = (gcStatus.jY > jYMax) ? gcStatus.jY : jYMax;

        cXMin = (gcStatus.cX < cXMin) ? gcStatus.cX : cXMin;
        cXMax = (gcStatus.cX > cXMax) ? gcStatus.cX : cXMax;

        cYMin = (gcStatus.cY < cYMin) ? gcStatus.cY : cYMin;
        cYMax = (gcStatus.cY > cYMax) ? gcStatus.cY : cYMax;

        jLMin = (gcStatus.jL < jLMin) ? gcStatus.jL : jLMin;
        jLMax = (gcStatus.jL > jLMax) ? gcStatus.jL : jLMax;

        jRMin = (gcStatus.jR < jRMin) ? gcStatus.jR : jRMin;
        jRMax = (gcStatus.jR > jRMax) ? gcStatus.jR : jRMax;

        delay(1);
    }

    Joystick.setXAxisRange(jXMin, jXMax);
    Joystick.setYAxisRange(jYMin, jYMax);

    Joystick.setRxAxisRange(cXMin, cXMax);
    Joystick.setRyAxisRange(cYMin, cYMax);

    Joystick.setThrottleRange(jRMin, jRMax);
    Joystick.setBrakeRange(jLMin, jLMax);
}

void setup() {
    Serial.begin(9600);
    Joystick.begin(false); // do not auto update state

    GC_HIGH;
    GC_OUTPUT;
    noInterrupts();
    gcInit();
    interrupts();
}

void loop() {
    unsigned char command[] = {0x40, 0x03, 0x00};

    noInterrupts();
    GC_OUTPUT;
    gcSend(command, 3);
    GC_INPUT;
    int status = gcGet(); // return 1 if success or 0 if timeout
    interrupts();

    if (!status) {
        Serial.println("failed");
        return;
    }

    gcConvert();

    if (gcStatus.jR && gcStatus.bZ && gcStatus.bStart && gcStatus.dDown)
        gcCalibrate();

    joystickSend();
    delay(1);
}
