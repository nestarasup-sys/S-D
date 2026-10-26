#version 440

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D sourceTexture;
layout(binding = 2) uniform sampler2D maskTexture;
layout(std140, binding = 0) uniform EffectUniforms {
    float opacity;
    float maskStrength;
};

void main()
{
    vec4 source = texture(sourceTexture, texCoord);
    float maskAlpha = texture(maskTexture, texCoord).a;
    float factor = mix(1.0, maskAlpha, clamp(maskStrength, 0.0, 1.0)) * clamp(opacity, 0.0, 1.0);
    fragColor = source * factor;
}
