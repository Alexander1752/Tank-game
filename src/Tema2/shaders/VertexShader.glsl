#version 330

// Input
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texture_coord;
layout(location = 3) in vec3 v_color;

// Uniform properties
uniform vec3 eye_position;
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

uniform vec3 object_color;
uniform int destruction;

// Output value to fragment shader
out vec3 color;
out vec3 normal;
out vec3 world_position;
out vec3 N;
out vec3 V;

void main()
{
    vec4 v4 = vec4(v_position.x, v_position.y * (1 - .05 * destruction), v_position.z, 1);

    world_position = (Model * v4).xyz;
    N = normalize(mat3(Model) * v_normal);
    V = normalize(eye_position - world_position);

    v4 += vec4(v_normal * .1 * destruction, 0);

    gl_Position = Projection * View * Model * v4;
    color = object_color;
    normal = abs(v_normal);
}
