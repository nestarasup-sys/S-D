#version 440

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D sourceTexture;
layout(binding = 2) uniform sampler2D blurredTexture;
layout(std140, binding = 0) uniform EffectUniforms {
    vec4 params;
    vec4 glowColor;
};

void main()
{
    vec4 source = texture(sourceTexture, texCoord);
    float alpha = clamp(texture(blurredTexture, texCoord).a * params.y * glowColor.a, 0.0, 1.0);
    vec4 glow = vec4(glowColor.rgb * alpha, alpha);
    vec4 outerGlow = source + glow * (1.0 - source.a);
    fragColor = outerGlow * clamp(params.x, 0.0, 1.0);
}
