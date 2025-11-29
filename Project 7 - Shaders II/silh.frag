#version 330 compatibility

uniform bool	uSilh;
uniform bool	uSurface;
uniform float 	uTol;

in  vec3  	vN;			// normal vector
in  vec3  	vL;			// vector from point to light
in  vec3  	vE;			// vector from point to eye

const vec3 	Color =		vec3( 0., 1., 0.8 );
const float	Ka =		0.1;
const float	Kd =		0.6;
const float	Ks =		0.3;
const float	Shininess =	30.;
const vec3	SpecularColor =	vec3( 1.0, 1.0, 1.0 );
const vec3	SilhColor =	vec3( 1.0, 0.5, 0.0 );

void
main()
{
	vec3 myColor = Color;

// per-fragment lighting:

	vec3 Normal    = normalize(vN);
	vec3 Light     = normalize(vL);
	vec3 Eye       = normalize(vE);

	vec3 ambient = Ka * myColor;

	float d = max( dot(Normal,Light), 0. );       // only do diffuse if the light can see the point
	vec3 diffuse = Kd * d * myColor;

	float s = 0.;
	if( d > 0. )	          // only do specular if the light can see the point
	{
		vec3 ref = normalize(  reflect( -Light, Normal )  );
		float cosphi = dot( Eye, ref );
		if( cosphi > 0. )
			s = pow( max( cosphi, 0. ), Shininess );
	}
	vec3 specular = Ks * s * SpecularColor;

// all the silhouette stuff happens below here:

	float edge = abs(dot(Normal, Eye));
	bool isSilhouette = (edge < uTol);

	if( uSilh   &&   isSilhouette )
	{
		gl_FragColor = vec4( SilhColor, 1. );
	}
	else
	{
		if( uSurface )
			gl_FragColor = vec4( ambient + diffuse + specular,  1. );
		else
			discard;
	}
}
