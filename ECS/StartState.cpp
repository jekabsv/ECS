#include "StartState.h"
#include "RenderComponent.h"

void StartState::Init()
{
    Mesh triangleMesh = {
        {400, 100, 1.0f, 0.0f, 0.0f, 1.0f},
        {200, 500, 0.0f, 1.0f, 0.0f, 1.0f},
        {600, 500, 0.0f, 0.0f, 1.0f, 1.0f} 
    };

    _data->assets.AddMesh("triangle", triangleMesh);

    Mesh squareMesh = {
        {200, 100, 1.0f, 0.0f, 0.0f, 1.0f}, 
        {200, 300, 0.0f, 1.0f, 0.0f, 1.0f}, 
        {400, 100, 0.0f, 0.0f, 1.0f, 1.0f}, 

        {200, 300, 0.0f, 1.0f, 0.0f, 1.0f}, 
        {400, 300, 1.0f, 1.0f, 0.0f, 1.0f}, 
        {400, 100, 0.0f, 0.0f, 1.0f, 1.0f}  
    };

    _data->assets.AddMesh("square", squareMesh);




    ecs::Entity e = ecs.create_entity();
    ecs.add_component(e, RenderComponent(1, "square"));



}

void StartState::Render()
{

    if (!_data->renderer)
    {
        std::cout << SDL_GetError() << '\n';
        return;
    }

    SDL_RenderClear(_data->renderer);


    ecs.for_each(ecs.CreateMask<RenderComponent>(), [this](ecs::Entity e, std::function<void* (ecs::TypeId)> getComponent)
        {
            RenderComponent* rc = reinterpret_cast<RenderComponent*>(getComponent(ecs::getTypeId<RenderComponent>()));
            if (rc->render)
            {
                Mesh mesh = _data->assets.GetMesh(rc->mesh);
                SDL_RenderGeometry(_data->renderer, nullptr,mesh.data(), mesh.size(), nullptr, 0);
            }
        });

    SDL_RenderPresent(_data->renderer);
    return;
}