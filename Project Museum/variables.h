#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Model.h"
#include "Light.h"

typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::mat3 mat3;
typedef glm::mat4 mat4;

#ifdef MAINPROGRAM 
#define EXTERN 
#else 
#define EXTERN extern 
#endif

// General state settings
const bool TRANSPOSE_MAT = true;
const bool NO_TRANSPOSE_MAT = false;
const vec3 X_AXIS(1.0f, 0, 0);
const vec3 Y_AXIS(0, 1.0f, 0);
const vec3 Z_AXIS(0, 0, 1.0f);

// Camera setttings
EXTERN mat4 projMat;
EXTERN mat4 viewMat;

#ifdef MAINPROGRAM
GLFWwindow* window = nullptr;
Shader* phongShader = nullptr;
Model* teapot = nullptr;
Light* dirLight = nullptr;
Light* pntLight = nullptr;
Light* spotlight = nullptr;
unsigned int width = 800;
unsigned int height = 600;
float fovY = 90.0f;
vec3 camPos(0.0f, 0.0f, 5.0f);
vec3 camLookAt(0.0f, 0.0f, 0.0f);
vec3 camUp(0.0f, 1.0f, 0.0f);
#else
EXTERN GLFWwindow* window;
EXTERN Shader* phongShader;
EXTERN Model* teapot;
EXTERN Light* dirLight;
EXTERN Light* pntLight;
EXTERN Light* spotlight;
EXTERN unsigned int width, height;
EXTERN float fovY;
EXTERN vec3 camPos;
EXTERN vec3 camLookAt;
EXTERN vec3 camUp;
#endif