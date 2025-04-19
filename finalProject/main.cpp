#include <glad.h>
#include <glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>

// --- Settings ---
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const char* DATA_FILENAME = "head256x256x109";
const int DATA_WIDTH = 256;
const int DATA_HEIGHT = 256;
const int DATA_DEPTH = 109;

// --- Globals ---
GLFWwindow* window = nullptr;
unsigned int shaderProgram = 0;
unsigned int quadVAO = 0, quadVBO = 0;
unsigned int texture3D = 0;
char* rawBuffer = nullptr;
unsigned char* rgbaBuffer = nullptr;
long filelen = 0;

// Interaction
float angleX = 0.0f;
float angleY = 0.0f;
float nearPlane = 1.0f;

// --- Function Prototypes ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
bool loadAndCompileShaders(const char* vertexPath, const char* fragmentPath);
bool loadTextureData(const char* filename);
bool create3DTexture();
void setupVertexData();
void cleanup();

// --- Main Function ---
int main() {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Create Window
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL MRI Viewer", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // 3. Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // 4. Load and Compile Shaders
    if (!loadAndCompileShaders("vertex.glsl", "fragment.glsl")) {
        cleanup();
        return -1;
    }

    // 5. Load MRI Data
    if (!loadTextureData(DATA_FILENAME)) {
        cleanup();
        return -1;
    }

    // 6. Convert to RGBA and Create 3D Texture
    if (!create3DTexture()) {
        cleanup();
        return -1;
    }

    // 7. Setup Vertex Data for rendering slices (a simple quad)
    setupVertexData();

    // 8. Configure OpenGL state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    // Display controls information
    std::cout << "Controls:" << std::endl;
    std::cout << "  S - Move inside the head (increase near plane)" << std::endl;
    std::cout << "  A - Move outside the head (decrease near plane)" << std::endl;
    std::cout << "  Arrow Keys - Rotate the view" << std::endl;
    std::cout << "  R - Reset view" << std::endl;
    std::cout << "  ESC - Exit the application" << std::endl;


    // --- Render Loop ---
    while (!glfwWindowShouldClose(window)) {
        // Input
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.5f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, nearPlane, 4.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 transformTex = glm::mat4(1.0f);
        transformTex = glm::translate(transformTex, glm::vec3(0.5f, 0.5f, 0.5f));
        transformTex = glm::rotate(transformTex, glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
        transformTex = glm::rotate(transformTex, glm::radians(angleX), glm::vec3(1.0f, 0.0f, 0.0f));
        transformTex = glm::translate(transformTex, glm::vec3(-0.5f, -0.5f, -0.5f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "Ttex"), 1, GL_FALSE, glm::value_ptr(transformTex));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, texture3D);
        glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

        glBindVertexArray(quadVAO);
        int numSlices = 150;
        for (int i = 0; i < numSlices; ++i) {
           
            float slicePos = -0.5f + (float)i / (numSlices - 1);
            float depthCoord = (float)i / (numSlices - 1);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, slicePos));
            model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            glUniform1f(glGetUniformLocation(shaderProgram, "profundidad"), depthCoord);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanup();
    return 0;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        float rotationStep = 2.0f;
        float planeStep = 0.01f;

        switch (key) {
        case GLFW_KEY_A:
            nearPlane -= planeStep;
            std::cout << "Near Plane: " << nearPlane << std::endl;
            break;
        case GLFW_KEY_S:
            nearPlane += planeStep;
            std::cout << "Near Plane: " << nearPlane << std::endl;
            break;
        case GLFW_KEY_LEFT:
            angleY -= rotationStep;
            break;
        case GLFW_KEY_RIGHT:
            angleY += rotationStep;
            break;
        case GLFW_KEY_UP:
            angleX -= rotationStep;
            break;
        case GLFW_KEY_DOWN:
            angleX += rotationStep;
            break;
        case GLFW_KEY_R:
            angleX = 0.0f;
            angleY = 0.0f;
            nearPlane =1.0f;
            std::cout << "View Reset" << std::endl;
            break;
        }
    }
}

bool loadAndCompileShaders(const char* vertexPath, const char* fragmentPath) {

    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        return false;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertex);
        return false;
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex);
    glAttachShader(shaderProgram, fragment);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        return false;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    std::cout << "Shaders loaded and compiled successfully." << std::endl;
    return true;
}


bool loadTextureData(const char* filename) {
    FILE* fileptr;
    errno_t err;

    if ((err = fopen_s(&fileptr, filename, "rb")) != 0) {
        char errBuff[100];
        strerror_s(errBuff, sizeof(errBuff), err);
        std::cerr << "ERROR::TEXTURE::Cannot open file '" << filename << "': " << errBuff << std::endl;
        return false;
    }

    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);

    long expectedSize = (long)DATA_WIDTH * DATA_HEIGHT * DATA_DEPTH;
    if (filelen != expectedSize) {
        std::cerr << "ERROR::TEXTURE::File size " << filelen << " does not match expected size " << expectedSize << std::endl;
        fclose(fileptr);
        return false;
    }

    rawBuffer = (char*)malloc(filelen * sizeof(char));
    if (rawBuffer == nullptr) {
        std::cerr << "ERROR::TEXTURE::Failed to allocate memory for raw file buffer" << std::endl;
        fclose(fileptr);
        return false;
    }

    size_t bytesRead = fread(rawBuffer, 1, filelen, fileptr);
    fclose(fileptr);

    if (bytesRead != (size_t)filelen) {
        std::cerr << "ERROR::TEXTURE::Failed to read the correct number of bytes. Read " << bytesRead << ", expected " << filelen << std::endl;
        free(rawBuffer);
        rawBuffer = nullptr;
        return false;
    }

    std::cout << "Texture data '" << filename << "' loaded successfully. Size: " << filelen << " bytes." << std::endl;
    return true;
}

bool create3DTexture() {
    if (!rawBuffer) {
        std::cerr << "ERROR::TEXTURE::Raw data buffer is null. Load data first." << std::endl;
        return false;
    }

    size_t rgbaBufferSize = (size_t)DATA_WIDTH * DATA_HEIGHT * DATA_DEPTH * 4;
    rgbaBuffer = new unsigned char[rgbaBufferSize];
    if (!rgbaBuffer) {
        std::cerr << "ERROR::TEXTURE::Failed to allocate memory for RGBA buffer" << std::endl;
        return false;
    }

    for (long nIndx = 0; nIndx < filelen; ++nIndx) {
        unsigned char intensity = static_cast<unsigned char>(rawBuffer[nIndx]);
        rgbaBuffer[nIndx * 4 + 0] = intensity;
        rgbaBuffer[nIndx * 4 + 1] = intensity;
        rgbaBuffer[nIndx * 4 + 2] = intensity;
        rgbaBuffer[nIndx * 4 + 3] = intensity;
    }

    glGenTextures(1, &texture3D);
    glBindTexture(GL_TEXTURE_3D, texture3D);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, DATA_WIDTH, DATA_HEIGHT, DATA_DEPTH, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuffer);

    glBindTexture(GL_TEXTURE_3D, 0);

    std::cout << "3D Texture created successfully." << std::endl;

    return true;
}


void setupVertexData() {

    float quadVertices[] = {

        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, // Bottom Left
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f, // Bottom Right
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f, // Top Right

        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, // Bottom Left
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f, // Top Right
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f  // Top Left
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void cleanup() {
    std::cout << "Cleaning up..." << std::endl;
    if (quadVAO != 0) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO != 0) glDeleteBuffers(1, &quadVBO);
    if (texture3D != 0) glDeleteTextures(1, &texture3D);
    if (shaderProgram != 0) glDeleteProgram(shaderProgram);

    if (rawBuffer != nullptr) {
        free(rawBuffer);
        rawBuffer = nullptr;
    }
    if (rgbaBuffer != nullptr) {
        delete[] rgbaBuffer;
        rgbaBuffer = nullptr;
    }

    if (window != nullptr) {
        glfwTerminate();
    }
    std::cout << "Cleanup complete." << std::endl;
}