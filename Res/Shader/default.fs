#version 330 core

out vec4 FragColor;
in vec3 outNormal;
void main()
{    
	vec3 normal = normalize(outNormal);
	vec3 ambient = vec3(0.1, 0.1, 0.1);
	vec3 lightColor = vec3(0.8, 0.8, 0.8);
    vec3 lightDir = normalize(vec3(0, 1, 1));
	float ndl = max(0.0,dot(normal,lightDir));
	vec3 diffuse =lightColor * ndl;

	FragColor = vec4(diffuse + ambient, 1.0);
}