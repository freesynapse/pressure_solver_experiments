#type VERTEX_SHADER
#version 450 core

layout(location = 0) in vec2 a_position;

//
void main()
{
    gl_Position = vec4(a_position, 0.0, 1.0);

}


#type FRAGMENT_SHADER
#version 450 core

layout(location = 0) out vec4 out_color;

//
void main()
{
    out_color = vec4(1.0);
    
}


