#include "scene/Scene.h"

namespace eng
{
    void Scene::Update(float deltaTime)
    {
        for (auto it = m_objects.begin(); it != m_objects.end();)
        {
            if ((*it)->IsAlive())
            {
                (*it)->Update(deltaTime);
                ++it;
            }
            else
            {
                it = m_objects.erase(it);
            }
        }
    }

    void Scene::Clear()
    {
        m_objects.clear();
    }

    GameObject* Scene::CreateObject(const std::string& name, GameObject* parent)
    {
        auto obj = new GameObject();
        obj->SetName(name);
        SetParent(obj, parent);
        return obj;
    }

    void Scene::SetParent(GameObject* obj, GameObject* parent)
    {

    }
}