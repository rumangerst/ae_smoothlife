R"(

//f-decl
vec4 BiCubic( sampler2D textureSampler, vec2 TexCoord );

varying vec2 texCoord;
uniform sampler2D texture0;
uniform float render_w;
uniform float render_h;
uniform float field_w;
uniform float field_h;
uniform float time;

vec3 color_bgr = vec3(103.0/255.0,129.0/255.0,164.0/255.0);
vec3 color_dark = vec3(83.0/255.0,107.0/255.0,140.0/255.0);
vec3 color_bright = vec3(160.0 / 255.0,175.0 / 255.0,190.0 /255.0);
vec3 color_green = vec3(55.0 / 160.0,174.0 / 255.0,57.0 / 255.0);
vec3 color_green2 = vec3(28.0 / 255.0,89.0 / 255.0,29.0 / 255.0);

const float BGR_FOCUS_BASE_WEAKNESS = 15.0;

vec2 rotate(vec2 p, float angle)
{
    mat2 M = mat2( cos( angle ), -sin( angle ),
                   sin( angle ),  cos( angle ));

    return M * p;
}

float filling(vec2 coord, float n_pixel)
{
    float n = n_pixel / field_w; // pixel -> texcoord
    float N = 0;

    N += BiCubic(texture0, coord + n * vec2(-1.0,-1.0)).r;
    N += BiCubic(texture0, coord + n * vec2(-1.0,1.0)).r;
    N += BiCubic(texture0, coord + n * vec2(1.0,1.0)).r;
    N += BiCubic(texture0, coord + n * vec2(1.0,-1.0)).r;

    return N / 4.0;
}

float minmax_diff(vec2 coord, float n_pixel)
{
    float n = n_pixel / field_w; // pixel -> texcoord
    float N = 0;

    float p1 = BiCubic(texture0, coord + n * vec2(-1.0,-1.0)).r;
    float p2 = BiCubic(texture0, coord + n * vec2(-1.0,1.0)).r;
    float p3 = BiCubic(texture0, coord + n * vec2(1.0,1.0)).r;
    float p4 = BiCubic(texture0, coord + n * vec2(1.0,-1.0)).r;
    float p5 = BiCubic(texture0, coord).r;
    float p6 = BiCubic(texture0, coord + n * vec2(-1.0,0.0)).r;
    float p7 = BiCubic(texture0, coord + n * vec2(1.0,0.0)).r;
    float p8 = BiCubic(texture0, coord + n * vec2(0.0,-1.0)).r;
    float p9 = BiCubic(texture0, coord + n * vec2(0.0,1.0)).r;

    return max(p1,max(p2,max(p3,max(p4,max(p5,max(p6,max(p7,max(p8,p9)))))))) - min(p1,min(p2,min(p3,min(p4,min(p5,min(p6,min(p7,min(p8,p9))))))));
}

float nsin(float x)
{
    return (1.0 + sin(x)) / 2.0;
}

float ncos(float x)
{
    return (1.0 + cos(x)) / 2.0;
}

vec3 calculate_color(vec3 bgr, vec2 coord)
{
    float light_x = 1.0 / field_w;
    float light_y = 1.0 / field_h;

    float f = BiCubic(texture0, coord).r;
    float f_light = BiCubic(texture0, coord + vec2(light_x, light_y)).r;

    //Calculate base color
    vec3 color = mix(bgr, color_dark, f);

    // "Green" effect
    float green_n = filling(coord, 2) - 0.0 * minmax_diff(coord, 1);
    float f_green = pow(green_n, 3);

    //float f_green = green_n * filling(coord, 3);

    color = mix(color, color_green, f_green);

    // "3D" effect
    color += mix(vec3(0), color_bright, (f_light - f) * 0.2);

    //color = vec3(f_green);

    return color;
}

void main(void)
{
    // Better background
    vec3 bgr = mix(color_bgr, color_bgr * 1.2, (1 - texCoord.y) * (1 - texCoord.x));

    //Calculate main
    vec3 front = calculate_color(bgr, texCoord);

    // Calculate background
    vec3 back = calculate_color( color_bgr, rotate(vec2(texCoord.x, 1.0 - texCoord.y) / 1.5, (0.5 - sin(time / 100000)) ) );

    // Mix them
    float mix_amount = 2.0 + BGR_FOCUS_BASE_WEAKNESS *  distance(texCoord, vec2(nsin(time / 100000), ncos(time / 12000))) ;

    vec3 mixed = (front * mix_amount + back) / (mix_amount + 1.0);

    gl_FragColor = vec4(mixed, 1.0);

    //gl_FragColor = vec4(front, 1.0);
}


/**
  BiCubic filtering
  src: http://www.codeproject.com/Articles/236394/Bi-Cubic-and-Bi-Linear-Interpolation-with-GLSL
 */

float BSpline( float x )
{
        float f = x;
        if( f < 0.0 )
        {
                f = -f;
        }

        if( f >= 0.0 && f <= 1.0 )
        {
                return ( 2.0 / 3.0 ) + ( 0.5 ) * ( f* f * f ) - (f*f);
        }
        else if( f > 1.0 && f <= 2.0 )
        {
                return 1.0 / 6.0 * pow( ( 2.0 - f  ), 3.0 );
        }
        return 1.0;
}

float Triangular( float f )
{
        f = f / 2.0;
        if( f < 0.0 )
        {
                return ( f + 1.0 );
        }
        else
        {
                return ( 1.0 - f );
        }
        return 0.0;
}

float CatMullRom( float x )
{
    const float B = 0.0;
    const float C = 0.5;
    float f = x;
    if( f < 0.0 )
    {
        f = -f;
    }
    if( f < 1.0 )
    {
        return ( ( 12 - 9 * B - 6 * C ) * ( f * f * f ) +
            ( -18 + 12 * B + 6 *C ) * ( f * f ) +
            ( 6 - 2 * B ) ) / 6.0;
    }
    else if( f >= 1.0 && f < 2.0 )
    {
        return ( ( -B - 6 * C ) * ( f * f * f )
            + ( 6 * B + 30 * C ) * ( f *f ) +
            ( - ( 12 * B ) - 48 * C  ) * f +
            8 * B + 24 * C)/ 6.0;
    }
    else
    {
        return 0.0;
    }
}

vec4 BiCubic( sampler2D textureSampler, vec2 TexCoord )
{
    float texelSizeX = 1.0 / field_w; //size of one texel
    float texelSizeY = 1.0 / field_h; //size of one texel
    vec4 nSum = vec4( 0.0, 0.0, 0.0, 0.0 );
    vec4 nDenom = vec4( 0.0, 0.0, 0.0, 0.0 );
    float a = fract( TexCoord.x * field_w ); // get the decimal part
    float b = fract( TexCoord.y * field_h ); // get the decimal part
    for( int m = -1; m <=2; m++ )
    {
        for( int n =-1; n<= 2; n++)
        {
                        vec4 vecData = texture2D(textureSampler,
                               TexCoord + vec2(texelSizeX * float( m ),
                                        texelSizeY * float( n )));
                        float f  = CatMullRom( float( m ) - a );
                        vec4 vecCooef1 = vec4( f,f,f,f );
                        float f1 = CatMullRom ( -( float( n ) - b ) );
                        vec4 vecCoeef2 = vec4( f1, f1, f1, f1 );
            nSum = nSum + ( vecData * vecCoeef2 * vecCooef1  );
            nDenom = nDenom + (( vecCoeef2 * vecCooef1 ));
        }
    }
    return nSum / nDenom;
}

)"

