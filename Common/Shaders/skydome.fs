#version 410

layout(location = 0) out vec4 FragColor;

uniform sampler2D gSampler;

uniform vec4 gLowColor = vec4(253.0/256.0, 94.0/256.0, 0.0/256.0, 1.0);
uniform vec4 gHighColor = vec4(50.0/256.0, 146.0/256.0, 246.0/256.0, 1.0);

in vec2 TexCoords0;
in float Height;

void main()
{
      vec4 TexColor = texture(gSampler, TexCoords0.xy);

      vec4 SkyColor = mix(gLowColor, gHighColor, Height);

      FragColor = TexColor * 0.7 + SkyColor * 0.3;
}
