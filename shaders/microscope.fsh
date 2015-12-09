R"(

varying vec2 texCoord;
uniform sampler2D texture0;
uniform float render_w;
uniform float render_h;
uniform float field_w;
uniform float field_h;

vec3 color_bgr = vec3(103.0/255.0,129.0/255.0,164.0/255.0);
vec3 color_dark = vec3(83.0/255.0,107.0/255.0,140.0/255.0);
vec3 color_bright = vec3(160.0 / 255.0,175.0 / 255.0,190.0 /255.0);
vec3 color_green = vec3(55.0 / 160.0,174.0 / 255.0,57.0 / 255.0);
vec3 color_green2 = vec3(28.0 / 255.0,89.0 / 255.0,29.0 / 255.0);

const float GREEN_THRESHOLD = 0.5;

vec2 rotate(vec2 p, float angle)
{
    mat2 M = mat2( cos( angle ), -sin( angle ),
                   sin( angle ),  cos( angle ));

    return M * p;
}

float filling(vec2 coord, float n)
{
    float N = 0;

    for(float i = coord.x - n; i < coord.x + n; ++i)
    {
        for(float j = coord.y - n; j < coord.y + n; ++j)
        {
            N += texture(texture0, vec2(i,j)).r;
        }
    }

    return N / pow(1.0 + 2.0*n, 2);
}

vec3 calculate_color(vec3 bgr, vec2 coord)
{
    float light_x = 1.0 / field_w;
    float light_y = 1.0 / field_h;

    float f = texture(texture0, coord).r;
    float f_light = texture(texture0, coord + vec2(light_x, light_y)).r;

    //Calculate base color
    vec3 color = mix(bgr, color_dark, f);

    // "Green" effect
    /*float green_n = filling(coord, 2.0);
    float f_green = clamp(green_n - GREEN_THRESHOLD, 0.0, 1.0) / (1.0 - GREEN_THRESHOLD);

    color = mix(color, color_green, f_green);*/

    // "3D" effect
    color += mix(vec3(0), color_bright, (f_light - f) * 0.2);

    return color;
}

void main(void)
{
    // Better background
    vec3 bgr = mix(color_bgr, color_bgr * 1.2, (1 - texCoord.y) * (1 - texCoord.x));

    //Calculate main
    vec3 front = calculate_color(bgr, texCoord);

    // Calculate background
    vec3 back = calculate_color( color_bgr, rotate(texCoord / 1.5, radians(60.0)) );

    // Mix them
    vec3 mixed = (front * 2.0 + back) / 3.0;

    gl_FragColor = vec4(mixed, 1.0);
}
)"

