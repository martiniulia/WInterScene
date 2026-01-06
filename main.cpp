#if defined (_APPLE_)
#define GLFW_INCLUDE_GLCOREARB
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <iostream>

#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"

// Dimensiuni fereastra
int glWindowWidth = 1024;
int glWindowHeight = 768;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

// Matrici si locatii
glm::mat4 model;
GLint modelLoc;
glm::mat4 view;
GLint viewLoc;
glm::mat4 projection;
GLint projLoc;

bool firstMouse = true;
float lastX = 512.0f;
float lastY = 384.0f;

float yaw = -90.0f;
float pitch = 0.0f;
float mouseSensitivity = 0.1f;

// Camera (Laboratorul 5)
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 0.0f)
);

glm::vec3 spotLightPos(2.7f, 3.5f, 0.13f);   // sus, lângă felinar
glm::vec3 spotLightDir(0.0f, -1.0f, 0.0f);  // în jos

// Coordonate felinare din Blender
std::vector<glm::vec3> pointLightPositions = {
    {2.22f, 0.42f, 2.72f},   // felinar 1
    {2.50f, 0.42f, 2.66f},   // felinar 2
    {2.43f, 0.10f, 3.57f},   // felinar 3
    {2.71f, 0.11f, 3.57f},   // felinar 4
    {2.00f, 0.10f, 1.88f},   // felinar 5
    {2.28f, 0.10f, 1.81f}    // felinar 6
};

// Culori felinare (cald, portocaliu/galben)
std::vector<glm::vec3> pointLightColors = {
    {1.5f, 1.2f, 0.8f},  // felinar 1 - mai puternic
    {1.5f, 1.2f, 0.8f},  // felinar 2
    {1.5f, 1.2f, 0.8f},  // felinar 3
    {1.5f, 1.2f, 0.8f},  // felinar 4
    {1.5f, 1.2f, 0.8f},  // felinar 5
    {1.5f, 1.2f, 0.8f}   // felinar 6
};


float cameraSpeed = 3.0f;

// Input
bool pressedKeys[1024];
float angle = 0.0f;

// View modes
int viewMode = 0; // 0 = FILL, 1 = WIREFRAME, 2 = SMOOTH

// Object transformations
float objectScale = 1.0f;
glm::vec3 objectTranslate(0.0f, 0.0f, 0.0f);
float objectRotation = 0.0f;

// Animation
float animationTime = 0.0f;
float doorRotation = 0.0f; // Pentru animație ușă (exemplu)

// Light rotation
float lightAngle = 0.0f;

enum ViewMode {
    VIEW_SOLID = 0,
    VIEW_WIREFRAME,
    VIEW_POLYGONAL,
    VIEW_SMOOTH
};

ViewMode currentViewMode = VIEW_SOLID;

// Resurse (Laboratorul 8)
gps::Model3D myModel;
gps::Model3D lightCube;
gps::Shader myCustomShader;
gps::Shader depthMapShader;
gps::Shader lightCubeShader;
gps::Shader lightGlowShader;

// Shadow mapping
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;
GLuint shadowMapFBO;
GLuint depthMapTexture;
glm::mat4 lightSpaceTrMatrix;

// Fog parameters
float fogDensity = 0.035f; // Mărit pentru vizibilitate mai bună
glm::vec3 fogColor(0.75f, 0.8f, 0.85f); // Culoare ceață ușoară

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
    glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);
    myCustomShader.useShaderProgram();
    projection = glm::perspective(glm::radians(55.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // inversat (ecran vs OpenGL)

    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // limitare pitch (fara flip)
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    myCamera.rotate(pitch, yaw);
}

void processMovement(float deltaTime) {
    float speed = cameraSpeed * deltaTime;

    // View modes: V = SOLID, M = WIREFRAME, N = POLYGONAL, B = SMOOTH
    if (pressedKeys[GLFW_KEY_V]) {
        currentViewMode = VIEW_SOLID;
    }
    if (pressedKeys[GLFW_KEY_M]) {
        currentViewMode = VIEW_WIREFRAME;
    }
    if (pressedKeys[GLFW_KEY_N]) {
        currentViewMode = VIEW_POLYGONAL;
    }
    if (pressedKeys[GLFW_KEY_B]) {
        currentViewMode = VIEW_SMOOTH;
    }

    // Apply current view mode
    switch (currentViewMode) {
        case VIEW_SOLID:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_PROGRAM_POINT_SIZE);
            break;
        case VIEW_WIREFRAME:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        case VIEW_POLYGONAL:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            glEnable(GL_PROGRAM_POINT_SIZE);
            break;
        case VIEW_SMOOTH:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
    }

    // Scene rotation
    if (pressedKeys[GLFW_KEY_Q]) {
        angle += 1.0f;
    }
    if (pressedKeys[GLFW_KEY_E]) {
        angle -= 1.0f;
    }

    // Object transformations (exemplu - poți ajusta tastele)
    if (pressedKeys[GLFW_KEY_R]) {
        objectRotation += 1.0f;
    }
    if (pressedKeys[GLFW_KEY_T]) {
        objectRotation -= 1.0f;
    }
    if (pressedKeys[GLFW_KEY_U]) {
        objectScale += 0.01f;
        objectScale = glm::min(objectScale, 2.0f);
    }
    if (pressedKeys[GLFW_KEY_J]) {
        lightAngle += 1.0f; // Rotate light left
    }
    if (pressedKeys[GLFW_KEY_L]) {
        lightAngle -= 1.0f; // Rotate light right
    }
    if (pressedKeys[GLFW_KEY_W]) {
        myCamera.move(gps::MOVE_FORWARD, speed);
    }
    if (pressedKeys[GLFW_KEY_S]) {
        myCamera.move(gps::MOVE_BACKWARD, speed);
    }
    if (pressedKeys[GLFW_KEY_A]) {
        myCamera.move(gps::MOVE_LEFT, speed);
    }
    if (pressedKeys[GLFW_KEY_D]) {
        myCamera.move(gps::MOVE_RIGHT, speed);
    }
    glm::vec3 pos = myCamera.getPosition();

    pos.x = glm::clamp(pos.x, -8.0f, 8.0f);
    pos.z = glm::clamp(pos.z, -8.0f, 8.0f);
    pos.y = glm::clamp(pos.y, 0.2f, 5.0f);

    myCamera.setPosition(pos);

}

bool initOpenGLWindow() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "Proiect OpenGL - Scena Iarna", NULL, NULL);

    if (!glWindow) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return false;
    }

    glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
    glfwSetKeyCallback(glWindow, keyboardCallback);
    glfwSetCursorPosCallback(glWindow, mouseCallback);

    glfwMakeContextCurrent(glWindow);
    glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSwapInterval(1);

#if !defined (_APPLE_)
    glewExperimental = GL_TRUE;
    glewInit();
#endif

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

    return true;
}

void initFBO() {
    // Create FBO for shadow mapping
    glGenFramebuffers(1, &shadowMapFBO);

    // Create depth texture
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Attach texture to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
    // Ca in lab9 - folosim rotirea luminii
    glm::vec3 lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    glm::mat4 lightRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 rotatedLightDir = glm::vec3(lightRotationMatrix * glm::vec4(lightDir, 0.0f));

    glm::vec3 lightPos = rotatedLightDir; // Pozitionam lumina undeva pe directie
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 lightView = glm::lookAt(lightPos, target, up);
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 20.0f);

    return lightProjection * lightView;
}

void renderScene(float deltaTime) {
    // Depth map creation pass
    depthMapShader.useShaderProgram();

    lightSpaceTrMatrix = computeLightSpaceTrMatrix();
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Object transformations pentru depth pass
    model = glm::mat4(1.0f);
    model = glm::translate(model, objectTranslate);
    model = glm::rotate(model, glm::radians(angle + objectRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(objectScale));
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    myModel.Draw(depthMapShader);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Final scene rendering pass (with shadows)
    glViewport(0, 0, retina_width, retina_height);
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    myCustomShader.useShaderProgram();

    // Directional light (soare) - ca in lab9
    glm::vec3 lightDir = glm::vec3(0.0f, 1.0f, 1.0f); // directia initiala
    glm::mat4 lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 rotatedLightDir = glm::vec3(lightRotation * glm::vec4(lightDir, 0.0f));
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f); // lumina alba ca in lab9

    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightDir"), 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

    // Trimite point lights (felinarele) la shader
    int numPointLights = (int)pointLightPositions.size();
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "numPointLights"), numPointLights);

    for (int i = 0; i < numPointLights && i < 8; i++) {
        std::string posName = "pointLightPos[" + std::to_string(i) + "]";
        std::string colorName = "pointLightColor[" + std::to_string(i) + "]";

        glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, posName.c_str()), 1, glm::value_ptr(pointLightPositions[i]));
        glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, colorName.c_str()), 1, glm::value_ptr(pointLightColors[i]));
    }

    // Bind shadow map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 1);

    // Send light space matrix
    glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Send camera position for fog
    glm::vec3 cameraPos = myCamera.getPosition();
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));

    // Send fog parameters
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), fogDensity);
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "fogColor"), 1, glm::value_ptr(fogColor));

    // Send smooth shading flag
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "isSmooth"), currentViewMode == VIEW_SMOOTH);

    // Object transformations (scale, translate, rotate)
    model = glm::mat4(1.0f);
    model = glm::translate(model, objectTranslate);
    model = glm::rotate(model, glm::radians(angle + objectRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(objectScale));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Calculate and send normal matrix after model is set
    glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(view * model));
    glUniformMatrix3fv(glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // Animation update
    animationTime += deltaTime;
    doorRotation = sin(animationTime * 0.5f) * 45.0f; // Oscilatie ușă exemplu (poți folosi pentru componente individuale)

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    myModel.Draw(myCustomShader);

    // Draw light cube
    lightCubeShader.useShaderProgram();
    
    glUniformMatrix4fv(glGetUniformLocation(lightCubeShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(lightCubeShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    model = lightRotation;
    model = glm::translate(model, 5.0f * lightDir);
    model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
    glUniformMatrix4fv(glGetUniformLocation(lightCubeShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    
    lightCube.Draw(lightCubeShader);

    // Dimensiune cub pentru felinare
    float lightCubeSize = 0.025f; // cub mic

    // Setam shader-ul cuburilor de lumina
    lightCubeShader.useShaderProgram();

    // Trimitem view si projection
    glUniformMatrix4fv(glGetUniformLocation(lightCubeShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(lightCubeShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    for (int i = 0; i < pointLightPositions.size() && i < 8; i++) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pointLightPositions[i]); // pozitia felinarului
        model = glm::scale(model, glm::vec3(lightCubeSize));  // cub mic
        glUniformMatrix4fv(glGetUniformLocation(lightCubeShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        // culoare galben cald
        glm::vec3 color = pointLightColors[i] * 0.3f; // reduc intensitatea daca vrei mai subtil
        glUniform3fv(glGetUniformLocation(lightCubeShader.shaderProgram, "objectColor"), 1, glm::value_ptr(color));

        lightCube.Draw(lightCubeShader);
    }

}


void cleanup() {
    glDeleteTextures(1, &depthMapTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &shadowMapFBO);
    glfwDestroyWindow(glWindow);
    glfwTerminate();
}

int main(int argc, const char* argv[]) {

    if (!initOpenGLWindow()) {
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    /*glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);*/

    // Incarcare Shadere
    myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
    myCustomShader.useShaderProgram();
    depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
    lightCubeShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");

    // Initializare matrici
    modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
    viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");

    projection = glm::perspective(glm::radians(55.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
    projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Initialize FBO for shadow mapping
    initFBO();
    // INCARCARE MODEL FINAL (Exportul tau din Blender)
    // Primul argument: calea catre fisierul .obj
    // Al doilea argument: folderul unde sunt texturile (.png)
    myModel.LoadModel("scene.obj", "./");
    lightCube.LoadModel("cube.obj", "./");
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(glWindow)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processMovement(deltaTime);
        renderScene(deltaTime);
        glfwPollEvents();
        glfwSwapBuffers(glWindow);
    }

    cleanup();
    return 0;
}