/* 
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*		PROGRAM:	CSCI 3090U Final Graphics Project
*		PURPOSE:	AoL (America Online) Logo Render
*		AUTHORS:	Alvin Lum, Kenny Le
* LAST MODIFIED:	March 29, 2019
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	ABOUT
* -----------------------------------------------------------------------------------------------------------------------------------------------------
* - This project renders AoL (America Online) Model and Logo (OBJ format) generated through 3DPaint and Blender and applies multiple effects to them
*
* - This project uses GLFW graphics library framework (instead of GLUT) for more control over main loop and other functionalities to allow
* easier expansion in the future.
*
* - The logos are rotated, scaled, and translated
*
* - The AoL man moves in a bezier spline
*
* - Adapts some code from CSCI 3090U Labs and Lectures
*
* - Adapts some methods and uses several small external libraries from http://www.opengl-tutorial.org/beginners-tutorials/ 
*	- AntTweakBar			// External library to create a transparent debug window on top to display variable values 
*	- shader.hpp			// Shader
*	- texture.hpp			// Texturer
*	- controls.hpp			// User controls camera: mouse to control camera lookAt, arrow keys to control camera position
*	- objloader.hpp			// OBJ Loader, works with Blender exported models (CSCI 3090U provided objloader did not correctly load Blender exported OBJs)
*	- vboindexer.hpp		// Vertex Buffer Object Indexer (indexes for OBJ)
*	- quaternion_utils.hpp	// Quaternion functions (ie. LookAt, RotateTowards, RotationBetweenVectors)
*
* - DISCLAIMER:	This project is for educational purposes only. No copyright infringement intended.
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	HOW TO RUN
* -----------------------------------------------------------------------------------------------------------------------------------------------------
* - Run the program application (.exe) or compile using the NMake files. Alternatively, use CMake or open the solution in Visual Studio and run (Debug)
* 
* - Watch the program interact with objects using multiple effects, then press ESC to exit the program.
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/

// Standard
#define _USE_MATH_DEFINES	// For Pi (radian <-> degree conversions)
#include <stdio.h>			// Standard
#include <stdlib.h>			// Standard
#include <vector>			// Vectors
#include <string>			// String input/output
#include <iostream>			// Input/output stream
#include <fstream>			// File stream
#include <cmath>			// Math functions
#include <wtypes.h>			// Get screen resolution to create fullscreen application

// Glew
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
using namespace glm;

// External Libraries
#include <AntTweakBar.h>				// Tweak parameters on the go UI window
#include <common/shader.hpp>			// Shader
#include <common/texture.hpp>			// Texturer
#include <common/controls.hpp>			// User controls camera: mouse to control camera lookAt, arrow keys to control camera position
#include <common/objloader.hpp>			// OBJ Loader, works with Blender exported models
#include <common/vboindexer.hpp>		// Vertex Buffer Object Indexer (indexes for OBJ)
#include <common/quaternion_utils.hpp>	// Quaternion functions (ie. LookAt, RotateTowards, RotationBetweenVectors)

using namespace std;
// ----------------------------------------------------------------------------------------------------------------------------------------------------

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	CONFIGURATIONS - Users may edit these configurations for testing
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/

bool FULL_SCREEN	= false;	// Screen Resolution - true for fullscreen, if false then defaults to SCREEN_WIDTH, SCREEN_HEIGHT
int SCREEN_WIDTH	= 1024;
int SCREEN_HEIGHT	= 768;

bool DEBUG			= true;		// Print debug diagnostic messages in the console for testing

double magnitude	= 1.0f;		// Magnitude of how fast the Logos rotate, scale, and translate

float LIGHT_X		= 2.5f;		// Light position (X, Y, Z)
float LIGHT_Y		= 2.5f;
float LIGHT_Z		= 4.0f;

GLclampf BG_RED		= 0.00f;	// Background Colour - Red, Green, Blue (0.00 to 1.00)
GLclampf BG_GREEN	= 0.25f;
GLclampf BG_BLUE	= 0.50f;
// ----------------------------------------------------------------------------------------------------------------------------------------------------

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	GLOBAL VARIABLES
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/
GLFWwindow* window;
GLuint programId;

// OBJ vertex buffer objects and textures for Object 1: AoL Logo
GLuint positions_vbo1;
GLuint textureCoords_vbo1;
GLuint normals_vbo1;
GLuint indexBuffer1;
GLuint texture1;
GLuint textureId1;
vector<unsigned short> indices1;

// OBJ vertex buffer objects and textures for Object 2: AoL Man
GLuint positions_vbo2;
GLuint textureCoords_vbo2;
GLuint normals_vbo2;
GLuint indexBuffer2;
GLuint texture2;
GLuint textureId2;
vector<unsigned short> indices2;

// Render IDs and handles
GLuint MatrixID;
GLuint ViewMatrixID;
GLuint ModelMatrixID;
GLuint vertexPosition_modelspaceID;
GLuint vertexUVID;
GLuint vertexNormal_modelspaceID;

// Perspective projection
glm::mat4 projectionMatrix;

// Orient everything around our preferred view
glm::mat4 viewMatrix;

// Light
GLuint LightID;
glm::vec3 lightPos;

// Time (for loop, update)
double lastTime;
double lastFrameTime;
double currentTime;
double deltaTime;
double elapsedFrames;

// Rotation, scaling, translation variables to modify in realtime
float rotationAngleDegree = 0.0f;
float rotationX = 0.0f;
float scalingValue = 0.0f;
float translationValue = 0.0f;

// Rotation for man model
float rotationZ = 0.0f;

// Normal mapping
GLuint normalTexture;
GLuint normalTextureId;

// Bezier curve control points (for AoL man travel curve)
glm::vec3 controlPoint1(-2.00f, -1.50f, 2.00f);
glm::vec3 controlPoint2(2.50f, 2.50f, -0.50f);
glm::vec3 controlPoint3(-1.00f, 1.00f, -2.50f);
glm::vec3 controlPoint4(2.00f, -2.50f, -0.50f);
float manX = 0.0f;
float manY = 0.0f; 
float manZ = 0.0f;
// ----------------------------------------------------------------------------------------------------------------------------------------------------

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	FUNCTIONS - Mini functions that the main method uses (other larger functions are in separate .cpp/h and included as a library)
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/
// Get the user's screen resolution to allow fullscreen option
void getScreenResolution(int& width, int& height)
{
	RECT screen;
	HWND handle = GetDesktopWindow();	// Get handle of destop window
	GetWindowRect(handle, &screen);		// Get screen
	width = screen.right;				// Adjust width
	height = screen.bottom;				// Adjust height
}

// Move the AoL Man in a bezier curve path over input of time (t = 0 to 1)
void moveBezierPath(float t)
{
	manX = pow(1 - t, 3) * controlPoint1.x + pow(1 - t, 2) * 3 * t * controlPoint2.x + (1 - t) * pow(3 * t, 2) * controlPoint3.x + pow(t, 3) * controlPoint4.x;
	manY = pow(1 - t, 3) * controlPoint1.y + pow(1 - t, 2) * 3 * t * controlPoint2.y + (1 - t) * pow(3 * t, 2) * controlPoint3.y + pow(t, 3) * controlPoint4.y;
	manZ = pow(1 - t, 3) * controlPoint1.z + pow(1 - t, 2) * 3 * t * controlPoint2.z + (1 - t) * pow(3 * t, 2) * controlPoint3.z + pow(t, 3) * controlPoint4.z;
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	CREATE LOGO GEOMETRY - initialize loaded in OBJ geometry
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/
static void createLogoGeometry(void) {
	// Load the UV texture (DDS file is a compressed image file)
	texture1 = loadDDS("aol_logo_textured_1.DDS");
	textureId1  = glGetUniformLocation(programId, "Logo");

	normalTexture = loadBMP_custom("Logo_Norm_Map.bmp");
	normalTextureId = glGetUniformLocation(programId, "LogoNormal");
 
	// Read aol_logo.obj (vertex positions, uv texture positions, and normals) into vector of vertices to load into buffer for display
	vector<glm::vec3> positions1;
	vector<glm::vec2> textures1;
	vector<glm::vec3> normals1;
	bool load = loadOBJ("aol_logo_textured_1.obj", positions1, textures1, normals1);
 
	vector<glm::vec3> indexed_positions1;
	vector<glm::vec2> indexed_textures1;
	vector<glm::vec3> indexed_normals1;
	indexVBO(positions1, textures1, normals1, indices1, indexed_positions1, indexed_textures1, indexed_normals1);
 
	// Load into vertex buffer objects to display
	glGenBuffers(1, &positions_vbo1);
	glBindBuffer(GL_ARRAY_BUFFER, positions_vbo1);
	glBufferData(GL_ARRAY_BUFFER, indexed_positions1.size() * sizeof(glm::vec3), &indexed_positions1[0], GL_STATIC_DRAW);
 
	glGenBuffers(1, &textureCoords_vbo1);
	glBindBuffer(GL_ARRAY_BUFFER, textureCoords_vbo1);
	glBufferData(GL_ARRAY_BUFFER, indexed_textures1.size() * sizeof(glm::vec2), &indexed_textures1[0], GL_STATIC_DRAW);
 
	glGenBuffers(1, &normals_vbo1);
	glBindBuffer(GL_ARRAY_BUFFER, normals_vbo1);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals1.size() * sizeof(glm::vec3), &indexed_normals1[0], GL_STATIC_DRAW);
 
	// Generate buffer for indices
	glGenBuffers(1, &indexBuffer1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer1);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices1.size() * sizeof(unsigned short), &indices1[0] , GL_STATIC_DRAW);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	CREATE MAN GEOMETRY - initialize loaded in OBJ geometry
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/
static void createManGeometry(void) {
	// Load the UV texture (DDS file is a compressed image file)
	texture2 = loadDDS("aol_man_textured_1.DDS");
	textureId2 = glGetUniformLocation(programId, "Man");

	// Read aol_logo.obj (vertex positions, uv texture positions, and normals) into vector of vertices to load into buffer for display
	vector<glm::vec3> positions2;
	vector<glm::vec2> textures2;
	vector<glm::vec3> normals2;
	bool load = loadOBJ("aol_man_textured_1.obj", positions2, textures2, normals2);

	vector<glm::vec3> indexed_positions2;
	vector<glm::vec2> indexed_textures2;
	vector<glm::vec3> indexed_normals2;
	indexVBO(positions2, textures2, normals2, indices2, indexed_positions2, indexed_textures2, indexed_normals2);

	// Load into vertex buffer objects to display
	glGenBuffers(1, &positions_vbo2);
	glBindBuffer(GL_ARRAY_BUFFER, positions_vbo2);
	glBufferData(GL_ARRAY_BUFFER, indexed_positions2.size() * sizeof(glm::vec3), &indexed_positions2[0], GL_STATIC_DRAW);

	glGenBuffers(1, &textureCoords_vbo2);
	glBindBuffer(GL_ARRAY_BUFFER, textureCoords_vbo2);
	glBufferData(GL_ARRAY_BUFFER, indexed_textures2.size() * sizeof(glm::vec2), &indexed_textures2[0], GL_STATIC_DRAW);

	glGenBuffers(1, &normals_vbo2);
	glBindBuffer(GL_ARRAY_BUFFER, normals_vbo2);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals2.size() * sizeof(glm::vec3), &indexed_normals2[0], GL_STATIC_DRAW);

	// Generate buffer for indices
	glGenBuffers(1, &indexBuffer2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer2);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices2.size() * sizeof(unsigned short), &indices2[0], GL_STATIC_DRAW);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	DRAW LOGO - Draw a copy of the logo (create geometry first), bool to make Logo rotate, scale, or translate over time
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/
void drawLogo(glm::vec3 position, bool rotating, bool scaling, bool translating) {
	
	float velocity = (float)(magnitude * 0.005f * elapsedFrames);

	// Load texture into buffer
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture1);
	glUniform1i(textureId1, 0);

	// Load normal map into buffer
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalTexture);
	glUniform1i(normalTextureId, 0);

	// Load vertex positions
	glBindBuffer(GL_ARRAY_BUFFER, positions_vbo1);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(vertexPosition_modelspaceID, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Load UV texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, textureCoords_vbo1);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(vertexUVID, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Load normals
	glBindBuffer(GL_ARRAY_BUFFER, normals_vbo1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(vertexNormal_modelspaceID, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Index the buffer and draw the triangles loaded from the OBJ model
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer1);

	// Rotate based on time
	glm::mat4 RotationMatrix;
	if (rotating) {
		rotationX += (float)((M_PI / 2.0f) * deltaTime * (magnitude));
		if (rotationX >= 360 * (M_PI / 180)) {
			rotationX = 0;
		}
		rotationAngleDegree = (float)(rotationX * (180 / M_PI));
		RotationMatrix = eulerAngleYXZ(rotationX, 0.0f, 0.0f);
	} else {
		RotationMatrix = eulerAngleYXZ(0.0f, 0.0f, 0.0f);
	}

	// Scale based on abs Sin (0 to 1)
	glm::mat4 ScalingMatrix;
	if (scaling) {
		scalingValue = (float)(abs(sin(velocity)*1.0f));
		ScalingMatrix = scale(mat4(), vec3(scalingValue, scalingValue, scalingValue));
	} else {
		ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
	}

	// Translate based on Sin (-1 to 1)
	glm::mat4 TranslationMatrix;
	if (translating) {
		translationValue = (float)(sin(velocity)*1.0f);
		TranslationMatrix = translate(mat4(), vec3(position.x + translationValue, position.y, position.z));
	} else {
		TranslationMatrix = translate(mat4(), position);
	}

	glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
	glm::mat4 MVP = projectionMatrix * viewMatrix * ModelMatrix;

	// Send transformation information to current bound shader
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &viewMatrix[0][0]);

	// Draw Logo
	glDrawElements(GL_TRIANGLES, indices1.size(), GL_UNSIGNED_SHORT, nullptr);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	DRAW MAN - Draw a copy of the man (create geometry first), bool to make Man rotate, scale, or translate over time
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/
void drawMan(glm::vec3 position, bool rotating, bool scaling, bool translating) {

	float velocity = (float)(magnitude * 0.005f * elapsedFrames);

	// Load into buffer
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture2);
	glUniform1i(textureId2, 0);

	glBindBuffer(GL_ARRAY_BUFFER, positions_vbo2);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(vertexPosition_modelspaceID, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, textureCoords_vbo2);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(vertexUVID, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, normals_vbo2);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(vertexNormal_modelspaceID, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Index the buffer and draw the triangles loaded from the OBJ model
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer2);

	// Rotate based on time
	glm::mat4 RotationMatrix;
	if (rotating) {
		rotationZ += (float)((M_PI / 2.0f) * deltaTime * (magnitude*2));
		if (rotationZ >= 360 * (M_PI / 180)) {
			rotationZ = 0;
		}
		rotationAngleDegree = (float)(rotationZ * (180 / M_PI));
		RotationMatrix = eulerAngleYXZ(0.0f, 0.0f, -rotationZ);
	}
	else {
		RotationMatrix = eulerAngleYXZ(0.0f, 0.0f, 0.0f);
	}

	// Scale based on abs Sin (0 to 1)
	glm::mat4 ScalingMatrix;
	if (scaling) {
		float scalingValue = (float)(abs(sin(velocity)*1.0f));
		ScalingMatrix = scale(mat4(), vec3(scalingValue, scalingValue, scalingValue));
	}
	else {
		ScalingMatrix = scale(mat4(), vec3(1.0f, 1.0f, 1.0f));
	}

	// Translate in sin/cos curve
	glm::mat4 TranslationMatrix;
	if (translating) {
		//float manX = (float)(cos(velocity)*0.5f); // AoL man in sin/cos movement
		//float manY = (float)(sin(velocity)*0.5f);
		moveBezierPath(abs(sin(velocity*0.25)));	// call function to set manX, manY, manZ to part of bezier curve specified in time (sin 0 to 1)
		TranslationMatrix = translate(mat4(), vec3(manX, manY, manZ));
	}
	else {
		TranslationMatrix = translate(mat4(), position);
	}

	glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
	glm::mat4 MVP = projectionMatrix * viewMatrix * ModelMatrix;

	// Send transformation information to current bound shader
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &viewMatrix[0][0]);

	// Draw Man
	glDrawElements(GL_TRIANGLES, indices2.size(), GL_UNSIGNED_SHORT, nullptr);
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	INIT WINDOWS - initialize and set up windows and the cursor
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/
static int initWindows(int screenWidth, int screenHeight) {

	// Initialize GLFW and its configurations
	if (!glfwInit())
	{
		fprintf(stderr, "[WARNING] Failed to initialize GLFW!\n");
		getchar();
		return -1;	// Return error
	}
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	if (FULL_SCREEN)
		getScreenResolution(screenWidth, screenHeight);

	// Open window based on screen resolution
	window = glfwCreateWindow(screenWidth, screenHeight, "CSCI 3090U Graphics Project - America Online Logo", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "[WARNING] Failed to open GLFW window. This project uses OpenGL 3.3 and some Graphics Cards might not be compatible (ie. older Intel CPUs).\n");
		getchar();
		glfwTerminate();
		return -1;	// Return error
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "[WARNING] Failed to initialize GLEW.\n");
		getchar();
		glfwTerminate();
		return -1;	// Return error
	}

	// Initialize Tw Bar in Top Left for debug statistics
	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(screenWidth, screenHeight);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	// Hide the mouse and enable unlimited mouvement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwPollEvents();

	// TwBar from external library to display debug variables, refreshes every 0.1 seconds
	TwBar * EulerGUI = TwNewBar("AoL Logo Variables");
	TwSetParam(EulerGUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	TwAddVarRW(EulerGUI, "Rotation (deg)", TW_TYPE_FLOAT, &rotationAngleDegree, "step=1.0");
	TwAddVarRW(EulerGUI, "Scaling factor", TW_TYPE_FLOAT, &scalingValue, "step=0.01");
	TwAddVarRW(EulerGUI, "Translation", TW_TYPE_FLOAT, &translationValue, "step=0.01");

	return 0;
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	RENDER - render setup and constants (ie. background colour)
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/
static void render(void) {
	// Create specified background colour
	glClearColor(BG_RED, BG_GREEN, BG_BLUE, 1.0);

	// Use our shader
	glUseProgram(programId);

	// Turn on depth buffering
	glEnable(GL_DEPTH_TEST);

	// Discards all fragments except for closest to camera
	glDepthFunc(GL_LESS);

	// Cull triangles with normals not facing camera view
	glEnable(GL_CULL_FACE);

	// Position of our light
	glUseProgram(programId);
	LightID = glGetUniformLocation(programId, "LightPosition_worldspace");
	lightPos = glm::vec3(LIGHT_X, LIGHT_Y, LIGHT_Z);
	glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

	// Handle for model-view-projection MVP (adapted from method in link specified at the top of program)
	MatrixID = glGetUniformLocation(programId, "MVP");
	ViewMatrixID = glGetUniformLocation(programId, "V");
	ModelMatrixID = glGetUniformLocation(programId, "M");

	// Handle for buffers (adapted from method in link specified at the top of program)
	vertexPosition_modelspaceID = glGetAttribLocation(programId, "vertexPosition_modelspace");
	vertexUVID = glGetAttribLocation(programId, "vertexUV");
	vertexNormal_modelspaceID = glGetAttribLocation(programId, "vertexNormal_modelspace");
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	UPDATE FUNCTION - Repeatedly run to transform, scale, rotate objects, etc.
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/
static void update(void) {
	// Update time/deltaTime values
	currentTime = glfwGetTime();
	deltaTime = (float)(currentTime - lastFrameTime);
	lastFrameTime = currentTime;
	elapsedFrames++; // For scaling/translation/rotation over time

	// Clear screen so we can swap the buffer and draw new screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Compute matrix from mouse/keyboard input for camera control
	computeMatricesFromInputs();

	//glm::mat4 projectionMatrix = getProjectionMatrix();
	//glm::mat4 ViewMatrix = getViewMatrix();
	projectionMatrix = glm::perspective(0.75f, 1.25f, 0.1f, 100.0f);
	viewMatrix = glm::lookAt(
		glm::vec3(0, 0, 10), // Camera 
		glm::vec3(0, 0, 0),  // Look At
		glm::vec3(0, 1, 0)   // Head up
	);

	glm::mat4 modelMatrix = glm::mat4(1.0);
	glm::mat4 MVP = projectionMatrix * viewMatrix * modelMatrix;

	// Draw Logos
	drawLogo(vec3(-1.0f, 1.0f, 0.0f), false, false, false);	// Draw a [static]		logo in top left
	drawLogo(vec3(2.0f, 1.0f, 0.0f), true, false, false);	// Draw a [rotating]	logo in top right
	drawLogo(vec3(-1.0f, -2.0f, 0.0f), false, true, false);	// Draw a [scaling]		logo in bottom left
	drawLogo(vec3(2.0f, -2.0f, 0.0f), false, false, true);	// Draw a [translating] logo in bottom right

	drawMan(vec3(-3.0f, -2.0f, 0.2f), true, false, true);	// Draw a [rotating] and [translating] man in bottom right

	// Draw the Tw Bar window (debug variable display)
	TwDraw();

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	// Buffer swap
	glfwSwapBuffers(window);
	glfwPollEvents();
}
// ----------------------------------------------------------------------------------------------------------------------------------------------------

/*
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*	MAIN FUNCTION
* -----------------------------------------------------------------------------------------------------------------------------------------------------
*/
int main(void)
{
	// Initialize GLFW Windows
	initWindows(SCREEN_WIDTH, SCREEN_HEIGHT);

	// Initialize Vertex and Fragment Shaders
	programId = LoadShaders("shaders/vertex.glsl", "shaders/fragment.glsl");
		// To-do: Normal map shaders for our normal maps

	// Initialize Rendering
	render();

	// Initialize Logo Geometry
	createLogoGeometry();
	createManGeometry();
 
	// Time computation (adapted from method in link specified at the top of program)
	lastTime = glfwGetTime();
	lastFrameTime = lastTime;

	// The program will enter this loop and will continue to run and translate/rotate/scale objects over time until ESC key is pressed
	do{
		update();
	} while (glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
 
	// Loop escaped, ending program, clean vertex buffer object, shader, avoid memory leaks
	glDeleteBuffers(1, &positions_vbo1);
	glDeleteBuffers(1, &textureCoords_vbo1);
	glDeleteBuffers(1, &normals_vbo1);
	glDeleteBuffers(1, &indexBuffer1);
	glDeleteProgram(programId);
	glDeleteTextures(1, &texture1);
 
	// Close both windows
	TwTerminate();
	glfwTerminate();
 
	return 0;
}
 
