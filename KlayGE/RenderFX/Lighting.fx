float DirectionalLighting(float3 lightDir, float3 normal)
{
	return dot(-lightDir, normal);
}

float PointLighting(float3 lightPos, float3 pos, float3 normal)
{
	return dot(lightPos - pos, normal);
}
