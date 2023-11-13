#type VERTEX_SHADER
#version 450 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in float a_size;
layout(location = 2) in float a_orientation;
layout(location = 3) in float a_linewidth;
layout(location = 4) in float a_magnitude;

const float M_SQRT2 = 1.4142135623730951;

out vec2 v_pos;
out float v_size;
out vec2 v_rotation;
out float v_linewidth;
out float v_magnitude;
out float v_skip_vertex;

uniform float u_antialias;
uniform float u_rot;

//
void main()
{
    v_pos = a_position;
    v_size = a_size;
    // v_rotation = vec2(cos(a_orientation), sin(a_orientation));
    v_rotation = vec2(cos(a_orientation + u_rot), sin(a_orientation + u_rot));
    v_linewidth = a_linewidth;
    v_magnitude = a_magnitude;

    v_skip_vertex = (a_magnitude == 0.0 ? 1.0 : 0.0);
    
    gl_Position = vec4(a_position, 0.0, 1.0);
    gl_PointSize = M_SQRT2 * a_size + 2.0 * (a_linewidth + 1.5 * u_antialias);

}


#type FRAGMENT_SHADER
#version 450 core

layout(location = 0) out vec4 out_color;

const float PI = 3.14159265358979323846264;
const float M_SQRT2 = 1.4142135623730951;

in vec2 v_pos;
in float v_size;
in vec2 v_rotation;
in float v_linewidth;
in float v_magnitude;
in float v_skip_vertex;

uniform float u_antialias;
uniform vec4 u_arrow_color = vec4(0.35, 0.95, 0.51, 1.0);

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

// Fill function for arrows
vec4 filled(float distance,     // Signed distance to line
            float linewidth,    // Stroke line width
            float antialias,    // Stroke antialiased area
            vec4 fill)          // Fill color
{
    float t = linewidth / 2.0 - antialias;
    float signed_distance = distance;
    float border_distance = abs(signed_distance) - t;
    float alpha = border_distance / antialias;
    alpha = exp(-alpha * alpha);
    if( border_distance < 0.0 )
        return fill;
    else if( signed_distance < 0.0 )
        return fill;
    else
        return vec4(fill.rgb, alpha * fill.a);
}

//
float line_distance(vec2 p, vec2 p1, vec2 p2)
{
    vec2 center = (p1 + p2) * 0.5;
    float len = length(p2 - p1);
    vec2 dir = (p2 - p1) / len;
    vec2 rel_p = p - center;
    return dot(rel_p, vec2(dir.y, -dir.x));
}

//
float segment_distance(vec2 p, vec2 p1, vec2 p2)
{
    vec2 center = (p1 + p2) * 0.5;
    float len = length(p2 - p1);
    vec2 dir = (p2 - p1) / len;
    vec2 rel_p = p - center;
    float dist1 = abs(dot(rel_p, vec2(dir.y, -dir.x)));
    float dist2 = abs(dot(rel_p, dir)) - 0.5*len;
    return max(dist1, dist2);
}

//
float arrow_triangle(vec2 texcoord, float body, float head, float height, 
                     float linewidth, float antialias)
{
    float w = linewidth/2.0 + antialias;
    vec2 start = -vec2(body/2.0, 0.0);
    vec2 end = vec2(body/2.0, 0.0);
    
    // Head : 3 lines
    float d1 = line_distance(texcoord,
    end, end - head*vec2(+1.0,-height));
    float d2 = line_distance(texcoord,
    end - head*vec2(+1.0,+height), end);
    float d3 = texcoord.x - end.x + head;
    
    // Body : 1 segment
    float d4 = segment_distance(texcoord,
    start, end - vec2(linewidth,0.0));
    float d = min(max(max(d1, d2), -d3), d4);
    return d;
}
//
void main()
{
    if (v_skip_vertex == 1.0)
        discard;

    vec2 p = gl_PointCoord.xy - vec2(0.5);

    p = vec2(v_rotation.x*p.x - v_rotation.y*p.y,
             v_rotation.y*p.x + v_rotation.x*p.y);

    // float d = line(p, vec2(0.0), vec2(1.0), 0.02);
    float d = arrow_triangle(p, 0.6, 0.1, 0.6, v_linewidth, u_antialias);

    // vec4 color = vec4(v_magnitude, u_arrow_color.gba);
    vec4 color = colormap(v_magnitude);
    out_color = filled(d, v_linewidth, u_antialias, color);
}


