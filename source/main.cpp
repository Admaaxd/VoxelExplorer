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

bool isGUIEnabled = false;
bool escapeKeyPressedLastFrame = false;
bool isOutlineEnabled = false;

bool isCrosshairEnabled = true;
glm::vec3 crosshairColor(1.0f, 1.0f, 1.0f);
GLfloat crosshairSize = 1.0f;

glm::vec3 hitPos, hitNormal;
GLint blockType;

std::vector<GLfloat> memoryUsageHistory;
constexpr int8_t MEMORY_HISTORY_SIZE = 100;

int main()
{
	GLFWwindow* window;
	main::initializeGLFW(window);
	main::initializeGLAD();

	glfwSetFramebufferSizeCallback(window, main::framebuffer_size_callback);
	glfwSetCursorPosCallback(window, main::mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	shader mainShader("shaders/main.vs", "shaders/main.fs");
	shader meshingShader("shaders/meshing.vs", "shaders/meshing.fs");
	shader crosshairShader("shaders/crosshair.vs", "shaders/crosshair.fs");
	Crosshair crosshair;
	crosshair.initialize();
	BlockOutline blockOutline;

	World world;
	Player player(camera, world);
	glfwSetWindowUserPointer(window, &player);
	glfwSetMouseButtonCallback(window, main::mouseButtonCallback);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST);

	main::initializeImGui(window);

	while (!glfwWindowShouldClose(window))
	{
		main::updateFPS();

		main::processInput(window);
		camera.update(deltaTime);

		glm::mat4 view = camera.getViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(75.0f), (GLfloat)(SCR_WIDTH / (GLfloat)SCR_HEIGHT), 0.1f, 330.0f);
		glm::mat4 model = glm::mat4(1.0f);

		glm::vec3 playerPosition = camera.getPosition();
		world.updatePlayerPosition(playerPosition);
		world.processChunkLoadQueue(2);

		glClearColor(0.4f, 0.6f, 0.8f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		mainShader.use();
		mainShader.setMat4("model", model);
		mainShader.setMat4("view", view);
		mainShader.setMat4("projection", projection);

		world.Draw();

		if (isCrosshairEnabled) crosshair.render(crosshairShader, crosshairColor, crosshairSize);

		main::renderBlockOutline(player, projection, view, blockOutline);
		
		if (isOutlineEnabled) main::initializeMeshOutline(meshingShader, model, view, projection, world);

		if (isGUIEnabled) main::renderImGui(window, playerPosition, player);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	main::cleanupImGui();
	main::cleanup(mainShader, meshingShader, crosshair);

	glfwTerminate();
	return 0;
}

void main::cleanup(shader& mainShader, shader& meshingShader, Crosshair& crosshair)
{
	mainShader.Delete();
	meshingShader.Delete();
	crosshair.Delete();
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

void main::renderBlockOutline(const Player& player, const glm::mat4& projection, const glm::mat4& view, BlockOutline& blockOutline) {
	glm::vec3 hitPos, hitNormal;
	GLint blockType;

	if (player.rayCast(hitPos, hitNormal, blockType)) {
		glm::mat4 model = glm::translate(glm::mat4(1.0f), hitPos);
		glm::mat4 MVP = projection * view * model;

		blockOutline.render(MVP);
	}
}

void main::initializeMeshOutline(shader& meshingShader, glm::mat4 model, glm::mat4 view, glm::mat4 projection, World& world)
{
	meshingShader.use();
	glm::mat4 outlineModel = glm::scale(model, glm::vec3(1.00f)); // Slightly scale up for outline effect
	meshingShader.setMat4("view", view);
	meshingShader.setMat4("projection", projection);
	meshingShader.setMat4("model", outlineModel);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Render in wireframe mode
	glLineWidth(3.0f); // Set outline width
	glEnable(GL_POLYGON_OFFSET_LINE); // Enable polygon offset for lines
	glPolygonOffset(-0.5, -0.5);

	world.Draw();

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset to fill mode
	glDisable(GL_POLYGON_OFFSET_LINE); // Disable polygon offset for lines
	glLineWidth(1.0f);
}

void main::initializeImGui(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	ImGui::StyleColorsDark();
}

void main::renderImGui(GLFWwindow* window, const glm::vec3& playerPosition, Player& player) {
	glDisable(GL_DEPTH_TEST);

	// Start ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Menu");

	ImGui::Text("FPS: %.1f", fps); // FPS counter

	ImGui::Text("Player Position: (%.2f, %.2f, %.2f)", playerPosition.x, playerPosition.y, playerPosition.z); // Player Position in the world

	ImGui::Separator();
	ImGui::Text("Select Block Type:");
	static const char* blockTypeNames[] = {
		"Dirt", "Stone", "Grass", "Sand", "Water",
	};

	GLint selectedBlockType = player.getSelectedBlockType();
	if (ImGui::Combo("Block Type", &selectedBlockType, blockTypeNames, IM_ARRAYSIZE(blockTypeNames))) {
		player.setSelectedBlockType(selectedBlockType);
	}

	ImGui::Separator();
	ImGui::Checkbox("Enable Mesh Outline", &isOutlineEnabled); // Checkbox for enabling/disabling outline

	// Crosshair customization
	ImGui::Separator();
	if (ImGui::CollapsingHeader("Crosshair settings", ImGuiTreeNodeFlags_None)) {
		ImGui::Checkbox("Enable Crosshair", &isCrosshairEnabled);
		ImGui::ColorEdit3("Crosshair Color", (GLfloat*)&crosshairColor); // Color picker for crosshair
		ImGui::SliderFloat("Crosshair Size", &crosshairSize, 0.1f, 3.0f); // Slider for crosshair size
	}

	//// Memory Usage Graph ////
	ImGui::Separator();
	if (ImGui::CollapsingHeader("Memory Usage", ImGuiTreeNodeFlags_DefaultOpen)) {
		// Get current memory usage in MB
		size_t memoryUsage = main::getCurrentMemoryUsage() / (1024 * 1024);

		// Add current memory usage to history
		if (memoryUsageHistory.size() >= MEMORY_HISTORY_SIZE) {
			memoryUsageHistory.erase(memoryUsageHistory.begin());
		}
		memoryUsageHistory.push_back(static_cast<GLfloat>(memoryUsage));

		// Display the memory usage graph
		ImGui::PlotLines("", memoryUsageHistory.data(), memoryUsageHistory.size(), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(0, 100));

		// Display current memory usage value
		ImGui::Text("Current Memory Usage: %zu MB", memoryUsage);
	}

	if (ImGui::Button("Exit Game")) glfwSetWindowShouldClose(window, true);  // Close the game

	ImGui::End();

	// Render ImGui
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glEnable(GL_DEPTH_TEST);
}

void main::cleanupImGui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void main::processInput(GLFWwindow* window)
{
	// Check for Escape key press
	bool isEscapePressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
	if (isEscapePressed && !escapeKeyPressedLastFrame) {
		// Toggle GUI visibility
		isGUIEnabled = !isGUIEnabled;

		// Enable or disable the cursor based on GUI state
		glfwSetInputMode(window, GLFW_CURSOR, isGUIEnabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

		if (!isGUIEnabled) {
			// Reset mouse position when GUI is disabled
			GLdouble xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			lastX = static_cast<GLfloat>(xpos);
			lastY = static_cast<GLfloat>(ypos);
		}
	}
	escapeKeyPressedLastFrame = isEscapePressed;

	if (!isGUIEnabled) {
		// Use a helper function to update movement states
		updateMovementState(window, GLFW_KEY_W, Direction::FORWARD);
		updateMovementState(window, GLFW_KEY_S, Direction::BACKWARD);
		updateMovementState(window, GLFW_KEY_A, Direction::LEFT);
		updateMovementState(window, GLFW_KEY_D, Direction::RIGHT);
		updateMovementState(window, GLFW_KEY_SPACE, Direction::UP);
		updateMovementState(window, GLFW_KEY_LEFT_SHIFT, Direction::DOWN);
	}
}

void main::updateMovementState(GLFWwindow* window, GLint key, Direction direction) {
	bool isKeyPressed = glfwGetKey(window, key) == GLFW_PRESS;
	camera.setMovementState(direction, isKeyPressed);
}

void main::mouse_callback(GLFWwindow* window, GLdouble xposIn, GLdouble yposIn)
{
	if (isGUIEnabled) {
		return;
	}

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

void main::mouseButtonCallback(GLFWwindow* window, GLint button, GLint action, GLint mods)
{
	Player* player = static_cast<Player*>(glfwGetWindowUserPointer(window));

	if (player) {
		player->handleMouseInput(button, action, isGUIEnabled);
	}
}

size_t main::getCurrentMemoryUsage()
{
	PROCESS_MEMORY_COUNTERS_EX pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
	return pmc.PrivateUsage;
}
