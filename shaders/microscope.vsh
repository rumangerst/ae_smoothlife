R"(

varying vec2 texCoord;
uniform float render_w;
uniform float render_h;

void main(void)
{   
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
    texCoord = gl_MultiTexCoord0.xy;
}

)";
