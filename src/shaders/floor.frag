R"zzz(
#version 330 core
in vec4 face_normal;
in vec4 vertex_normal;
in vec4 light_direction;
in vec4 world_position;
in vec4 vertex_color;
out vec4 fragment_color;
void main() {
	vec4 pos = world_position;
	float check_width = 5.0;
	float i = floor(pos.x / check_width);
	float j  = floor(pos.z / check_width);
	vec3 color = vertex_color.xyz;
	float dot_nl = dot(normalize(light_direction), normalize(face_normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	color = clamp(dot_nl * color, 0.0, 1.0);
	fragment_color = vec4(color, 1.0);
}
)zzz"
