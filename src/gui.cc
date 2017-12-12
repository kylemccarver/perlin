#include "gui.h"
#include "config.h"
//#include <jpegio.h>
#include <iostream>
#include <debuggl.h>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

GUI::GUI(GLFWwindow* window)
	:window_(window)
{
	glfwSetWindowUserPointer(window_, this);
	glfwSetKeyCallback(window_, KeyCallback);
	glfwSetCursorPosCallback(window_, MousePosCallback);
	glfwSetMouseButtonCallback(window_, MouseButtonCallback);

	glfwGetWindowSize(window_, &window_width_, &window_height_);
	float aspect_ = static_cast<float>(window_width_) / window_height_;
	projection_matrix_ = glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
}

GUI::~GUI()
{
}

void GUI::keyCallback(int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window_, GL_TRUE);
		return ;
	}

	if (captureWASDUPDOWN(key, action))
		return ;
	else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
		fps_mode_ = !fps_mode_;
	} else if (key == GLFW_KEY_T && action != GLFW_RELEASE) {
		transparent_ = !transparent_;
	}
	else if (key == GLFW_KEY_SPACE && action != GLFW_RELEASE) {
		advance = !advance;
	}
	else if(key == GLFW_KEY_R && action != GLFW_RELEASE)
	{
		this->reset();
	}
	else if(key == GLFW_KEY_O && action != GLFW_RELEASE)
	{
		toggleOctave = !toggleOctave;
		this->reset();
	}
	else if(key == GLFW_KEY_LEFT_BRACKET && action != GLFW_RELEASE)
	{
		if(toggleOctave)
		{
			if(octaves > 1)
				--octaves;
			dirty = true;
			advance = false;
			displayValues();
		}
	}
	else if(key == GLFW_KEY_RIGHT_BRACKET && action != GLFW_RELEASE)
	{
		if(toggleOctave)
		{
			++octaves;
			dirty = true;
			advance = false;
			displayValues();
		}
	}
	else if(key == GLFW_KEY_SEMICOLON && action != GLFW_RELEASE)
	{
		if(toggleOctave)
		{
			if(persistence > 0.1)
				persistence -= 0.1;
			dirty = true;
			advance = false;
			displayValues();
		}
	}
	else if(key == GLFW_KEY_APOSTROPHE && action != GLFW_RELEASE)
	{
		if(toggleOctave)
		{
			persistence += 0.1;
			dirty = true;
			advance = false;
			displayValues();
		}
	}
	else if(key == GLFW_KEY_COMMA && action != GLFW_RELEASE)
	{
		if(mapType == 2 && sinPow >= 1.0)
			sinPow -= 1.0;
		else if(mapType == 3 && ringPow >= 0.1)
			ringPow -= 0.1;
		dirty = true;
		advance = false;
		displayValues();
	}
	else if(key == GLFW_KEY_PERIOD && action != GLFW_RELEASE)
	{
		if(mapType == 2)
			sinPow += 1.0;
		else if(mapType == 3)
			ringPow += 0.1;
		dirty = true;
		advance = false;
		displayValues();
	}
	else if(key == GLFW_KEY_LEFT && action != GLFW_RELEASE)
	{
		if(height > 0.0)
		{
			height -= 1.0;
			dirty = true;
			advance = false;
			displayValues();
		}
	}
	else if(key == GLFW_KEY_RIGHT && action != GLFW_RELEASE)
	{
		height += 1.0;
		dirty = true;
		advance = false;
		displayValues();
	}
	else if(key == GLFW_KEY_1 && action != GLFW_RELEASE)
	{
		if(mapType != 1)
		{
			mapType = 1;
			toggleOctave = false;
			this->reset();
		}
	}
	else if(key == GLFW_KEY_2 && action != GLFW_RELEASE)
	{
		if(mapType != 2)
		{
			mapType = 2;
			toggleOctave = false;
			this->reset();
		}
	}
	else if(key == GLFW_KEY_3 && action != GLFW_RELEASE)
	{
		if(mapType != 3)
		{
			mapType = 3;
			toggleOctave = false;
			this->reset();
		}
	}
}

void GUI::reset()
{
	octaves = 1;
	height = 1.0;
	persistence = 0.1;
	sinPow = 0.0;
	ringPow = 0.0;
	dirty = true;
	advance = false;
	displayValues();
}

void GUI::displayValues()
{
	std::cout << ">>>>>>>>>>>>>>>>\n";
	std::cout << "Using octaves: " << toggleOctave << "\n";
	std::cout << "Height: " << height << "\n";
	std::cout << "Octaves: " << octaves << "\n";
	std::cout << "Persistence: " << persistence << "\n";
	std::cout << "Power: ";
	if(mapType == 2)
		std::cout << sinPow << "\n";
	else
		std::cout << ringPow << "\n";
	std::cout << ">>>>>>>>>>>>>>>>" << std::endl;
}

void GUI::mousePosCallback(double mouse_x, double mouse_y)
{
	last_x_ = current_x_;
	last_y_ = current_y_;
	current_x_ = mouse_x;
	current_y_ = window_height_ - mouse_y;
	float delta_x = current_x_ - last_x_;
	float delta_y = current_y_ - last_y_;
	if (sqrt(delta_x * delta_x + delta_y * delta_y) < 1e-15)
		return;
	glm::vec3 mouse_direction = glm::normalize(glm::vec3(delta_x, delta_y, 0.0f));
	glm::vec2 mouse_start = glm::vec2(last_x_, last_y_);
	glm::vec2 mouse_end = glm::vec2(current_x_, current_y_);
	glm::uvec4 viewport = glm::uvec4(0, 0, window_width_, window_height_);

	bool drag_camera = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_RIGHT;

	if (drag_camera) {
		glm::vec3 axis = glm::normalize(
				orientation_ *
				glm::vec3(mouse_direction.y, -mouse_direction.x, 0.0f)
				);
		orientation_ =
			glm::mat3(glm::rotate(rotation_speed_, axis) * glm::mat4(orientation_));
		tangent_ = glm::column(orientation_, 0);
		up_ = glm::column(orientation_, 1);
		look_ = glm::column(orientation_, 2);
	}
}

void GUI::mouseButtonCallback(int button, int action, int mods)
{
	drag_state_ = (action == GLFW_PRESS);
	current_button_ = button;
}

void GUI::updateMatrices()
{
	// Compute our view, and projection matrices.
	if (fps_mode_)
		center_ = eye_ + camera_distance_ * look_;
	else
		eye_ = center_ - camera_distance_ * look_;

	view_matrix_ = glm::lookAt(eye_, center_, up_);
	light_position_ = glm::vec4(eye_, 1.0f);

	aspect_ = static_cast<float>(window_width_) / window_height_;
	projection_matrix_ =
		glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
	model_matrix_ = glm::mat4(1.0f);
}

MatrixPointers GUI::getMatrixPointers() const
{
	MatrixPointers ret;
	ret.projection = &projection_matrix_[0][0];
	ret.model= &model_matrix_[0][0];
	ret.view = &view_matrix_[0][0];
	return ret;
}

bool GUI::captureWASDUPDOWN(int key, int action)
{
	if (key == GLFW_KEY_W) {
		if (fps_mode_)
			eye_ += zoom_speed_ * look_;
		else
			camera_distance_ -= zoom_speed_;
		return true;
	} else if (key == GLFW_KEY_S) {
		if (fps_mode_)
			eye_ -= zoom_speed_ * look_;
		else
			camera_distance_ += zoom_speed_;
		return true;
	} else if (key == GLFW_KEY_A) {
		if (fps_mode_)
			eye_ -= pan_speed_ * tangent_;
		else
			center_ -= pan_speed_ * tangent_;
		return true;
	} else if (key == GLFW_KEY_D) {
		if (fps_mode_)
			eye_ += pan_speed_ * tangent_;
		else
			center_ += pan_speed_ * tangent_;
		return true;
	} else if (key == GLFW_KEY_DOWN) {
		if (fps_mode_)
			eye_ -= pan_speed_ * up_;
		else
			center_ -= pan_speed_ * up_;
		return true;
	} else if (key == GLFW_KEY_UP) {
		if (fps_mode_)
			eye_ += pan_speed_ * up_;
		else
			center_ += pan_speed_ * up_;
		return true;
	}
	return false;
}


// Delegrate to the actual GUI object.
void GUI::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->keyCallback(key, scancode, action, mods);
}

void GUI::MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mousePosCallback(mouse_x, mouse_y);
}

void GUI::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mouseButtonCallback(button, action, mods);
}
