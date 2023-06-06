#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

std::string loadShaderSource(const std::string& filePath) {
    std::ifstream inFile(filePath);
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    return buffer.str();
}

static unsigned int compileShader(
    unsigned int type,
    const std::string& source
) {
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);

        std::string typeStr = "";
        switch (type) {
        case GL_VERTEX_SHADER:
            typeStr = "vertex";
            break;
        case GL_FRAGMENT_SHADER:
            typeStr = "fragment";
            break;
        default:
            break;
        }

        std::cout
            << "failed to compile "
            << typeStr
            << " shader:"
            << std::endl
            << message
            << std::endl;

        glDeleteShader(id);
        return 0;
    }
    return id;
}

unsigned int createShaderProgram(
    const std::string& vShdrFilePath,
    const std::string& fShdrFilePath
) {
    unsigned int program = glCreateProgram();

    std::string vShdrSrc = loadShaderSource(vShdrFilePath);
    std::string fShdrSrc = loadShaderSource(fShdrFilePath);
    unsigned int vShdr = compileShader(GL_VERTEX_SHADER, vShdrSrc);
    unsigned int fShdr = compileShader(GL_FRAGMENT_SHADER, fShdrSrc);

    glAttachShader(program, vShdr);
    glAttachShader(program, fShdr);

    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vShdr);
    glDeleteShader(fShdr);

    return program;
}

struct Material {
    glm::vec3 color;
    float emission_strength;
    glm::vec3 emission_color;
    float padding;
};

struct Sphere {
    glm::vec3 position;
    float radius;
    Material material;
};

struct Plane {
    glm::vec3 position;
    float padding0;
    glm::vec3 normal;
    float padding1;
    Material material;
};

float sphere_x = 0.01f;
float sphere_y = -0.015f;
float sphere_z = 0.13f;
int numBounces = 1;
int numSamples = 1;

bool render_stale = true;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    render_stale = true;

    switch (key) {
    case GLFW_KEY_W:
        sphere_y += 0.001f;
        break;
    case GLFW_KEY_S:
        sphere_y -= 0.001f;
        break;
    case GLFW_KEY_A:
        sphere_x += 0.001f;
        break;
    case GLFW_KEY_D:
        sphere_x -= 0.001f;
        break;
    case GLFW_KEY_Z:
        sphere_z += 0.001f;
        break;
    case GLFW_KEY_X:
        sphere_z -= 0.001f;
        break;
    case GLFW_KEY_1:
        numBounces = (numBounces > 0) ? (numBounces - 1) : 0;
        break;
    case GLFW_KEY_2:
        numBounces += 1;
        break;
    case GLFW_KEY_3:
        numSamples = (numSamples > 1) ? (numSamples / 2) : 1;
        break;
    case GLFW_KEY_4:
        numSamples *= 2;
        break;
    default:
        render_stale = false;
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "failed to initialize glfw" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "CSCI544 Assignment 3", nullptr, nullptr);
    if (!window) {
        std::cerr << "failed to create glfw window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    gladLoadGL();

    GLuint path_tracer = createShaderProgram(
        "../src/screen_quad.vert.glsl",
        "../src/path_tracer.frag.glsl"
    );

    GLuint vao;
    glGenVertexArrays(1, &vao);

    GLuint query;
    glGenQueries(1, &query);

    glm::vec3 camera_position = glm::vec3(0.0f, 0.0f, -4.0f);
    float fov = 0.8f;
    float nearClip = 0.1f;
    float aspect = 1.0f;
    float half_height = nearClip * tan(fov / 2.0f);
    float half_width = aspect * half_height;

    Material material;

    std::vector<Sphere> spheres;

    Sphere sphere;
    sphere.position = camera_position + glm::vec3(-0.025f, -0.025f, 0.1f);
    sphere.radius = 0.01f;
    sphere.material.color = glm::vec3(0.3f, 0.3f, 0.8f);
    sphere.material.emission_color = glm::vec3(0.0f);
    sphere.material.emission_strength = 0.0f;
    spheres.push_back(sphere);

    sphere.position = camera_position + glm::vec3(sphere_x, sphere_y, sphere_z);
    sphere.radius = 0.02f;
    sphere.material.color = glm::vec3(0.8f, 0.8f, 0.3f);
    sphere.material.emission_color = glm::vec3(0.0f);
    sphere.material.emission_strength = 0.0f;
    spheres.push_back(sphere);

    sphere.position = camera_position + glm::vec3(-0.02f, 0.01f, 0.15f);
    sphere.radius = 0.01;
    sphere.material.color = glm::vec3(0.3f, 0.8f, 0.8f);
    sphere.material.emission_color = glm::vec3(0.0f);
    sphere.material.emission_strength = 0.0f;
    spheres.push_back(sphere);

    GLuint sphereSSBO;
    glGenBuffers(1, &sphereSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, spheres.size() * sizeof(Sphere), spheres.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    std::vector<Plane> planes;

    Plane plane;
    plane.position = glm::vec3(0.0, half_height, 0.0);
    plane.normal = glm::vec3(0.0, -1.0, 0.0);
    plane.material.color = glm::vec3(0.8);
    plane.material.emission_color = glm::vec3(0.0);
    plane.material.emission_strength = 0.0;
    planes.push_back(plane);

    plane.position = glm::vec3(0.0, -half_height, 0.0);
    plane.normal = glm::vec3(0.0, 1.0, 0.0);
    plane.material.color = glm::vec3(0.8, 0.8, 0.8);
    plane.material.emission_color = glm::vec3(0.0);
    plane.material.emission_strength = 0.0;
    planes.push_back(plane);

    plane.position = glm::vec3(half_width, 0.0, 0.0);
    plane.normal = glm::vec3(-1.0, 0.0, 0.0);
    plane.material.color = glm::vec3(0.8, 0.3, 0.3);
    plane.material.emission_color = glm::vec3(0.0);
    plane.material.emission_strength = 0.0;
    planes.push_back(plane);

    plane.position = glm::vec3(-half_width, 0.0, 0.0);
    plane.normal = glm::vec3(1.0, 0.0, 0.0);
    plane.material.color = glm::vec3(0.3, 0.8, 0.3);
    plane.material.emission_color = glm::vec3(0.0);
    plane.material.emission_strength = 0.0;
    planes.push_back(plane);

    plane.position = glm::vec3(0.0, 0.0, camera_position.z + 0.2);
    plane.normal = glm::vec3(0.0, 0.0, -1.0);
    plane.material.color = glm::vec3(0.8, 0.8, 0.8);
    plane.material.emission_color = glm::vec3(0.0);
    plane.material.emission_strength = 0.0;
    planes.push_back(plane);

    GLuint planeSSBO;
    glGenBuffers(1, &planeSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, planeSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, planes.size() * sizeof(plane), planes.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    while (!glfwWindowShouldClose(window)) {

        if (render_stale) { // only re-render the scene if uniforms have changed
            spheres[1].position = camera_position + glm::vec3(sphere_x, sphere_y, sphere_z);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 1 * sizeof(Sphere), sizeof(Sphere), &spheres[1]);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            glBeginQuery(GL_TIME_ELAPSED, query);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(path_tracer);

            GLuint numBouncesLoc = glGetUniformLocation(path_tracer, "numBounces");
            glUniform1i(numBouncesLoc, numBounces);

            GLuint numSamplesLoc = glGetUniformLocation(path_tracer, "numSamples");
            glUniform1i(numSamplesLoc, numSamples);

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphereSSBO);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, planeSSBO);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            glUseProgram(0);

            glEndQuery(GL_TIME_ELAPSED);

            GLuint64 time_elapsed;
            glGetQueryObjectui64v(query, GL_QUERY_RESULT, &time_elapsed);
            double time_elapsed_ms = time_elapsed / 1.0e6;

            char title[256];
            snprintf(
                title,
                sizeof(title),
                "CSCI544 Assignment 3 | bounces: %d | samples: %d | last frame time: %.2lf ms",
                numBounces,
                numSamples,
                time_elapsed_ms
            );

            glfwSetWindowTitle(window, title);

            glfwSwapBuffers(window);
            render_stale = false;
        }

        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}