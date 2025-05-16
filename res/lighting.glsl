#ifndef LIGHTING_H
#define LIGHTING_H
#pragma stage: frag

#include "pbrUtils.glsl"
#include "shaderMath.glsl"

// Implementation based on the Lighting.glslh include file for the Hazel Engine, all due credit goes to @StudioCherno


struct PBRParameters
{
	vec3 Albedo;
	float Roughness;
	float Metalness;

	vec3 Normal;
	vec3 View;
	float NdotV;
} m_params;

vec3 CalculateDirLights()
{
    const vec3 Fdielectric = vec3(0.04);
    vec3 F0 = mix(Fdielectric, m_params.Albedo, m_params.Metalness);
    
	vec3 result = vec3(0.0);
	for (int i = 0; i < 1; i++) //Only one light for now
	{
		float sunPower = u_sceneData.sunlightDirection.w;
		if (sunPower == 0.0)			continue;

		vec3 Li = u_sceneData.sunlightDirection.xyz;
		vec3 Lradiance = u_sceneData.sunlightColor.xyz * sunPower;
		vec3 Lh = normalize(Li + m_params.View);

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(m_params.Normal, Li));
		// float cosLh = max(0.0, dot(m_params.Normal, Lh)); // This is just NdotH

		// vec3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, m_params.View)), m_params.Roughness);
		vec3 F = FresnelSchlick(F0, max(0.0, dot(Lh, m_params.View)));		
		float D = DistributionGGX(m_params.Normal, Lh, m_params.Roughness);
		float G = GaSchlickGGX(cosLi, m_params.NdotV, m_params.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - m_params.Metalness);
		vec3 diffuseBRDF = kd * m_params.Albedo;

		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * m_params.NdotV);
		specularBRDF = clamp(specularBRDF, vec3(0.0f), vec3(10.0f));
		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}
	return result;
}
#endif
