#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Add these global variables at the top of your file:
float lastX = 400, lastY = 400;
float yaw = -90.0f; // Yaw is initialized to -90.0 degrees to look forward
float pitch = 0.0f;
bool firstMouse = true;
bool mouseCaptured = false;
float sensitivity = 0.1f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;


bool keyPressed = false;

GLfloat lineVertices[6]; // two 3D points (x, y, z)
bool firstClick = true;
bool drawLine = false;
int clickCount = 0;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 2.0f);   // Starting camera position
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // Direction looking forward
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);     // Up direction

glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));

float cameraSpeed = 0.05f; // Movement speed
bool isJumping = false;
float jumpVelocity = 0.0f;
float gravity = -9.81f; // realistic gravity, in units per second squared

// Converts screen coordinates to OpenGL NDC
glm::vec2 screenToOpenGLCoords(double x, double y, int width, int height) {
	float ndcX = (2.0f * x) / width - 1.0f;
	float ndcY = 1.0f - (2.0f * y) / height;
	return glm::vec2(ndcX, ndcY);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (!mouseCaptured)
		return;

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	// Clamp pitch
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	// Convert to direction vector
	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		int width, height;
		glfwGetCursorPos(window, &xpos, &ypos);
		glfwGetWindowSize(window, &width, &height);

		glm::vec2 pos = screenToOpenGLCoords(xpos, ypos, width, height);

		if (clickCount == 0) {
			lineVertices[0] = pos.x;
			lineVertices[1] = pos.y;
			lineVertices[2] = 0.0f;
			clickCount++;
		}
		else if (clickCount == 1) {
			lineVertices[3] = pos.x;
			lineVertices[4] = pos.y;
			lineVertices[5] = 0.0f;
			clickCount = 0;
			drawLine = true;
		}
	}
}

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"   gl_Position = projection * view * vec4(aPos, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"	FragColor = vec4(0.8f, 0.3f, 0.02f, 1.0);\n"
"}\n\0";

int main() {
	glfwInit();

	GLFWwindow* window = glfwCreateWindow(800, 800, "Learn OpenGL", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	gladLoadGL();
	glViewport(0, 0, 800, 800);

	// Compile shaders
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Square background
	GLfloat squareVertices[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,

		-1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f
	};

	// VAO & VBO for square
	GLuint squareVAO, squareVBO;
	glGenVertexArrays(1, &squareVAO);
	glGenBuffers(1, &squareVBO);
	glBindVertexArray(squareVAO);
	glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// VAO & VBO for dynamic line
	GLuint lineVAO, lineVBO;
	glGenVertexArrays(1, &lineVAO);
	glGenBuffers(1, &lineVBO);
	glBindVertexArray(lineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glfwSetCursorPosCallback(window, mouse_callback);


	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	float cameraSpeed = 0.2f * deltaTime; // 2.5 units per second


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	bool drawTriangle = true;
	bool wireframe = false;


	while (!glfwWindowShouldClose(window)) {

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !keyPressed) {
			wireframe = !wireframe;
			keyPressed = true;
			glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
		}
		if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
			keyPressed = false;
		}

		// Left click to toggle camera mouse capture
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !mouseCaptured) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			mouseCaptured = true;
			firstMouse = true;
		}
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && mouseCaptured) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			mouseCaptured = false;
		}

		horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));

		// Movement controls
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			cameraPos += cameraSpeed * horizontalFront;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			cameraPos -= cameraSpeed * horizontalFront;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			cameraPos -= glm::normalize(glm::cross(horizontalFront, cameraUp)) * cameraSpeed;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			cameraPos += glm::normalize(glm::cross(horizontalFront, cameraUp)) * cameraSpeed;

		// JUMPING
		// Handle jump input
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !isJumping && cameraPos.y <= 0.001f) {
			isJumping = true;
			jumpVelocity = 2.5f; // units per second
		}
		// Update jump physics
		if (isJumping) {
			jumpVelocity += gravity * deltaTime; // simulate gravity per frame
			cameraPos.y += jumpVelocity * deltaTime;

			if (cameraPos.y <= 0.0f) { // Hit the ground
				cameraPos.y = 0.0f;
				isJumping = false;
				jumpVelocity = 0.0f;
			}
		}

		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		glUseProgram(shaderProgram);

		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 800.0f, 0.1f, 100.0f);

		GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);

		GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

		// Draw background square
		glBindVertexArray(squareVAO);

		if (drawTriangle)
			glDrawArrays(GL_TRIANGLES, 0, 6);

		ImGui::Begin("ImGUI Ventana");
		ImGui::Text("Hola");
		ImGui::Checkbox("Dibujar Triangulo", &drawTriangle);
		ImGui::Checkbox("Modo Wireframe", &wireframe);
		glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Draw line if ready
		if (drawLine) {
			glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVertices), lineVertices);
			glBindVertexArray(lineVAO);
			glDrawArrays(GL_LINES, 0, 2);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}


	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Cleanup
	glDeleteVertexArrays(1, &squareVAO);
	glDeleteBuffers(1, &squareVBO);
	glDeleteVertexArrays(1, &lineVAO);
	glDeleteBuffers(1, &lineVBO);
	glDeleteProgram(shaderProgram);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
