#include "Transform.hpp"
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

Hush::Transform::Transform(const glm::vec3 &position, const glm::vec3 &scale, const glm::quat &rotation)
    : m_position(position),
      m_scale(scale),
      m_rotation(rotation)
{
}

void Hush::Transform::SetPosition(const glm::vec3 &position) noexcept
{
    this->m_position = position;
}

glm::vec3 Hush::Transform::GetPosition() const noexcept
{
    return this->m_position; 
}

void Hush::Transform::SetScale(const glm::vec3& scale) noexcept {
    this->m_scale = scale;
}

glm::vec3 Hush::Transform::GetScale() const noexcept
{
    return this->m_scale;
}

void Hush::Transform::SetRotationQuat(const glm::quat& rotationQuat) noexcept {
    this->m_rotation = rotationQuat;
}

glm::quat Hush::Transform::GetRotationQuat() const noexcept
{
    return this->m_rotation;
}


glm::vec3 Hush::Transform::Forward() const noexcept {
    return this->m_rotation * glm::vec3(0.0F, 0.0F, this->m_scale.z);
}

glm::vec3 Hush::Transform::Up() const noexcept {
    return this->m_rotation * glm::vec3(0.0F, this->m_scale.y, 0.0F);
}

glm::vec3 Hush::Transform::Right() const noexcept {
    return this->m_rotation * glm::vec3(this->m_scale.x, 0.0F, 0.0F);
}

