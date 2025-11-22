#version 330 compatibility

in vec3 vN; // surface normal vector of the current fragment
in vec3 vL; // vector from current fragment to the light
in vec3 vE; // vector from current fragment to the eye

const vec3 Color = vec3( 0., 1., 0.5 ); // snake color
const vec3 SpecularColor = vec3( 1., 1., 1. ); // specular highlight color

const float Ka = 0.1; // amount of ambient;
const float Kd = 0.6; // amount of diffuse
const float Ks = 0.3; // amount of specular
const float Shininess = 30.; // bright spot shininess

void
main( )
{
	vec3 myColor = Color;

	vec3 Normal = normalize(vN);
	vec3 Light = normalize(vL);
	vec3 Eye = normalize(vE);

	vec3 ambient = Ka * myColor;

	float dd = max( dot(Normal,Light), 0. ); // only do diffuse if the light can see the point
	vec3 diffuse = Kd * dd * myColor;

	float s = 0.;
	if( dd > 0. ) // only do specular if the light can see the point
	{
		vec3 ref = normalize( reflect( -Light, Normal ) );
		float cosphi = dot( Eye, ref );
		if( cosphi > 0. )
		s = pow( max( cosphi, 0. ), Shininess );
	}

	vec3 specular = Ks * s * SpecularColor;
	gl_FragColor = vec4( ambient + diffuse + specular, 1. );
}