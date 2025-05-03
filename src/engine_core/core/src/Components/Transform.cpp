#include "../../base/src/Common.hpp"
#include "Transform.hpp"
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "Mat4Math.hpp"

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

const glm::vec3 &Hush::Transform::GetPosition() const noexcept
{
	return this->m_position;
}

glm::vec3 &Hush::Transform::GetPosition() noexcept
{
	return this->m_position;
}

void Hush::Transform::SetScale(const glm::vec3 &scale) noexcept
{
	this->m_scale = scale;
}

const glm::vec3 &Hush::Transform::GetScale() const noexcept
{
	return this->m_scale;
}

glm::vec3 &Hush::Transform::GetScale() noexcept
{
	return this->m_scale;
}

void Hush::Transform::SetRotationQuat(const glm::quat &rotationQuat) noexcept
{
	this->m_rotation = rotationQuat;
}

glm::quat Hush::Transform::GetRotationQuat() const noexcept
{
	return this->m_rotation;
}

void Hush::Transform::SetEulerAngles(const glm::vec3 &euler) noexcept
{
	this->m_rotation = glm::quat(euler);
}

glm::vec3 Hush::Transform::GetEulerAngles() const noexcept
{
	return glm::eulerAngles(this->m_rotation);
}

glm::vec3 Hush::Transform::Forward() const noexcept
{
	return this->m_rotation * glm::vec3(0.0F, 0.0F, this->m_scale.z);
}

glm::vec3 Hush::Transform::Up() const noexcept
{
	return this->m_rotation * glm::vec3(0.0F, this->m_scale.y, 0.0F);
}

glm::vec3 Hush::Transform::Right() const noexcept
{
	return this->m_rotation * glm::vec3(this->m_scale.x, 0.0F, 0.0F);
}

void Hush::Transform::SetTransformationMatrix(const glm::mat4 &xform)
{
	Mat4Math::DecomposeTRS(xform, this->m_position, this->m_rotation, this->m_scale);
}


glm::mat4 Hush::Transform::GetTransformationMatrix() const {
	return Mat4Math::ComposeTRS(this->m_position, this->m_rotation, this->m_scale);
}


glm::mat4 Hush::Transform::XForm(const Transform& other) const {
	return this->GetTransformationMatrix() * other.GetTransformationMatrix();
}

glm::mat4 Hush::Transform::operator*(const Transform &other) const
{
	return this->XForm(other);
}
