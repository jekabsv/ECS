#include "ECS.h"
#include <iostream>
#include "Engine.h"

int main()
{
    Engine engine;
    if (engine.Initialize() != 0)
        return 1;
    engine.run();
    return 0;
}