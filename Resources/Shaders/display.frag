#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 outColor;
//layout(binding = 1) uniform sampler2D u_texture;

void main()
{
    outColor = vec4(1.0);//texture(u_texture, v_texCoord);
}