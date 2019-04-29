#version 460 core

layout ( location = 0 ) in vec2 vcoord;
layout ( location = 0 ) out vec4 uFragColor;
layout ( binding = 2, rgba32f ) uniform image2D outputImage;

void main() {
    //imageStore(outputImage, ivec2(vcoord*imageSize(outputImage)), vec4(1.f.xxx,1.f));
    const ivec2 size = imageSize(outputImage);
    ivec2 coord = ivec2(vcoord*size); coord.y = (size.y - 1) - coord.y;
    uFragColor = vec4(imageLoad(outputImage,coord).xyz,1.f);
}
