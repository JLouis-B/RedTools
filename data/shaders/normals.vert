
uniform mat4 mWorldViewProj;
uniform mat4 mInvWorld;
uniform mat4 mTransWorld;

void main(void)
{
	gl_Position = mWorldViewProj * gl_Vertex;

	vec4 normal = vec4(gl_Normal, 0.0);
	normal = mInvWorld * normal;
	normal = normalize(normal);

	gl_FrontColor = gl_BackColor = vec4(normal.x, normal.y, normal.z, 0.0);

	gl_TexCoord[0] = gl_MultiTexCoord0;
}
