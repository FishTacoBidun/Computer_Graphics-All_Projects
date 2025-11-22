#version 330 compatibility

uniform float uPigD; // center of bulge
uniform float uPigH; // bulge height

out vec3 vL; // vector from the vertex to the light
out vec3 vN; // surface normal vector
out vec3 vE; // vector from the vertex to the eye

const float PigW = 4.0; // width of the bulge
const vec3 LightPosition = vec3(15., 15., 15.); // light position

float smoothpulse(float e0, float e1, float e2, float e3, float x)
{
    float left  = smoothstep(e0, e1, x);
    float right = 1.0 - smoothstep(e2, e3, x);
    return left * right;
}

void main()
{
    vec4 MCvertex = gl_Vertex; // vertex in model coordinates

    float e0 = uPigD - PigW/2.;
    float e1 = uPigD - PigW/4.;
    float e2 = uPigD + PigW/4.;
    float e3 = uPigD + PigW/2.;

    float pulse = smoothpulse(e0, e1, e2, e3, MCvertex.x); // needs to range from 0. to 1. -- see smoothpulse notes

    float yzscale = 1.0 + pulse * uPigH;
    MCvertex.yz *= yzscale; // scale the thickness of the snake

    vec4 ECposition = gl_ModelViewMatrix * MCvertex; // vertex position in eye coordinates

    vN = normalize(gl_NormalMatrix * gl_Normal); // normal vector
    vL = LightPosition - ECposition.xyz; // vector from the vertex to the light position
    vE = -ECposition.xyz; // vector from the vertex to the eye position

    gl_Position = gl_ModelViewProjectionMatrix * MCvertex;
}