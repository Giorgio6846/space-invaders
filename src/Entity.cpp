#include "Entity.h"
#include "defs.h"

int Entity::getPositionX()
{
    return *positionX;
}

int Entity::getPositionY()
{
    return *positionY;
}

bool Entity::setPositionX(int positionX)
{
    if(positionX >= 0 && positionX <= SCREEN_WIDTH)
    {
        *this->positionX = positionX;
        return 1;
    }
    return 0;
}

bool Entity::setPositionY(int positionY)
{
    if (positionY >= 0 && positionY <= SCREEN_HEIGHT)
    {
        *this->positionY = positionY;
        return 1;
    }
    return 0;
}

