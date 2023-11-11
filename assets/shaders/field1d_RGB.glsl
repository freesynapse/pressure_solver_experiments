#type VERTEX_SHADER
#version 450 core

layout(location = 0) in vec2 a_position;
layout(location = 4) in vec2 a_uv;

out vec2 v_uv;

//
void main()
{
    v_uv = a_uv;
    gl_Position = vec4(a_position, 0.0, 1.0);

}


#type FRAGMENT_SHADER
#version 450 core

layout(location = 0) out vec4 out_color;

in vec2 v_uv;

uniform sampler2D u_sampler;

//
float colormap_red(float x)
{
    if (x < 0.7)
        return 4.0 * x - 1.5;
    else
        return -4.0 * x + 4.5;
}

//
float colormap_green(float x)
{
    if (x < 0.5)
        return 4.0 * x - 0.5;
    else
        return -4.0 * x + 3.5;
}

float colormap_blue(float x)
{
    if (x < 0.3)
       return 4.0 * x + 0.5;
    else
       return -4.0 * x + 2.5;
}

vec4 colormap(float x)
{
    float r = clamp(colormap_red(x), 0.0, 1.0);
    float g = clamp(colormap_green(x), 0.0, 1.0);
    float b = clamp(colormap_blue(x), 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}

//
void main()
{
    float color = texture(u_sampler, v_uv).r;
    out_color = colormap(color);
    
}


