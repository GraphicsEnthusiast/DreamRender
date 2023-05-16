#version 330 core

in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D texPass0;
uniform sampler2D texPass1;
uniform sampler2D texPass2;
uniform sampler2D texPass3;
uniform sampler2D texPass4;
uniform sampler2D texPass5;
uniform sampler2D texPass6;

vec3 ToneMapping(in vec3 c, float limit) {
    float luminance = 0.3f * c.x + 0.6f * c.y + 0.1f * c.z;
    return c * 1.0f / (1.0f + luminance / limit);
}

vec3 ACES_ToneMapping(in vec3 x)  {
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;

    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

void main() {
    vec3 color = texture2D(texPass0, texCoord).rgb;
    color = ToneMapping(color, 1.5f);
//  color = ACES_ToneMapping(color);
    color = pow(color, vec3(1.0f / 2.2f));

    fragColor = vec4(color, 1.0f);
}
