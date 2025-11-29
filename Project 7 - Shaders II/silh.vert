#version 330 compatibility

out  vec3  	vN;		// normal vector
out  vec3  	vL;		// vector from point to light
out  vec3  	vE;		// vector from point to eye

const vec3 LightPosition	= vec3( 15. ,15. ,15.);

void
main()
{
	vN =  normalize( gl_NormalMatrix * gl_Normal );	// normal vector

	vec4 ECposition = gl_ModelViewMatrix * gl_Vertex;

	vL = LightPosition - ECposition.xyz;		// vector from the point to the light position

	vE = vec3( 0., 0., 0. ) - ECposition.xyz;	// vector from the point to the eye position 

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
