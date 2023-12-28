#version 460

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D fontTexture;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragUv;

void main()
{
    outColor = fragColor * texture(fontTexture, fragUv.st);
}