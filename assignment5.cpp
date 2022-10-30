#include "assignment5.hpp"
#include "parametric_shapes.hpp"
#include "CelestialBody.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/ShaderProgramManager.hpp"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment5::~Assignment5()
{
	bonobo::deinit();
}

bool
edaf80::Assignment5::testSphereSphere(float p1, float r1, float p2, float r2) {
	if (abs(p1 - p2) < r1 + r2) {
		return true;
	}
	return false;
}

void
edaf80::Assignment5::run()
{
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 4.0f, 20.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h

	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram("Fallback",
	                                         { { ShaderType::vertex, "common/fallback.vert" },
	                                           { ShaderType::fragment, "common/fallback.frag" } },
	                                         fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

	//
	// Todo: Insert the creation of other shader programs.
	//       (Check how it was done in assignment 3.)
	//

	GLuint player_shader = 0u;
	program_manager.CreateAndRegisterProgram("Player",
		{ { ShaderType::vertex, "EDAF80/default.vert" },
		  { ShaderType::fragment, "EDAF80/default.frag" } },
		player_shader);
	if (player_shader == 0u)
		LogError("Failed to load player shader");

	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram("Skybox",
		{ { ShaderType::vertex, "EDAF80/skybox.vert" },
		  { ShaderType::fragment, "EDAF80/skybox.frag" } },
		skybox_shader);
	if (skybox_shader == 0u)
		LogError("Failed to load skybox shader");

	//
	// Todo: Load your geometry
	//
	const float playerRadius = 2.0;
	const float asteroidRadius = 3.0;

	auto playerSphere = parametric_shapes::createSphere(playerRadius, 100u, 100u);
	if (playerSphere.vao == 0u) {
		LogError("Failed to retrieve the mesh for the player");
		return;
	}

	auto skyboxSphere = parametric_shapes::createSphere(100.0f, 100u, 100u);
	if (skyboxSphere.vao == 0u) {
		LogError("Failed to retrieve the mesh for the skybox");
		return;
	}

	auto asteroidSphere = parametric_shapes::createSphere(asteroidRadius, 100u, 100u);
	if (asteroidSphere.vao == 0u) {
		LogError("Failed to retrieve the mesh for the skybox");
		return;
	}

	auto camera_position = mCamera.mWorld.GetTranslation();

	GLuint playerTexture = bonobo::loadTexture2D(config::resources_path("cubemaps/Gru/posx.jpg"));
	GLuint asteroidTexture = bonobo::loadTexture2D(config::resources_path("cubemaps/Gru/minion.jpg"));
	GLuint moonTexture = bonobo::loadTexture2D(config::resources_path("planets/2k_moon.jpg"));

	GLuint skyboxTexture = bonobo::loadTextureCubeMap(
		config::resources_path("cubemaps/Space/posx.jpg"),
		config::resources_path("cubemaps/Space/negx.jpg"),
		config::resources_path("cubemaps/Space/posy.jpg"),
		config::resources_path("cubemaps/Space/negy.jpg"),
		config::resources_path("cubemaps/Space/posz.jpg"),
		config::resources_path("cubemaps/Space/negz.jpg"));

	Node player;
	player.set_geometry(playerSphere);
	player.set_program(&player_shader);
	player.add_texture("diffuse_texture", playerTexture, GL_TEXTURE_2D);
	player.get_transform().SetRotate(135, glm::vec3(0, 1, 0));

	Node skybox;
	skybox.set_geometry(skyboxSphere);
	skybox.set_program(&skybox_shader);
	skybox.add_texture("cubemap", skyboxTexture, GL_TEXTURE_CUBE_MAP);

	const int NUM_ASTEROIDS = 30;
	const int NUM_MOONS = 10;

	Node asteroids[NUM_ASTEROIDS];
	glm::vec3 velocities[NUM_ASTEROIDS];
	float rotationSpeed[NUM_ASTEROIDS];
	float rotationAngle[NUM_ASTEROIDS];

	Node moons[NUM_MOONS];
	glm::vec3 velocitiesMoons[NUM_MOONS];

	for (int i = 0; i < NUM_ASTEROIDS; i++) {
		asteroids[i].set_geometry(asteroidSphere);
		asteroids[i].set_program(&player_shader);
		asteroids[i].add_texture("diffuse_texture", asteroidTexture, GL_TEXTURE_2D);
		int randX = rand() % 40 - 20;
		int randY = rand() % 40 - 20;
		asteroids[i].get_transform().SetTranslate(glm::vec3(randX, randY, -150));
		asteroids[i].get_transform().SetRotate(135, glm::vec3(0, 1, 0));

		velocities[i] = glm::vec3(0, 0, rand() % 10 / 5.0f + 1);

		rotationSpeed[i] = (rand() % 2) / 4 - 0.125;
		rotationAngle[i] = 135;
	}

	for (int i = 0; i < NUM_MOONS; i++) {
		moons[i].set_geometry(playerSphere);
		moons[i].set_program(&player_shader);
		moons[i].add_texture("diffuse_texture", moonTexture, GL_TEXTURE_2D);
		int randX = rand() % 80 - 40;
		int randY = rand() % 80 - 40;
		moons[i].get_transform().SetTranslate(glm::vec3(randX, randY, -150));
		moons[i].get_transform().SetRotate(135, glm::vec3(0, 1, 0));

		velocitiesMoons[i] = glm::vec3(0, 0, rand() % 10 / 10.0f + 1);
	}

	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);


	auto lastTime = std::chrono::high_resolution_clock::now();

	bool use_orbit_camera = false;
	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;
	int numCoins = 0;
	float moonRotation = 0;

	bool player_dead = false;

	while (!glfwWindowShouldClose(window)) {
		moonRotation = fmod(moonRotation + 0.1, 360.0);
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		camera_position = mCamera.mWorld.GetTranslation();

		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);
		glm::vec3 y_shift = glm::vec3(0, 0.5, 0);
		glm::vec3 x_shift = glm::vec3(0.5, 0, 0);
		if (inputHandler.GetKeycodeState(GLFW_KEY_W) & PRESSED && (player.get_transform().GetTranslation().y <= 40)) {
			camera_position = camera_position + y_shift;
			player.get_transform().SetTranslate(player.get_transform().GetTranslation() + y_shift);
			mCamera.mWorld.SetTranslate(camera_position);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_A) & PRESSED && (player.get_transform().GetTranslation().x >= -40)) {
			camera_position = camera_position - x_shift;
			player.get_transform().SetTranslate(player.get_transform().GetTranslation() - x_shift);
			mCamera.mWorld.SetTranslate(camera_position);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_S) & PRESSED && (player.get_transform().GetTranslation().y >= -40)) {
			camera_position = camera_position - y_shift;
			player.get_transform().SetTranslate(player.get_transform().GetTranslation() - y_shift);
			mCamera.mWorld.SetTranslate(camera_position);
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_D) & PRESSED && (player.get_transform().GetTranslation().x <= 40)) {
			camera_position = camera_position + x_shift;
			player.get_transform().SetTranslate(player.get_transform().GetTranslation() + x_shift);
			mCamera.mWorld.SetTranslate(camera_position);
		}

		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED && player_dead) {
			for (int i = 0; i < NUM_ASTEROIDS; i++) {
				asteroids[i].set_geometry(asteroidSphere);
				asteroids[i].set_program(&player_shader);
				asteroids[i].add_texture("diffuse_texture", asteroidTexture, GL_TEXTURE_2D);
				int randX = rand() % 40 - 20;
				int randY = rand() % 40 - 20;
				asteroids[i].get_transform().SetTranslate(glm::vec3(randX, randY, -150));
				asteroids[i].get_transform().SetRotate(135, glm::vec3(0, 1, 0));

				velocities[i] = glm::vec3(0, 0, rand() % 10 / 5.0f + 1);

				rotationSpeed[i] = (rand() % 2) / 4 - 0.125;
				rotationAngle[i] = 135;
			}

			for (int i = 0; i < NUM_MOONS; i++) {
				moons[i].set_geometry(playerSphere);
				moons[i].set_program(&player_shader);
				moons[i].add_texture("diffuse_texture", moonTexture, GL_TEXTURE_2D);
				int randX = rand() % 80 - 40;
				int randY = rand() % 80 - 40;
				moons[i].get_transform().SetTranslate(glm::vec3(randX, randY, -150));
				moons[i].get_transform().SetRotate(135, glm::vec3(0, 1, 0));

				velocitiesMoons[i] = glm::vec3(0, 0, rand() % 10 / 10.0f + 1);
			}
			mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 4.0f, 20.0f));
			player.get_transform().SetTranslate(glm::vec3(0));
			player_dead = false;
			numCoins = 0;
		}

		mWindowManager.NewImGuiFrame();
		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImGuiWindowFlags_None);
		if (opened) {
			ImGui::Checkbox("Use orbit camera", &use_orbit_camera);
			ImGui::Checkbox("Show basis", &show_basis);
			ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
			ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
		}
		ImGui::End();

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if (!player_dead) {
			if (!shader_reload_failed) {
				//
				// Todo: Render all your geometry here.
				//
				skybox.render(mCamera.GetWorldToClipMatrix());
				player.render(mCamera.GetWorldToClipMatrix());
				/*float dt = std::chrono::duration<float>(deltaTimeUs).count();
				float rand_acc = rand() / RAND_MAX + 1;
				glm::vec3 acc = glm::vec3(0, 0, rand_acc);
				vel += acc * dt;*/

				for (int i = 0; i < NUM_ASTEROIDS; i++) {
					glm::vec3 currAsteroidPos = asteroids[i].get_transform().GetTranslation();
					glm::vec3 currPlayerPos = player.get_transform().GetTranslation();
					if (asteroids[i].get_transform().GetTranslation().z > 10) {
						velocities[i] = glm::vec3(0, 0, rand() % 10 / 5.0 + 1);
						rotationSpeed[i] = (rand() % 2) / 4 - 0.125;
						rotationAngle[i] = 135;
						int randX = rand() % 80 - 40;
						int randY = rand() % 80 - 40;
						asteroids[i].get_transform().SetTranslate(glm::vec3(randX, randY, -150));
						asteroids[i].get_transform().SetRotate(135, glm::vec3(0, 1, 0));
					}
					rotationAngle[i] = rotationAngle[i] + rotationSpeed[i];
					asteroids[i].render(mCamera.GetWorldToClipMatrix());
					asteroids[i].get_transform().SetTranslate(asteroids[i].get_transform().GetTranslation() + velocities[i]);
					asteroids[i].get_transform().SetRotate(rotationAngle[i], glm::vec3(0, 1, 0));

					if (testSphereSphere(currPlayerPos.x, playerRadius, currAsteroidPos.x, asteroidRadius) &&
						testSphereSphere(currPlayerPos.y, playerRadius, currAsteroidPos.y, asteroidRadius) &&
						testSphereSphere(currPlayerPos.z, playerRadius, currAsteroidPos.z, asteroidRadius)) {
						asteroids[i].get_transform().SetTranslate(glm::vec3(0, 0, 100));
						std::cout << "You got minion'd!\n";
						std::cout << "Moons stolen: " << numCoins << "\n\n";
						player_dead = true;
					}
				}

				for (int i = 0; i < NUM_MOONS; i++) {
					glm::vec3 currMoonPos = moons[i].get_transform().GetTranslation();
					glm::vec3 currPlayerPos = player.get_transform().GetTranslation();
					if (moons[i].get_transform().GetTranslation().z > 10) {
						velocitiesMoons[i] = glm::vec3(0, 0, rand() % 10 / 10.0 + 1);
						int randX = rand() % 80 - 40;
						int randY = rand() % 80 - 40;
						moons[i].get_transform().SetTranslate(glm::vec3(randX, randY, -150));
					}
					moons[i].get_transform().SetRotate(moonRotation, glm::vec3(0, 1, 0));
					moons[i].render(mCamera.GetWorldToClipMatrix());
					moons[i].get_transform().SetTranslate(moons[i].get_transform().GetTranslation() + velocitiesMoons[i]);

					if (testSphereSphere(currPlayerPos.x, playerRadius, currMoonPos.x, playerRadius) &&
						testSphereSphere(currPlayerPos.y, playerRadius, currMoonPos.y, playerRadius) &&
						testSphereSphere(currPlayerPos.z, playerRadius, currMoonPos.z, playerRadius)) {
						moons[i].get_transform().SetTranslate(glm::vec3(0, 0, 100));
						std::cout << "+1 moon stolen!\n";
						numCoins++;
					}
				}
			}
			

			glfwSwapBuffers(window);
		}
		
	}
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try {
		edaf80::Assignment5 assignment5(framework.GetWindowManager());
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
