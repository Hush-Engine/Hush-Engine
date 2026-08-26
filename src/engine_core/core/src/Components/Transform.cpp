#include "../../base/src/Common.hpp"
#include "Transform.hpp"
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "Mat4Math.hpp"

Hush::Transform::Transform(const glm::vec3 &position, const glm::vec3 &scale, const glm::quat &rotation)
{
	this->m_transform = Mat4Math::ComposeTRS(position, rotation, scale);
}

void Hush::Transform::SetPosition(glm::vec3 position) noexcept
{
	this->m_transform[Mat4Math::TRANSLATION_COLUMN] = glm::vec4(position, 1.0F);
	this->m_dirty = true;
}

// TODO: Remove these two functions, access should not skip the dirty flag
const glm::vec3 *Hush::Transform::GetPosition() const noexcept
{
	// WARN: This is potentially undefined behaviour, but, should work for all compilers
	// NOLINTNEXTLINE
	return reinterpret_cast<const glm::vec3 *>(&this->m_transform[Mat4Math::TRANSLATION_COLUMN]);
}

glm::vec3 *Hush::Transform::GetPosition() noexcept
{
	// WARN: This is potentially undefined behaviour, but, should work for all compilers
	// NOLINTNEXTLINE
	return reinterpret_cast<glm::vec3 *>(&this->m_transform[Mat4Math::TRANSLATION_COLUMN]);
}

glm::vec3 Hush::Transform::GetPositionValue() const noexcept
{
	return *reinterpret_cast<glm::vec3 *>(&this->m_transform[Mat4Math::TRANSLATION_COLUMN]);
}

void Hush::Transform::SetScale(glm::vec3 scale) noexcept
{
	this->m_scale = scale;
	this->m_dirty = true;
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
	this->m_dirty = true;
}

glm::quat Hush::Transform::GetRotationQuat() const noexcept
{
	return this->m_rotation;
}

void Hush::Transform::SetEulerAngles(const glm::vec3 &euler) noexcept
{
	this->m_rotation = glm::quat(euler);
	this->m_dirty = true;
}

glm::vec3 Hush::Transform::GetEulerAngles() const noexcept
{
	return glm::eulerAngles(this->m_rotation);
}

glm::vec3 Hush::Transform::Forward() const noexcept
{
	// TODO: Make this a direct access
	return this->m_rotation * glm::vec3(0.0F, 0.0F, 1.0F);
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
	// This overrides any other previous setters we made
	this->m_dirty = false;
	this->m_transform = xform;
	glm::vec3 discarded;
	Mat4Math::DecomposeTRS(this->m_transform, discarded, this->m_rotation, this->m_scale);
}

glm::mat4 Hush::Transform::GetTransformationMatrix() const
{
	if (!this->m_dirty)
	{
		return this->m_transform;
	}
	const glm::vec3 *position = this->GetPosition();
	this->m_transform = Mat4Math::ComposeTRS(*position, this->m_rotation, this->m_scale);
	this->m_dirty = false;
	return this->m_transform;
}

glm::mat4 Hush::Transform::XForm(const Transform &other) const
{
	return this->GetTransformationMatrix() * other.GetTransformationMatrix();
}

glm::mat4 Hush::Transform::InvXForm(const Transform &other) const
{
	return glm::inverse(this->GetTransformationMatrix()) * other.GetTransformationMatrix();
}

glm::mat4 Hush::Transform::operator*(const Transform &other) const
{
	return this->XForm(other);
}
