R"(

varying vec2 texCoord;
uniform sampler2D texture0;

void main(void)
{
    gl_FragColor = texture(texture0, texCoord).rrra;
}
)"

