#include "main.h"

constexpr GLuint SCR_WIDTH = 1280;
constexpr GLuint SCR_HEIGHT = 720;

GLfloat lastX = SCR_WIDTH / 2.0f;
GLfloat lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

Camera camera;

GLdouble lastTime = glfwGetTime();
uint8_t nbFrames = 0;
GLfloat fps = 0;

int main()
{
	GLFWwindow* window;
	main::initializeGLFW(window);
	main::initializeGLAD();

	glfwSetFramebufferSizeCallback(window, main::framebuffer_size_callback);
	glfwSetCursorPosCallback(window, main::mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	shader mainShader("shaders/main.vs", "shaders/main.fs");

	TextureManager textureManager;
	Chunk chunk(0, 0, textureManager);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window))
	{
		main::updateFPS();

		main::processInput(window);
		camera.update(deltaTime);

		glm::mat4 view = camera.getViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(75.0f), (GLfloat)(SCR_WIDTH / (GLfloat)SCR_HEIGHT), 0.1f, 235.0f);
		glm::mat4 model = glm::mat4(1.0f);

		glClearColor(0.4f, 0.6f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		mainShader.use();
		mainShader.setMat4("model", model);
		mainShader.setMat4("view", view);
		mainShader.setMat4("projection", projection);

		chunk.Draw();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	mainShader.Delete();

	glfwTerminate();
	return 0;
}

void main::initializeGLFW(GLFWwindow*& window)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "VoxelExplorer?", nullptr, nullptr);

	if (!window)
	{
		std::cerr << "Failed to create GLFW window!:(" << std::endl;
		glfwTerminate();
		exit(-1);
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, GLint width, GLint height) {
		glViewport(0, 0, width, height);
	});
}

void main::initializeGLAD()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD!" << std::endl;
		glfwTerminate();
		exit(-1);
	}
}

void main::framebuffer_size_callback(GLFWwindow* window, GLint width, GLint height)
{
	glViewport(0, 0, width, height);
}

void main::updateFPS() {
	GLfloat currentTime = static_cast<GLfloat>(glfwGetTime());
	nbFrames++;
	if (currentTime - lastTime >= 1.0) {
		fps = nbFrames;
		nbFrames = 0;
		lastTime += 1.0;
	}
	deltaTime = currentTime - lastFrame;
	lastFrame = currentTime;
}

void main::processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.setIsMovingForward(true);
	else
		camera.setIsMovingForward(false);

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.setIsMovingBackward(true);
	else
		camera.setIsMovingBackward(false);

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.setIsMovingLeft(true);
	else
		camera.setIsMovingLeft(false);

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.setIsMovingRight(true);
	else
		camera.setIsMovingRight(false);

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.setIsMovingUp(true);
	else
		camera.setIsMovingUp(false);

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera.setIsMovingDown(true);
	else
		camera.setIsMovingDown(false);
}

void main::mouse_callback(GLFWwindow* window, GLdouble xposIn, GLdouble yposIn)
{
	GLfloat xpos = static_cast<GLfloat>(xposIn);
	GLfloat ypos = static_cast<GLfloat>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.updateCameraOrientation(camera.getYaw() + xoffset * 0.1f, camera.getPitch() + yoffset * 0.1f);
}