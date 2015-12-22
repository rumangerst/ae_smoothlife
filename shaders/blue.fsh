R"(


varying vec2 texCoord;
uniform sampler2D texture0;
uniform float render_w;
uniform float render_h;
uniform float field_w;
uniform float field_h;
uniform float time;

vec3 color_bgr = vec3(0,0,0);
vec3 color_dark = 1 - vec3(83.0/255.0,107.0/255.0,140.0/255.0);
vec3 color_bright = 1 - vec3(160.0 / 255.0,175.0 / 255.0,190.0 /255.0);
vec3 color_dotty = vec3(0.3,0.3,0.67);
vec3 color_swirly = vec3(23.0 / 255.0, 148.0 / 255.0, 209.0 / 255.0);

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

    N += texture(texture0, coord + n * vec2(-1.0,-1.0)).r;
    N += texture(texture0, coord + n * vec2(-1.0,1.0)).r;
    N += texture(texture0, coord + n * vec2(1.0,1.0)).r;
    N += texture(texture0, coord + n * vec2(1.0,-1.0)).r;

    return N / 4.0;
}

float minmax_diff(vec2 coord, float n_pixel)
{
    float n = n_pixel / field_w; // pixel -> texcoord
    float N = 0;

    float p1 = texture(texture0, coord + n * vec2(-1.0,-1.0)).r;
    float p2 = texture(texture0, coord + n * vec2(-1.0,1.0)).r;
    float p3 = texture(texture0, coord + n * vec2(1.0,1.0)).r;
    float p4 = texture(texture0, coord + n * vec2(1.0,-1.0)).r;
    float p5 = texture(texture0, coord).r;
    float p6 = texture(texture0, coord + n * vec2(-1.0,0.0)).r;
    float p7 = texture(texture0, coord + n * vec2(1.0,0.0)).r;
    float p8 = texture(texture0, coord + n * vec2(0.0,-1.0)).r;
    float p9 = texture(texture0, coord + n * vec2(0.0,1.0)).r;

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
    float light_x = 0.5 / field_w;
    float light_y = 0.5 / field_h;

    float f = texture(texture0, coord).r;
    float f_light = texture(texture0, coord + vec2(light_x, light_y)).r;

    //Calculate base color
    vec3 border = vec3(0);
    vec3 dotty = vec3(0);
    vec3 swirly = vec3(0);

    // "Dotty" effect
    float dotty_n = filling(coord, 2);
    float f_dotty = pow(dotty_n, 3);

    dotty = color_dotty * f_dotty;

    // "Swirly" effect
    float swirly_n = filling(coord, 1);
    float f_swirly = clamp(pow(swirly_n, 3) - f_dotty,0,1);

    swirly = f_swirly * color_swirly;

    // "3D" effect
    border = color_bright * (f_light - f) * 0.2;

    return f * swirly + dotty + border;
}

void main(void)
{
    // Better background
    //vec3 bgr = mix(color_bgr, color_bgr * 1.2, (1 - texCoord.y) * (1 - texCoord.x));

    //Calculate main
    vec3 front = calculate_color(color_bgr, texCoord);

    // Calculate background
    //vec3 back = calculate_color( color_bgr, rotate(vec2(texCoord.x, 1.0 - texCoord.y) / 1.5, (0.5 - sin(time / 100000)) ) );

    // Mix them
    //float mix_amount = 2.0 + BGR_FOCUS_BASE_WEAKNESS *  distance(texCoord, vec2(nsin(time / 100000), ncos(time / 12000))) ;

    //vec3 mixed = (front * mix_amount + back) / (mix_amount + 1.0);

    //gl_FragColor = vec4(mixed, 1.0);

    gl_FragColor = vec4(front, 1.0);
}

)"

