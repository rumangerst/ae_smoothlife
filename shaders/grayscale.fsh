R"(

varying vec2 texCoord;
uniform sampler2D texture0;
uniform float time;
uniform float field_w;
uniform float field_h;

void main(void)
{
    gl_FragColor = texture(texture0, texCoord).rrra;
}
)"

