#version 330 core

in vec2 texCoord;

uniform uint frameCounter;
uniform sampler2D lastFrame;
uniform sampler2D nowFrame;

void main() {
    vec3 lastColor = texture2D(lastFrame, texCoord).rgb;
    vec3 nowColor = texture2D(nowFrame, texCoord).rgb;
    vec3 color = mix(lastColor, nowColor, 1.0f / float(frameCounter + 1.0f));
    gl_FragData[0] = vec4(color, 1.0f);
}
