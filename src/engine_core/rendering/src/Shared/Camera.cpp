#include "Camera.hpp"

Hush::Camera::Camera(const glm::mat4 &projectionMat, const glm::mat4 &unreversedProjectionMat) noexcept
    : m_projectionMatrix(projectionMat),
      m_unreversedProjectionMatrix(unreversedProjectionMat)
{
}

Hush::Camera::Camera(float degFov, float width, float height, float nearP, float farP) noexcept
{
    this->m_fov = degFov;
    this->m_viewportSize = {width, height};
    this->m_nearPlane = nearP;
    this->m_farPlane = farP;
}

const glm::mat4 &Hush::Camera::GetUnreversedProjectionMatrix() const noexcept
{
    return this->m_unreversedProjectionMatrix;
}

void Hush::Camera::SetProjectionMatrix(const glm::mat4 projection, const glm::mat4 unReversedProjection)
{
    this->m_projectionMatrix = projection;
    this->m_unreversedProjectionMatrix = unReversedProjection;
}

void Hush::Camera::SetPerspectiveProjectionMatrix(const float radFov, const float width, const float height,
                                                  const float nearP, const float farP)
{
    // Yes, even though the last two parameters seem to be reversed, this is how other engines seem to be doing it
    // but, you know, adding this for future bugs and stuff
    this->m_projectionMatrix = glm::perspectiveFov(radFov, width, height, farP, nearP);
    this->m_unreversedProjectionMatrix = glm::perspectiveFov(radFov, width, height, nearP, farP);
}

float Hush::Camera::GetFarPlane() const noexcept
{
    return this->m_farPlane;
}
