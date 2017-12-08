#include <GL/glew.h>
#include <dirent.h>

#include "procedure_geometry.h"
#include "render_pass.h"
#include "config.h"
#include "gui.h"
#include "perlin.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>
#include <debuggl.h>

using namespace std;

int window_width = 800, window_height = 600;
const std::string window_title = "Perlin";

const char* vertex_shader =
#include "shaders/default.vert"
;

const char* geometry_shader =
#include "shaders/default.geom"
;

const char* fragment_shader =
#include "shaders/default.frag"
;

const char* floor_fragment_shader =
#include "shaders/floor.frag"
;

// FIXME: Add more shaders here.

void ErrorCallback(int error, const char* description) {
	std::cerr << "GLFW Error: " << description << "\n";
}

// Height map variables
const int mapSizeX = 128;
const int mapSizeZ = 128;
const int mapSizeY = 128;
double heightScale = 3.0;
double noiseScale = 0.05;
double heightMap[mapSizeX][mapSizeZ][mapSizeY];

void generateHeightMap(int type, GUI& gui)
{
	bool octaveNoise = gui.useOctaves();
	int numOctaves = gui.numOctaves();
	double persistence = gui.getPersistence();
	// Perlin noise
	if(type == 1)
	{
		Perlin noise;
		for(int x = 0; x < mapSizeX; ++x)
		{
			for(int y = 0; y < mapSizeY; ++y)
			{
				for(int z = 0; z < mapSizeZ; ++z)
				{
					double n;
					if(octaveNoise)
						n = noise.OctavePerlin(noiseScale * x, noiseScale * y, noiseScale * z, numOctaves, persistence);
					else
						n = noise.perlin(noiseScale * x, noiseScale * y, noiseScale * z);
					heightMap[x][y][z] = heightScale * n;
				}
			}
		}
	}
	else if(type == 2)
	{
		Perlin noise;
		double xPeriod = 5.0;
		double yPeriod = 5.0;
		double power = gui.getSinPow();
		for(int x = 0; x < mapSizeX; ++x)
		{
			for(int y = 0; y < mapSizeY; ++y)
			{
				for(int z = 0; z < mapSizeZ; ++z)
				{
					double n;
					if(octaveNoise)
						n = noise.OctavePerlin(noiseScale * x, noiseScale * y, noiseScale * z, numOctaves, persistence);
					else
						n = noise.perlin(noiseScale * x, noiseScale * y, noiseScale * z);
					double val = x * xPeriod / mapSizeX + y * yPeriod / mapSizeY + power * n;
					heightMap[x][y][z] = fabs(sin(val * 3.14159));
				}
			}
		}
	}
	else if(type == 3)
	{
		Perlin noise;
		double period = 5.0;
		double power = gui.getRingPow();
		for(int x = 0; x < mapSizeX; ++x)
		{
			for(int y = 0; y < mapSizeY; ++y)
			{
				for(int z = 0; z < mapSizeZ; ++z)
				{
					double n;
					if(octaveNoise)
						n = noise.OctavePerlin(noiseScale * x, noiseScale * y, noiseScale * z, numOctaves, persistence);
					else
						n = noise.perlin(noiseScale * x, noiseScale * y, noiseScale * z);
					double xVal = (x - mapSizeX / 2) / (double)mapSizeX;
					double yVal = (y - mapSizeY / 2) / (double)mapSizeY;
					double dist = sqrt(xVal * xVal + yVal * yVal) + power * n;
					heightMap[x][y][z] = fabs(sin(2 * period * dist * 3.14159));
				}
			}
		}
	}
}

// Terrain variables
double minX = 0.0;
double maxX = 10.0;
double dX = (maxX - minX) / (mapSizeX - 1);
double minZ = 0.0;
double maxZ = 10.0;
double dZ = (maxZ - minZ) / (mapSizeZ - 1);

void generateTerrain(vector<glm::vec4>& vertices, vector<glm::uvec3>& indices, int level)
{
	for(int x = 0; x < mapSizeX; ++x)
	{
		for(int z = 0; z < mapSizeZ; ++z)
		{
			double posX = minX + x * dX;
			double posZ = minZ + z * dZ;
			double posY = heightMap[x][z][level];

			vertices.push_back(glm::vec4(posX, posY, posZ, 1.0));
		}
	}

	for(int x = 0; x < mapSizeX - 1; ++x)
	{
		for(int z = 0; z < mapSizeZ - 1; ++z)
		{
			int v1 = x + z * mapSizeX;
			int v2 = x + z * mapSizeX + 1;
			int v3 = x + (z + 1) * mapSizeX;
			int v4 = x + (z + 1) * mapSizeX + 1;

			indices.push_back(glm::uvec3(v1, v2, v3));
			indices.push_back(glm::uvec3(v4, v3, v2));
		}
	}
}

GLFWwindow* init_glefw()
{
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwSetErrorCallback(ErrorCallback);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	auto ret = glfwCreateWindow(window_width, window_height, window_title.data(), nullptr, nullptr);
	CHECK_SUCCESS(ret != nullptr);
	glfwMakeContextCurrent(ret);
	glewExperimental = GL_TRUE;
	CHECK_SUCCESS(glewInit() == GLEW_OK);
	glGetError();  // clear GLEW's error for it
	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

	return ret;
}

int main(int argc, char* argv[])
{
	// if (argc < 2) {
	// 	std::cerr << "Input model file is missing" << std::endl;
	// 	std::cerr << "Usage: " << argv[0] << " <PMD file>" << std::endl;
	// 	return -1;
	// }
	GLFWwindow *window = init_glefw();

	GUI gui(window);

	std::vector<glm::vec4> floor_vertices;
	std::vector<glm::uvec3> floor_faces;
	//create_floor(floor_vertices, floor_faces);

	// FIXME: add code to create terrain geometry
	vector<glm::vec4> terrain_vertices;
	vector<glm::uvec3> terrain_faces;
	int level = 1;

	generateHeightMap(1, gui);
	generateTerrain(floor_vertices, floor_faces, 0);

	glm::vec4 light_position = glm::vec4(5.0f, 10.0f, 5.0f, 1.0f);
	MatrixPointers mats; // Define MatrixPointers here for lambda to capture
	/*
	 * In the following we are going to define several lambda functions to bind Uniforms.
	 * 
	 * Introduction about lambda functions:
	 *      http://en.cppreference.com/w/cpp/language/lambda
	 *      http://www.stroustrup.com/C++11FAQ.html#lambda
	 */
	auto matrix_binder = [](int loc, const void* data) {
		glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat*)data);
	};
	auto vector_binder = [](int loc, const void* data) {
		glUniform4fv(loc, 1, (const GLfloat*)data);
	};
	auto vector3_binder = [](int loc, const void* data) {
		glUniform3fv(loc, 1, (const GLfloat*)data);
	};
	auto float_binder = [](int loc, const void* data) {
		glUniform1fv(loc, 1, (const GLfloat*)data);
	};
	/*
	 * These lambda functions below are used to retrieve data
	 */
	auto std_model_data = [&mats]() -> const void* {
		return mats.model;
	}; // This returns point to model matrix
	glm::mat4 floor_model_matrix = glm::mat4(1.0f);
	auto floor_model_data = [&floor_model_matrix]() -> const void* {
		return &floor_model_matrix[0][0];
	}; // This return model matrix for the floor.
	auto std_view_data = [&mats]() -> const void* {
		return mats.view;
	};
	auto std_camera_data  = [&gui]() -> const void* {
		return &gui.getCamera()[0];
	};
	auto std_proj_data = [&mats]() -> const void* {
		return mats.projection;
	};
	auto std_light_data = [&light_position]() -> const void* {
		return &light_position[0];
	};
	auto alpha_data  = [&gui]() -> const void* {
		static const float transparet = 0.5; // Alpha constant goes here
		static const float non_transparet = 1.0;
		if (gui.isTransparent())
			return &transparet;
		else
			return &non_transparet;
	};
	// FIXME: add more lambdas for data_source if you want to use RenderPass.
	//        Otherwise, do whatever you like here
	ShaderUniform std_model = { "model", matrix_binder, std_model_data };
	ShaderUniform floor_model = { "model", matrix_binder, floor_model_data };
	ShaderUniform std_view = { "view", matrix_binder, std_view_data };
	ShaderUniform std_camera = { "camera_position", vector3_binder, std_camera_data };
	ShaderUniform std_proj = { "projection", matrix_binder, std_proj_data };
	ShaderUniform std_light = { "light_position", vector_binder, std_light_data };
	ShaderUniform object_alpha = { "alpha", float_binder, alpha_data };
	// FIXME: define more ShaderUniforms for RenderPass if you want to use it.
	//        Otherwise, do whatever you like here

	//std::vector<glm::vec2>& uv_coordinates = mesh.uv_coordinates;
	// RenderDataInput object_pass_input;
	// object_pass_input.assign(0, "vertex_position", nullptr, mesh.vertices.size(), 4, GL_FLOAT);
	// object_pass_input.assign(1, "normal", mesh.vertex_normals.data(), mesh.vertex_normals.size(), 4, GL_FLOAT);
	// object_pass_input.assign(2, "uv", uv_coordinates.data(), uv_coordinates.size(), 2, GL_FLOAT);
	// object_pass_input.assign_index(mesh.faces.data(), mesh.faces.size(), 3);
	// object_pass_input.useMaterials(mesh.materials);
	// RenderPass object_pass(-1,
	// 		object_pass_input,
	// 		{
	// 		  vertex_shader,
	// 		  geometry_shader,
	// 		  fragment_shader
	// 		},
	// 		{ std_model, std_view, std_proj,
	// 		  std_light,
	// 		  std_camera, object_alpha },
	// 		{ "fragment_color" }
	// 		);

	// FIXME: Create the RenderPass objects for terrain here.
	//        Otherwise do whatever you like.
	RenderDataInput terrain_pass_input;
	terrain_pass_input.assign(0, "vertex_position", terrain_vertices.data(), terrain_vertices.size(), 4, GL_FLOAT);
	terrain_pass_input.assign_index(terrain_faces.data(), terrain_faces.size(), 3);
	RenderPass terrain_pass(-1,
			terrain_pass_input,
			{ vertex_shader, geometry_shader, fragment_shader },
			{ std_view, std_proj, std_light, std_camera },
			{ "fragment_color" }
			);

	RenderDataInput floor_pass_input;
	floor_pass_input.assign(0, "vertex_position", floor_vertices.data(), floor_vertices.size(), 4, GL_FLOAT);
	floor_pass_input.assign_index(floor_faces.data(), floor_faces.size(), 3);
	RenderPass floor_pass(-1,
			floor_pass_input,
			{ vertex_shader, geometry_shader, floor_fragment_shader},
			{ floor_model, std_view, std_proj, std_light },
			{ "fragment_color" }
			);
	float aspect = 0.0f;

	bool draw_floor = true;
	bool draw_object = true;

	while (!glfwWindowShouldClose(window)) {
		// Setup some basic window stuff.
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glViewport(0, 0, window_width, window_height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glCullFace(GL_BACK);

		gui.updateMatrices();
		mats = gui.getMatrixPointers();
		bool advanceFrame = gui.advanceFrame();
		bool dirty = gui.isDirty();

		if(dirty)
		{
			gui.setClean();
			gui.stopAdvance();

			generateHeightMap(gui.getMapType(), gui);

			floor_vertices.clear();
			floor_faces.clear();

			generateTerrain(floor_vertices, floor_faces, 0);
			floor_pass.updateVBO(0, floor_vertices.data(), floor_vertices.size());
			floor_pass = RenderPass(floor_pass.getVAO(),
					floor_pass_input,
					{ vertex_shader, geometry_shader, floor_fragment_shader },
					{ floor_model, std_view, std_proj, std_light },
					{ "fragment_color "}
					);
		}

		if(advanceFrame)
		{
			if(level > 63)
				level = 0;

			floor_vertices.clear();
			floor_faces.clear();

			generateTerrain(floor_vertices, floor_faces, level);
			floor_pass.updateVBO(0, floor_vertices.data(), floor_vertices.size());
			floor_pass = RenderPass(floor_pass.getVAO(),
					floor_pass_input,
					{ vertex_shader, geometry_shader, floor_fragment_shader },
					{ floor_model, std_view, std_proj, std_light },
					{ "fragment_color "}
					);

		}
		floor_pass.setup();
		// Draw our triangles.
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));
		++level;
		if (draw_object) {
				// mesh.updateAnimation();
				// object_pass.updateVBO(0,
						      //mesh.animated_vertices.data(),
							  //mesh.animated_vertices.size());

			
			// object_pass.setup();
			// int mid = 0;
			// while (object_pass.renderWithMaterial(mid))
			// 	mid++;
		}
		// Poll and swap.
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}
