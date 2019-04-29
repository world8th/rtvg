#version 460 core

layout ( location = 0 ) in vec2 vcoord;
layout ( location = 0 ) out vec4 uFragColor;
layout ( binding = 2, rgba32f ) readonly uniform image2D outputImage;

void main() {
    uFragColor = vec4(imageLoad(outputImage, ivec2(vcoord*imageSize(outputImage))).xyz,1.f);
}
