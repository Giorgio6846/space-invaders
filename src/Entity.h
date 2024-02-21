#pragma once

class Entity
{
private:
    int * positionX;
    int * positionY;
    
public:
    Entity();
    ~Entity();

    int getPositionX();
    int getPositionY();

    bool setPositionX(int positionX);
    bool setPositionY(int positionY);
};

Entity::Entity()
{
    positionX = new int;
    positionY = new int;
}

Entity::~Entity()
{
}
