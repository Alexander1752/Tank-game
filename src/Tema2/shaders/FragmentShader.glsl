#version 330

// Input
// TODO(student): Get color value from vertex shader
in vec3 color;
in vec3 normal;
uniform int destruction;

uniform vec3 eye_position;
uniform float material_kd;
uniform float material_ks;
uniform int material_shininess;

// Output
layout(location = 0) out vec4 out_color;

struct light_source
{
   int  type;
   vec3 position;
   vec3 color;
   vec3 direction;
   float cutoff;
};

in vec3 world_position;
in vec3 N;
in vec3 V;

uniform light_source lights[10];
uniform int no_lights;

vec3 point_light_contribution(light_source light) {
	vec3 ret = light.color;

    vec3 L = normalize(light.position - world_position);
    vec3 H = normalize(L + V);

    float lum;
    if (dot(N, L) > 0)
        lum = 1;
    else lum = 0;

    ret *= (material_kd * max(dot(N, L), 0) + material_ks * lum * pow(max(dot(N, H), 0), material_shininess));

    if (light.type == 1) // spotlight
    {
        float spot_light = dot(-L, light.direction);
        float spot_light_limit = cos(light.cutoff);
        if (spot_light > spot_light_limit)
        {
	        float linear_att = (spot_light - spot_light_limit) / (1.0f - spot_light_limit);
            float light_att_factor = pow(linear_att, 2);
            ret *= light_att_factor;
        }
        else {
            ret = vec3(0);
        }
    }

    return ret;
}

void main()
{
    vec3 temp_color;

    if (destruction == 3)
        temp_color = vec3(.2);
    else
        temp_color = clamp(color * pow((normal.x + normal.y + normal.z) / 1.5, destruction * destruction), 0, 1);

    float ambient_light = 0.5;

    temp_color = temp_color * ambient_light;

    int i;
    for (i = 0; i < no_lights; i++)
        temp_color += point_light_contribution(lights[i]);

    if (destruction == 3)
        temp_color *= .1;

    out_color = vec4(temp_color, 1);
}
