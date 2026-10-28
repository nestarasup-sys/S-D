#version 440

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D sourceTexture;
layout(binding = 2) uniform sampler2D unusedTexture;
layout(std140, binding = 0) uniform EffectUniforms {
    vec4 params;
    vec4 glowColor;
};

void main()
{
    int r = clamp(int(params.x + 0.5), 1, 24);
    float sigma = max(0.5, float(r) / 3.0);
    float denominator = 2.0 * sigma * sigma;
    vec4 sum = vec4(0.0);
    float total = 0.0;
    for (int tap = -24; tap <= 24; ++tap) {
        if (abs(tap) > r)
            continue;
        float weight = exp(-float(tap * tap) / denominator);
        vec2 sampleUv = clamp(texCoord + vec2(0.0, params.z) * float(tap), vec2(0.0), vec2(1.0));
        sum += texture(sourceTexture, sampleUv) * weight;
        total += weight;
    }
    fragColor = sum / max(total, 0.00001) * clamp(params.w, 0.0, 1.0);
}
