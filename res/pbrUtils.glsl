const float PI = 3.14159265;
const float EPSILON = 0.000001f;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float GeometrySchlickGGX(float NdotV, float alpha) {
    float k = alpha / 2;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float alpha) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, alpha);
    float ggx1 = GeometrySchlickGGX(NdotL, alpha);
    return ggx1 * ggx2;
}

float DistributionGGX(vec3 N, vec3 H, float alpha) {
    float a2 = alpha * alpha;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

vec3 PBR(vec3 albedo, float metallic, float roughness, vec3 N, vec3 V, vec3 L, vec3 radiance)
{
    float alpha = roughness * roughness;
    vec3 H = normalize(V + L);
    float cosTheta = max(dot(N, L), 0.0);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    float NDF = DistributionGGX(N, H, alpha);    
    float G = GeometrySmith(N, V, L, alpha);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + EPSILON;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    vec3 directionalContribution = (kD * albedo / PI + specular) * radiance * NdotL;
    
    return directionalContribution;   
}
