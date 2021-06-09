#ifndef __MOVEMENTSENSOR__
#define __MOVEMENTSENSOR__

class MovementSensor
{
public:
    MovementSensor(int pin);
    bool isMovementDetected();

private:
    int pin;
};

#endif
