#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <random>

const int   WIDTH = 1920;
const int   HEIGHT = 1080;
const int   GRID_SIZE = 200;
const float GRID_SCALE = 100.0f;
const int   NUM_STARS = 300;
const float STAR_SIZE = 1.5f;
const float MASS_CHANGE_SPEED_FAST = 5.0f;
const float MASS_CHANGE_SPEED_SLOW = 1.0f;
const float PI = 3.14159265358979323846f;

float sphereX = 0.0f;
float sphereY = 0.0f;
float sphereZ = 0.0f;
float sphereRadius = 5.0f;
float sphereStrength = 0.0f;
float minDeformation = -5.0f;

float satX = 0.0f;
float satY = 0.0f;
float satZ = 0.0f;
float orbitalRadius = 10.0f;
float satAngle = 0.0f;
float satSpeed = 0.015f;
float satMeshRadius = 0.3f;

void multiplyMatrix(const float a[16], const float b[16], float result[16]) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
}

void perspectiveMatrix(float fovY, float aspectRatio, float nearZ, float farZ, float result[16]) {
    float tanHalfFovY = tan(fovY / 2.0f);
    float range = farZ - nearZ;

    result[0] = 1.0f / (aspectRatio * tanHalfFovY);
    result[1] = 0.0f;
    result[2] = 0.0f;
    result[3] = 0.0f;

    result[4] = 0.0f;
    result[5] = 1.0f / tanHalfFovY;
    result[6] = 0.0f;
    result[7] = 0.0f;

    result[8] = 0.0f;
    result[9] = 0.0f;
    result[10] = -((farZ + nearZ) / range);
    result[11] = -1.0f;

    result[12] = 0.0f;
    result[13] = 0.0f;
    result[14] = -((2.0f * farZ * nearZ) / range);
    result[15] = 0.0f;
}

void lookAtMatrix(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ, float result[16]) {
    float forward[3];
    forward[0] = centerX - eyeX;
    forward[1] = centerY - eyeY;
    forward[2] = centerZ - eyeZ;

    float forwardLength = sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
    forward[0] /= forwardLength;
    forward[1] /= forwardLength;
    forward[2] /= forwardLength;

    float right[3];
    right[0] = upY * forward[2] - upZ * forward[1];
    right[1] = upZ * forward[0] - upX * forward[2];
    right[2] = upX * forward[1] - upY * forward[0];

    float rightLength = sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
    right[0] /= rightLength;
    right[1] /= rightLength;
    right[2] /= rightLength;

    float up[3];
    up[0] = forward[1] * right[2] - forward[2] * right[1];
    up[1] = forward[2] * right[0] - forward[0] * right[2];
    up[2] = forward[0] * right[1] - forward[1] * right[0];

    result[0] = right[0];
    result[1] = up[0];
    result[2] = -forward[0];
    result[3] = 0.0f;

    result[4] = right[1];
    result[5] = up[1];
    result[6] = -forward[1];
    result[7] = 0.0f;

    result[8] = right[2];
    result[9] = up[2];
    result[10] = -forward[2];
    result[11] = 0.0f;

    result[12] = -(right[0] * eyeX + right[1] * eyeY + right[2] * eyeZ);
    result[13] = -(up[0] * eyeX + up[1] * eyeY + up[2] * eyeZ);
    result[14] = -(-forward[0] * eyeX - forward[1] * eyeY - forward[2] * eyeZ);
    result[15] = 1.0f;
}

GLuint createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, int segments) {
    vertices.clear();
    indices.clear();

    for (int y = 0; y <= segments; ++y) {
        float v = (float)y / (float)segments;
        float phi = v * PI;

        for (int x = 0; x <= segments; ++x) {
            float u = (float)x / (float)segments;
            float theta = u * 2.0f * PI;

            float xPos = radius * sin(phi) * cos(theta);
            float yPos = radius * cos(phi);
            float zPos = radius * sin(phi) * sin(theta);

            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
        }
    }

    for (int y = 0; y < segments; ++y) {
        for (int x = 0; x < segments; ++x) {
            int p1 = y * (segments + 1) + x;
            int p2 = p1 + 1;
            int p3 = (y + 1) * (segments + 1) + x;
            int p4 = p3 + 1;

            indices.push_back(p1);
            indices.push_back(p2);
            indices.push_back(p3);

            indices.push_back(p2);
            indices.push_back(p4);
            indices.push_back(p3);
        }
    }
}

void generateGrid(std::vector<float>& vertices, std::vector<unsigned int>& indices, float sphereX, float sphereZ, float sphereRadius, float sphereStrength, float minDeformation) {
    vertices.clear();
    indices.clear();
    for (int z = 0; z < GRID_SIZE; ++z) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            float xPos = (x - GRID_SIZE / 2.0f) / (float)(GRID_SIZE / 2.0f) * GRID_SCALE;
            float zPos = (z - GRID_SIZE / 2.0f) / (float)(GRID_SIZE / 2.0f) * GRID_SCALE;

            float yPos = 0.0f;

            float distX = xPos - sphereX;
            float distZ = zPos - sphereZ;
            float dist = std::sqrt(distX * distX + distZ * distZ);
            float normalizedDist = dist / (sphereRadius * 2.5f);

            if (dist < sphereRadius * 2.5f) {
                float deformation = -sphereStrength * 2.0f * (1.0f / (std::sqrt(std::pow(normalizedDist, 2.0f) + 0.1f)) - 1.0f);

                if (normalizedDist < 1.0f)
                    yPos = deformation;
                else
                    yPos = -sphereStrength * 0.1f;
            }
            else {
                yPos = -sphereStrength * 0.1f;
            }

            if (yPos < minDeformation) {
                yPos = minDeformation;
            }

            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);

            if (x < GRID_SIZE - 1 && z < GRID_SIZE - 1) {
                int current = z * GRID_SIZE + x;

                indices.push_back(current);
                indices.push_back(current + 1);
                indices.push_back(current + GRID_SIZE);

                indices.push_back(current + 1);
                indices.push_back(current + GRID_SIZE + 1);
                indices.push_back(current + GRID_SIZE);
            }
        }
    }
}

void generateStars(std::vector<float>& vertices) {
    vertices.clear();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-50.0f, 50.0f);

    for (int i = 0; i < NUM_STARS; ++i) {
        vertices.push_back(dis(gen));
        vertices.push_back(dis(gen));
        vertices.push_back(dis(gen));
    }
}

float computeLowestGridY(float sphereX, float sphereZ, float sphereRadius, float sphereStrength, float minDeformation) {
    float lowestY = 0.0f;
    for (int z = 0; z < GRID_SIZE; ++z) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            float xPos = (x - GRID_SIZE / 2.0f) / (float)(GRID_SIZE / 2.0f) * GRID_SCALE;
            float zPos = (z - GRID_SIZE / 2.0f) / (float)(GRID_SIZE / 2.0f) * GRID_SCALE;

            float distX = xPos - sphereX;
            float distZ = zPos - sphereZ;
            float dist = std::sqrt(distX * distX + distZ * distZ);

            float normalizedDist = dist / (sphereRadius * 2.5f);

            if (dist < sphereRadius * 2.5f) {
                float deformation = -sphereStrength * 2.0f * (1.0f / (std::sqrt(std::pow(normalizedDist, 2.0f) + 0.1f)) - 1.0f);
                if (normalizedDist < 1.0f && deformation < lowestY) {
                    lowestY = deformation;
                }
                else if (deformation < lowestY) {
                    lowestY = -sphereStrength * 0.1f;
                }
            }
            else if ((-sphereStrength * 0.1f) < lowestY) {
                lowestY = -sphereStrength * 0.1f;
            }

            if (lowestY < minDeformation) {
                lowestY = minDeformation;
            }
        }
    }
    return lowestY;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Spacetime Curvature", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    const char* gridVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* gridFragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        void main() {
            FragColor = vec4(0.8f, 0.8f, 0.8f, 1.0f);
        }
    )";

    const char* sphereVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* sphereFragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        void main() {
            FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    )";

    const char* starVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        uniform mat4 view;
        uniform mat4 projection;
        uniform float pointSize;

        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
            gl_PointSize = pointSize;
        }
    )";

    const char* starFragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        void main() {
            FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    )";

    GLuint gridShaderProgram = createShaderProgram(gridVertexShaderSource, gridFragmentShaderSource);
    GLuint sphereShaderProgram = createShaderProgram(sphereVertexShaderSource, sphereFragmentShaderSource);
    GLuint starShaderProgram = createShaderProgram(starVertexShaderSource, starFragmentShaderSource);

    std::vector<float>    gridVertices;
    std::vector<unsigned int> gridIndices;

    std::vector<float>    sphereVertices;
    std::vector<unsigned int> sphereIndices;

    float sphereMeshRadius = 1.0f;
    int   sphereSegments = 30;
    generateSphere(sphereVertices, sphereIndices, sphereMeshRadius, sphereSegments);

    std::vector<float>    satelliteVertices;
    std::vector<unsigned int> satelliteIndices;
    int   satelliteSegments = 20;
    generateSphere(satelliteVertices, satelliteIndices, satMeshRadius, satelliteSegments);

    std::vector<float> starVertices;
    generateStars(starVertices);
    generateGrid(gridVertices, gridIndices, sphereX, sphereZ, sphereRadius, sphereStrength, minDeformation);
    sphereY = computeLowestGridY(sphereX, sphereZ, sphereRadius, sphereStrength, minDeformation) + sphereRadius + sphereMeshRadius + 0.1f;

    GLuint gridVAO, gridVBO, gridEBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glGenBuffers(1, &gridEBO);

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, gridIndices.size() * sizeof(unsigned int), gridIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLuint sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLuint satelliteVAO, satelliteVBO, satelliteEBO;
    glGenVertexArrays(1, &satelliteVAO);
    glGenBuffers(1, &satelliteVBO);
    glGenBuffers(1, &satelliteEBO);

    glBindVertexArray(satelliteVAO);
    glBindBuffer(GL_ARRAY_BUFFER, satelliteVBO);
    glBufferData(GL_ARRAY_BUFFER, satelliteVertices.size() * sizeof(float), satelliteVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, satelliteEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, satelliteIndices.size() * sizeof(unsigned int), satelliteIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLuint starVAO, starVBO;
    glGenVertexArrays(1, &starVAO);
    glGenBuffers(1, &starVBO);

    glBindVertexArray(starVAO);
    glBindBuffer(GL_ARRAY_BUFFER, starVBO);
    glBufferData(GL_ARRAY_BUFFER, starVertices.size() * sizeof(float), starVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float model[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    float view[16];
    lookAtMatrix(0.0f, 20.0f, 40.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, view);

    float projection[16];
    perspectiveMatrix(45.0f * PI / 180.0f, (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f, projection);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    float lastFrame = 0.0f;
    float titleUpdateTimer = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float massChangeSpeed = MASS_CHANGE_SPEED_SLOW;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            massChangeSpeed = MASS_CHANGE_SPEED_FAST;
        }

        if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
            sphereStrength += massChangeSpeed * deltaTime;
        }
        if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
            sphereStrength = std::max(0.1f, sphereStrength - massChangeSpeed * deltaTime);
        }

        titleUpdateTimer += deltaTime;
        if (titleUpdateTimer >= 0.3f) {
            char title[128];
            snprintf(title, sizeof(title),
                "Spacetime Curvature | Mass: %.1f (Hold Shift for fast change, +/- to adjust)",
                sphereStrength);
            glfwSetWindowTitle(window, title);
            titleUpdateTimer = 0.0f;
        }

        glUseProgram(starShaderProgram);
        glBindVertexArray(starVAO);

        int starViewLoc = glGetUniformLocation(starShaderProgram, "view");
        glUniformMatrix4fv(starViewLoc, 1, GL_FALSE, view);
        int starProjectionLoc = glGetUniformLocation(starShaderProgram, "projection");
        glUniformMatrix4fv(starProjectionLoc, 1, GL_FALSE, projection);
        int pointSizeLoc = glGetUniformLocation(starShaderProgram, "pointSize");
        glUniform1f(pointSizeLoc, STAR_SIZE);

        glDrawArrays(GL_POINTS, 0, starVertices.size() / 3);

        glUseProgram(gridShaderProgram);
        glBindVertexArray(gridVAO);
        int viewLoc = glGetUniformLocation(gridShaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
        int projectionLoc = glGetUniformLocation(gridShaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);
        int modelLoc = glGetUniformLocation(gridShaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, gridIndices.size(), GL_UNSIGNED_INT, 0);

        glUseProgram(sphereShaderProgram);
        glBindVertexArray(sphereVAO);

        float sphereModel[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        sphereModel[12] = sphereX;
        sphereModel[13] = sphereY;
        sphereModel[14] = sphereZ;
        float scaleFactor = 2.0f;
        sphereModel[0] *= scaleFactor;
        sphereModel[5] *= scaleFactor;
        sphereModel[10] *= scaleFactor;
        sphereModel[12] /= scaleFactor;
        sphereModel[13] /= scaleFactor;
        sphereModel[14] /= scaleFactor;

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, sphereModel);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        satAngle += satSpeed;
        satX = orbitalRadius * cos(satAngle);
        satZ = orbitalRadius * sin(satAngle);
        satY = sphereY;

        float satModel[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        satModel[12] = satX;
        satModel[13] = satY;
        satModel[14] = satZ;

        glUseProgram(sphereShaderProgram);
        glBindVertexArray(satelliteVAO);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, satModel);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, satelliteIndices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        generateGrid(gridVertices, gridIndices, sphereX, sphereZ, sphereRadius, sphereStrength, minDeformation);
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, gridVertices.size() * sizeof(float), gridVertices.data());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridEBO);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, gridIndices.size() * sizeof(unsigned int), gridIndices.data());

        sphereY = computeLowestGridY(sphereX, sphereZ, sphereRadius, sphereStrength, minDeformation) + sphereRadius + sphereMeshRadius + 0.1f;
    }

    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteBuffers(1, &gridEBO);

    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);

    glDeleteVertexArrays(1, &satelliteVAO);
    glDeleteBuffers(1, &satelliteVBO);
    glDeleteBuffers(1, &satelliteEBO);

    glDeleteVertexArrays(1, &starVAO);
    glDeleteBuffers(1, &starVBO);

    glDeleteProgram(gridShaderProgram);
    glDeleteProgram(sphereShaderProgram);
    glDeleteProgram(starShaderProgram);

    glfwTerminate();
    return 0;
}
