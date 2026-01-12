#if defined (APPLE)
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

int glWindowWidth = 12004;
int glWindowHeight = 700;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

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

gps::Camera myCamera(
    glm::vec3(0.0f, 2.0f, 5.0f),
    glm::vec3(0.0f, 2.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f)
);

std::vector<glm::vec3> pointLightPositions = {
    {2.22f, 0.42f, 2.72f},
    {2.50f, 0.42f, 2.66f},
    {2.43f, 0.10f, 3.57f},
    {2.71f, 0.11f, 3.57f},
    {2.00f, 0.10f, 1.88f},
    {2.28f, 0.10f, 1.81f}
};

std::vector<glm::vec3> pointLightColors = {
    {1.0f, 0.75f, 0.4f},
    {1.0f, 0.75f, 0.4f},
    {1.0f, 0.75f, 0.4f},
    {1.0f, 0.75f, 0.4f},
    {1.0f, 0.75f, 0.4f},
    {1.0f, 0.75f, 0.4f}
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float cutOff;
    float outerCutOff;
};

std::vector<SpotLight> spotLights = {
    { {6.1f, 0.35f, 1.4f}, glm::vec3(0.0f, -1.0f, 0.0f), {1.0f, 0.75f, 0.4f}, glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(17.5f)) },
    { {6.8f, 0.32f, 0.82f}, glm::vec3(0.0f, -1.0f, 0.0f), {1.0f, 0.75f, 0.4f}, glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(17.5f)) },
    { {5.2f, 0.32f, 1.93f}, glm::vec3(0.0f, -1.0f, 0.0f), {1.0f, 0.75f, 0.4f}, glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(17.5f)) }
};

float cameraSpeed = 3.0f;

bool pressedKeys[1024];
float angle = 0.0f;
float dragonRotation = 0.0f;

int viewMode = 0;

float objectScale = 1.0f;
glm::vec3 objectTranslate(0.0f, 0.0f, 0.0f);
float objectRotation = 0.0f;

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
gps::Model3D dragonModel;

gps::Shader myCustomShader;
gps::Shader depthMapShader;
gps::Shader lightCubeShader;
gps::Shader lightGlowShader;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;
GLuint shadowMapFBO;
GLuint depthMapTexture;
glm::mat4 lightSpaceTrMatrix;

float fogDensity = 0.045f;
glm::vec3 fogColor(0.75f, 0.8f, 0.85f);

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
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    myCamera.rotate(pitch, yaw);
}

void processMovement(float deltaTime) {
    float speed = cameraSpeed * deltaTime;

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

    if (pressedKeys[GLFW_KEY_Q]) {
        dragonRotation += 1.0f;
    }
    if (pressedKeys[GLFW_KEY_E]) {
        dragonRotation -= 1.0f;
    }
    if (pressedKeys[GLFW_KEY_U]) {
        objectScale += 0.01f;
        objectScale = glm::min(objectScale, 2.0f);
    }
    if (pressedKeys[GLFW_KEY_J]) {
        lightAngle += 1.0f;
    }
    if (pressedKeys[GLFW_KEY_L]) {
        lightAngle -= 1.0f;
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

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    glWindow = glfwCreateWindow(mode->width, mode->height, "Project - Frozen", primaryMonitor, NULL);


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

#if !defined (APPLE)
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
    glGenFramebuffers(1, &shadowMapFBO);

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

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
    // Ca in lab9
    glm::vec3 lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
    glm::mat4 lightRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 rotatedLightDir = glm::vec3(lightRotationMatrix * glm::vec4(lightDir, 0.0f));

    glm::vec3 lightPos = rotatedLightDir;
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 lightView = glm::lookAt(lightPos, target, up);
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 20.0f);

    return lightProjection * lightView;
}

void renderScene(float deltaTime)
{  //depth map
    depthMapShader.useShaderProgram();

    lightSpaceTrMatrix = computeLightSpaceTrMatrix();
    glUniformMatrix4fv(
        glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
        1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix)
    );

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    model = glm::mat4(1.0f);
    model = glm::translate(model, objectTranslate);
    model = glm::rotate(model, glm::radians(angle + objectRotation),
        glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(objectScale));

    glUniformMatrix4fv(
        glGetUniformLocation(depthMapShader.shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );

    myModel.Draw(depthMapShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //final scene
    glViewport(0, 0, retina_width, retina_height);
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    myCustomShader.useShaderProgram();

    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Fog
    glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, "fogDensity"), fogDensity);
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "fogColor"), 1, glm::value_ptr(fogColor));
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "isSmooth"), currentViewMode == VIEW_SMOOTH);

    // sun
    glm::vec3 sunBaseDir(0.0f, 1.0f, 1.0f);
    glm::mat4 sunRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 sunDirWorld = glm::vec3(sunRotation * glm::vec4(sunBaseDir, 0.0f));

    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightDir"), 1, glm::value_ptr(sunDirWorld));
    glm::vec3 sunColor(0.3f, 0.3f, 0.3f);
    glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightColor"), 1, glm::value_ptr(sunColor));

    // felinare mici
    int numPointLights = (int)pointLightPositions.size();
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "numPointLights"), numPointLights);

    for (int i = 0; i < numPointLights; i++)
    {
        std::string posName = "pointLightPos[" + std::to_string(i) + "]";
        std::string colName = "pointLightColor[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, posName.c_str()), 1, glm::value_ptr(pointLightPositions[i]));
        glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, colName.c_str()), 1, glm::value_ptr(pointLightColors[i]));
    }

    //felinare mari
    int numSpotLights = (int)spotLights.size();
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "numSpotLights"), numSpotLights);

    for (int i = 0; i < numSpotLights; i++)
    {
        std::string base = "spotLights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (base + ".position").c_str()), 1, glm::value_ptr(spotLights[i].position));
        glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (base + ".direction").c_str()), 1, glm::value_ptr(spotLights[i].direction));
        glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (base + ".color").c_str()), 1, glm::value_ptr(spotLights[i].color));
        glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (base + ".cutOff").c_str()), spotLights[i].cutOff);
        glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (base + ".outerCutOff").c_str()), spotLights[i].outerCutOff);
    }

    //shadow map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 1);
    glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceTrMatrix));

    // main model
    model = glm::mat4(1.0f);
    model = glm::translate(model, objectTranslate);
    model = glm::rotate(model, glm::radians(angle + objectRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(objectScale));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(view * model));
    glUniformMatrix3fv(glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    myModel.Draw(myCustomShader);
    float dragonAnimTime = glfwGetTime();

    float bobHeight = 0.1f * sin(dragonAnimTime * 2.0f); 
    float sway = 0.05f * sin(dragonAnimTime * 3.0f);
    float bodyRotation = 5.0f * sin(dragonAnimTime * 1.5f); 

    glm::mat4 dragonMatrix = glm::mat4(1.0f);
    dragonMatrix = glm::rotate(dragonMatrix, glm::radians(dragonRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    dragonMatrix = glm::translate(dragonMatrix, glm::vec3(1.5f + sway, 2.0f + bobHeight, 0.5f));
    dragonMatrix = glm::rotate(dragonMatrix, glm::radians(bodyRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    dragonMatrix = glm::scale(dragonMatrix, glm::vec3(0.5f));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(dragonMatrix));
    glm::mat3 dragonNormalMatrix = glm::mat3(glm::inverseTranspose(view * dragonMatrix));
    glUniformMatrix3fv(glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(dragonNormalMatrix));

    dragonModel.Draw(myCustomShader);

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
    myModel.LoadModel("scene.obj", "./");
    dragonModel.LoadModel("dragon.obj", "./");
    
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