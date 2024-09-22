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

std::vector<std::string> dayFaces
{
	"skybox/daytimesky/right.jpg",
	"skybox/daytimesky/left.jpg",
	"skybox/daytimesky/top.bmp",
	"skybox/daytimesky/bottom.jpg",
	"skybox/daytimesky/front.jpg",
	"skybox/daytimesky/back.jpg"
};

std::vector<std::string> nightFaces
{
	"skybox/nightsky/right.jpg",
	"skybox/nightsky/left.jpg",
	"skybox/nightsky/top.jpg",
	"skybox/nightsky/bottom.jpg",
	"skybox/nightsky/front.jpg",
	"skybox/nightsky/back.jpg"
};

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
	SkyboxRenderer skybox(dayFaces, nightFaces ,"skybox/sun.png", "skybox/moon.png");
	Crosshair crosshair;
	crosshair.initialize();
	BlockOutline blockOutline;

	Frustum frustum;
	World world(frustum);
	Player player(camera, world);
	glfwSetWindowUserPointer(window, &player);
	glfwSetMouseButtonCallback(window, main::mouseButtonCallback);

	main::setupRenderingState();

	main::initializeImGui(window);

	    // -- Main Game Loop -- //
	while (!glfwWindowShouldClose(window))
	{
		main::updateFPS();

		main::processInput(window);
		camera.update(deltaTime);
		skybox.updateSunAndMoonPosition(deltaTime);

		main::processRendering(window, mainShader, meshingShader, crosshairShader, skybox, player, frustum, world, crosshair, blockOutline);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cleanup
	main::cleanupImGui();
	main::cleanup(mainShader, meshingShader, crosshair);

	glfwTerminate();
	return 0;
}


void main::processRendering(GLFWwindow* window, shader& mainShader, shader& meshingShader, shader& crosshairShader, SkyboxRenderer& skybox, 
	Player& player, Frustum& frustum, World& world, Crosshair& crosshair, BlockOutline& blockOutline) 
{
	// Prepare matrices
	glm::mat4 view = camera.getViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(75.0f), (GLfloat)(SCR_WIDTH / (GLfloat)SCR_HEIGHT), 0.1f, 330.0f);
	glm::mat4 model = glm::mat4(1.0f);

	// Update Frustum and World State
	frustum.update(projection * view);
	glm::vec3 playerPosition = camera.getPosition();
	world.updatePlayerPosition(playerPosition);
	world.processChunkLoadQueue(1);

	// Clear Buffers
	glClearColor(0.4f, 0.6f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Light and Skybox Positions
	glm::vec3 sunPosition = skybox.getSunPosition();
	glm::vec3 moonPosition = skybox.getMoonPosition();
	glm::vec3 lightDirection = glm::normalize(-sunPosition);
	glm::vec3 moonDirection = glm::normalize(-moonPosition);

	// Set Shader Uniforms
	mainShader.use();
	mainShader.setMat4("model", model);
	mainShader.setMat4("view", view);
	mainShader.setMat4("projection", projection);

	// Sun
	mainShader.setVec3("lightDirection", lightDirection);
	mainShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.95f));
	mainShader.setVec3("viewPos", camera.getPosition());

	// Moon
	mainShader.setVec3("moonDirection", moonDirection);
	mainShader.setVec3("moonColor", glm::vec3(0.5f, 0.5f, 0.8f));

	// Fog
	mainShader.setVec4("fogColor", glm::vec4(0.5f, 0.6f, 0.7f, 1.0f));
	mainShader.setVec3("cameraPosition", camera.getPosition());

	world.Draw(frustum);

	// Draw Crosshair
	if (isCrosshairEnabled) crosshair.render(crosshairShader, crosshairColor, crosshairSize);

	// Render Block Outline
	main::renderBlockOutline(player, projection, view, blockOutline);

	// Outline Mesh
	if (isOutlineEnabled) main::initializeMeshOutline(meshingShader, model, view, projection, world, frustum);

	// Render Skybox
	main::renderSkybox(skybox, view, projection, camera);

	// ImGui
	if (isGUIEnabled) main::renderImGui(window, playerPosition, player, world, frustum, skybox);
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

void main::setupRenderingState() {
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);
	glEnable(GL_DEPTH_TEST);
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

void main::initializeMeshOutline(shader& meshingShader, glm::mat4 model, glm::mat4 view, glm::mat4 projection, World& world, Frustum& frustum)
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

	world.Draw(frustum);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset to fill mode
	glDisable(GL_POLYGON_OFFSET_LINE); // Disable polygon offset for lines
	glLineWidth(1.0f);
}

void main::renderSkybox(SkyboxRenderer& skybox, glm::mat4& view, const glm::mat4& projection, Camera& camera)
{
	view = glm::mat4(glm::mat3(camera.getViewMatrix()));
	skybox.renderSkybox(view, projection);
	skybox.renderSun(view, projection);
	skybox.renderMoon(view, projection);
}

void main::initializeImGui(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	ImGui::StyleColorsDark();
}

void main::renderImGui(GLFWwindow* window, const glm::vec3& playerPosition, Player& player, World& world, Frustum& frustum, SkyboxRenderer& skyboxRenderer) {
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
		"Dirt", "Stone", "Grass", "Sand", "Water", "Oak log", "Oak leaf",
	};

	GLint selectedBlockType = player.getSelectedBlockType();
	if (ImGui::Combo("Block Type", &selectedBlockType, blockTypeNames, IM_ARRAYSIZE(blockTypeNames))) {
		player.setSelectedBlockType(selectedBlockType);
	}

	// Sun speed adjustment
	ImGui::Separator();
	ImGui::Text("Sun Settings");

	static bool initialized = false;
	static GLfloat sunSpeed = 0.01f;
	if (!initialized) {
		sunSpeed = skyboxRenderer.getOrbitSpeed();
		initialized = true;
	}

	if (ImGui::SliderFloat("Sun Speed", &sunSpeed, 0.0f, 2.0f, "%.3f")) {
		skyboxRenderer.setOrbitSpeed(sunSpeed);
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

	//// Visualize world and frustum ////
	ImGui::Separator();
	if (ImGui::CollapsingHeader("World Overview", ImGuiTreeNodeFlags_DefaultOpen)) {
		// Enable scrolling within the ImGui window
		ImGui::BeginChild("CanvasRegion", ImVec2(300, 300), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();  // Size of the canvas available within the child window
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		// Draw the background
		drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(50, 50, 50, 255));

		// Scale factor for the world
		GLfloat scale = 0.52f;

		glm::vec3 camPos = camera.getPosition();

		// Center the camera on the canvas
		ImVec2 canvasCenter = ImVec2(canvasPos.x + canvasSize.x / 2, canvasPos.y + canvasSize.y / 2);

		// Draw chunks relative to the camera position, centering the camera on the canvas
		for (const auto& pair : world.getChunks()) {
			const Chunk* chunk = pair.second;

			// Calculate chunk top-down coordinates
			ImVec2 chunkPosMin = ImVec2(
				canvasCenter.x + (chunk->getMinBounds().x - camPos.x) * scale,
				canvasCenter.y + (chunk->getMinBounds().z - camPos.z) * scale
			);
			ImVec2 chunkPosMax = ImVec2(
				canvasCenter.x + (chunk->getMaxBounds().x - camPos.x) * scale,
				canvasCenter.y + (chunk->getMaxBounds().z - camPos.z) * scale
			);

			// Draw the chunk (use different colors for culled and visible chunks)
			bool inFrustum = chunk->isInFrustum(frustum);
			ImU32 fillColor = inFrustum ? IM_COL32(0, 255, 0, 150) : IM_COL32(255, 0, 0, 150);
			ImU32 outlineColor = IM_COL32(0, 0, 0, 255); // Black outline

			// Draw the filled chunk
			drawList->AddRectFilled(chunkPosMin, chunkPosMax, fillColor);

			// Draw the outline for the chunk (on top of the fill)
			drawList->AddRect(chunkPosMin, chunkPosMax, outlineColor, 0.0f, 0, 1.0f);
		}

		// Draw the camera position in the center of the canvas
		drawList->AddCircleFilled(canvasCenter, 5.0f, IM_COL32(255, 255, 255, 255));

		ImGui::EndChild();
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
