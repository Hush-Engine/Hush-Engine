#include "Camera.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/gtc/type_ptr.hpp>

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


glm::vec3 Hush::Camera::ScreenToWorldPos(glm::mat4 viewMatrix, glm::vec2 mousePos, glm::vec3* outDirection) const {

	glm::vec2 viewportSize = this->GetViewportSize();
	// Convert window-space mouse pos to viewport-relative before computing NDC.
	glm::vec2 localPos = mousePos - this->m_viewportOffset;
	glm::vec2 ndc;
	ndc.x = (2.0f * localPos.x) / viewportSize.x - 1.0f;
	// Flip Y bc -Y is bottom in our coordinate space
	ndc.y = 1.0f - (2.0f * localPos.y) / viewportSize.y;

	glm::mat4 proj = this->GetProjectionMatrix();

	glm::vec4 clipNear = glm::vec4(ndc.x, ndc.y, -1.0f, 1.0f);
	glm::vec4 clipFar  = glm::vec4(ndc.x, ndc.y,  1.0f, 1.0f);

	// Transform from clip space to our local view space
	glm::mat4 invProj = glm::inverse(proj);
	glm::vec4 viewNear = invProj * clipNear;
	glm::vec4 viewFar  = invProj * clipFar;

	viewNear /= viewNear.w;
	viewFar  /= viewFar.w;

	// Transform View space into world space
	glm::mat4 invView = glm::inverse(viewMatrix);
	glm::vec4 worldNear = invView * viewNear;
	glm::vec4 worldFar  = invView * viewFar;

	// 6. Construct the ray
	auto origin = glm::vec3(worldNear);
	if (outDirection != nullptr) {
		*outDirection = glm::normalize(glm::vec3(worldFar - worldNear));
	}
	return origin;
}

glm::vec3 Hush::Camera::ScreenToWorldPosUnsafe(float* viewMatrix, glm::vec2 mousePos, glm::vec3* outDirection) const {
	glm::mat4 worldTransform = glm::make_mat4(viewMatrix);
	// Scripting passes the camera's world transform (local→world); ScreenToWorldPos expects the
	// view matrix (world→camera), so invert here.
	glm::mat4 viewMat = glm::inverse(worldTransform);
	return this->ScreenToWorldPos(viewMat, mousePos, outDirection);
}

glm::vec3 Hush::Camera::ProjectPlanePosition(glm::vec3 origin, glm::vec3 direction, float height) {

	glm::vec3 planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 planePoint = glm::vec3(0.0f, height, 0.0f);

    // Check if ray is parallel to the plane
    float denom = glm::dot(direction, planeNormal);
    if (std::abs(denom) < 1e-6f) { 
    	// BUG: No intersection should return inf, not 0
        return glm::vec3(0.f);
	}

    // Calculate distance (t) from ray origin to plane
    float t = glm::dot(planePoint - origin, planeNormal) / denom;
    if (t < 0.0f) { 
        return glm::vec3(0.0f); // Intersection is behind the camera
	}

	// World pos
    return origin + t * direction;	
}

float Hush::Camera::GetFarPlane() const noexcept
{
	return this->m_farPlane;
}
