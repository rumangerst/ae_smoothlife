R"(

varying vec2 texCoord;

void main(void)
{   
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
    texCoord = gl_MultiTexCoord0.xy;
}

)";
