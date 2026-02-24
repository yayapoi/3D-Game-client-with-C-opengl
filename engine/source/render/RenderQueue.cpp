#include "render/RenderQueue.h"
#include "render/Mesh.h"
#include "render/Material.h"
#include "graphics/GraphicsAPI.h"
#include "graphics/ShaderProgram.h"

namespace eng
{
    void RenderQueue::Init()
    {
        m_mesh2D = Mesh::CreatePlane();
    }

    void RenderQueue::Submit(const RenderCommand& command)
    {
        m_commands.push_back(command);
    }

    void RenderQueue::Submit(const RenderCommand2D& command)
    {
        m_commands2D.push_back(command);
    }

    void RenderQueue::Draw(GraphicsAPI& graphicsAPI, const CameraData& cameraData, const std::vector<LightData>& lights)
    {
        // 3D
        for (auto& command : m_commands)
        {
            graphicsAPI.BindMaterial(command.material);
            auto shaderProgram = command.material->GetShaderProgram();
            shaderProgram->SetUniform("uModel", command.modelMatrix);
            shaderProgram->SetUniform("uView", cameraData.viewMatrix);
            shaderProgram->SetUniform("uProjection", cameraData.projectionMatrix);
            shaderProgram->SetUniform("uCameraPos", cameraData.position);
            if (!lights.empty())
            {
                auto& light = lights[0];
                shaderProgram->SetUniform("uLight.color", light.color);
                shaderProgram->SetUniform("uLight.direction", glm::normalize(-light.position));
            }

            graphicsAPI.BindMesh(command.mesh);
            graphicsAPI.DrawMesh(command.mesh);
            graphicsAPI.UnbindMesh(command.mesh);
        }

        m_commands.clear();

        // 2D
        m_mesh2D->Bind();
        for (auto& command : m_commands2D)
        {
            // rendering
            m_mesh2D->Draw();

        }
        m_mesh2D->Unbind();
    }
}