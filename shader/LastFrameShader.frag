#version 330 core

in vec2 texCoord;

uniform sampler2D texPass0;
uniform sampler2D texPass1;
uniform sampler2D texPass2;
uniform sampler2D texPass3;
uniform sampler2D texPass4;
uniform sampler2D texPass5;
uniform sampler2D texPass6;

void main() {
    vec3 color = texture2D(texPass0, texCoord).rgb;
    gl_FragData[0] = vec4(color, 1.0f);
}
