#version 460

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

layout(push_constant) uniform PushConstant
{
    vec2 scale;
} pushConstant;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragUv;

void main()
{
    fragColor = color;
    fragUv = uv;
    gl_Position = vec4(pos * pushConstant.scale - 1, 0, 1);
}