// Shared by the image-based lighting compute shaders, pasted in with  #include "ibl_common.glsl".

const float PI = 3.14159265359;

// Direction through the centre of texel (x, y) of a cube map face, following OpenGL's cube map layout.
// Faces are numbered +X, -X, +Y, -Y, +Z, -Z, which is also the layer order of an imageCube.
vec3 cubeTexelDirection(int face, ivec2 texel, int size)
{
    vec2 st = (vec2(texel) + 0.5) / float(size) * 2.0 - 1.0;
    vec3 d;
    switch (face) {
    case 0:  d = vec3(1.0, -st.y, -st.x); break;
    case 1:  d = vec3(-1.0, -st.y, st.x); break;
    case 2:  d = vec3(st.x, 1.0, st.y); break;
    case 3:  d = vec3(st.x, -1.0, -st.y); break;
    case 4:  d = vec3(st.x, -st.y, 1.0); break;
    default: d = vec3(-st.x, -st.y, -1.0); break;
    }
    return normalize(d);
}

// Where a direction lands in an equirectangular panorama stored top row first. Same convention as the
// Khronos glTF Sample Renderer: v = 0 at the top of the sky; u runs around the horizon starting at -X.
vec2 directionToEquirectUV(vec3 d)
{
    return vec2(0.5 + 0.5 * atan(d.z, d.x) / PI, acos(clamp(d.y, -1.0, 1.0)) / PI);
}