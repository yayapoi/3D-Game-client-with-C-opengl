#include "scene/Component.h"

namespace eng
{
    GameObject* Component::GetOwner()
    {
        return m_owner;
    }
}