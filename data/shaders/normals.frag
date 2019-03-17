
void main (void)
{
    gl_FragColor = vec4((gl_Color.x + 1)/2.0, (gl_Color.y + 1)/2.0, (gl_Color.z + 1)/2.0, 1.0);
}
