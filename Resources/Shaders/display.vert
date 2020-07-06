#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;
layout(location = 0) out vec2 v_texCoord;

void main() 
{
    v_texCoord = a_texCoord;
    gl_Position = vec4(a_position, 1.0);
}