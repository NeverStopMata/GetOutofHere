// Include standard headers
#include <stdlib.h>
#include <iostream>
#include <vector>
#include "common\al.h"
#include "common\alc.h"

// Include GLEW
#include "GL\glew.h"
// Include GLFW
#include "GLFW\glfw3.h"
GLFWwindow* window;

// Include GLM
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/norm.hpp"
using namespace glm;
using namespace std;


#include "common\shader.hpp"
#include "common\texture.hpp"
#include "common\ghost.h"
#include "common\controls.hpp"
#include "common\objloader.hpp"
#include "common\vboindexer.hpp"
#include "common\text2D.hpp"
#include "common\quaternion_utils.hpp"
#include "common\tangentspace.hpp"
#include "common/particle.hpp"
#include "common/spirit.hpp"

ALuint alSource;
ALuint source;
ALsizei size2;
ALsizei frequent;
ALenum format;

//unsigned char* buf;
//void initAL(char* filename);
//void deleteAL();

struct WAVE_Data {
	char subChunkID[4]; //should contain the word data  
	long subChunk2Size; //Stores the size of the data block  
};

struct WAVE_Format {
	char subChunkID[4];
	long subChunkSize;
	short audioFormat;
	short numChannels;
	long sampleRate;
	long byteRate;
	short blockAlign;
	short bitsPerSample;
};

struct RIFF_Header {
	char chunkID[4];
	long chunkSize;//size not including chunkSize or chunkID  
	char format[4];
};

bool loadWavFile(const std::string filename, ALuint* buffer,
	ALsizei* size, ALsizei* frequency,
	ALenum* format) {
	//Local Declarations  
	FILE* soundFile = NULL;
	WAVE_Format wave_format;
	RIFF_Header riff_header;
	WAVE_Data wave_data;
	unsigned char* data;

	try {
		soundFile = fopen(filename.c_str(), "rb");
		if (!soundFile)
			throw (filename);

		// Read in the first chunk into the struct  
		fread(&riff_header, sizeof(RIFF_Header), 1, soundFile);

		//check for RIFF and WAVE tag in memeory  
		if ((riff_header.chunkID[0] != 'R' ||
			riff_header.chunkID[1] != 'I' ||
			riff_header.chunkID[2] != 'F' ||
			riff_header.chunkID[3] != 'F') ||
			(riff_header.format[0] != 'W' ||
				riff_header.format[1] != 'A' ||
				riff_header.format[2] != 'V' ||
				riff_header.format[3] != 'E'))
			throw ("Invalid RIFF or WAVE Header");

		//Read in the 2nd chunk for the wave info  
		fread(&wave_format, sizeof(WAVE_Format), 1, soundFile);
		//check for fmt tag in memory  
		if (wave_format.subChunkID[0] != 'f' ||
			wave_format.subChunkID[1] != 'm' ||
			wave_format.subChunkID[2] != 't' ||
			wave_format.subChunkID[3] != ' ')
			throw ("Invalid Wave Format");

		//check for extra parameters;  
		if (wave_format.subChunkSize > 16)
			fseek(soundFile, sizeof(short), SEEK_CUR);

		//Read in the the last byte of data before the sound file  
		fread(&wave_data, sizeof(WAVE_Data), 1, soundFile);
		//check for data tag in memory  
		if (wave_data.subChunkID[0] != 'd' ||
			wave_data.subChunkID[1] != 'a' ||
			wave_data.subChunkID[2] != 't' ||
			wave_data.subChunkID[3] != 'a')
			throw ("Invalid data header");

		//Allocate memory for data  
		data = new unsigned char[wave_data.subChunk2Size];

		// Read in the sound data into the soundData variable  
		if (!fread(data, wave_data.subChunk2Size, 1, soundFile))
			throw ("error loading WAVE data into struct!");

		//Now we set the variables that we passed in with the  
		//data from the structs  
		*size = wave_data.subChunk2Size;
		*frequency = wave_format.sampleRate;
		//The format is worked out by looking at the number of  
		//channels and the bits per sample.  
		if (wave_format.numChannels == 1) {
			if (wave_format.bitsPerSample == 8)
				*format = AL_FORMAT_MONO8;
			else if (wave_format.bitsPerSample == 16)
				*format = AL_FORMAT_MONO16;
		}
		else if (wave_format.numChannels == 2) {
			if (wave_format.bitsPerSample == 8)
				*format = AL_FORMAT_STEREO8;
			else if (wave_format.bitsPerSample == 16)
				*format = AL_FORMAT_STEREO16;
		}
		//create our openAL buffer and check for success  
		alGenBuffers(1, buffer);
		//errorCheck();  
		//now we put our data into the openAL buffer and  
		//check for success  
		alBufferData(*buffer, *format, (void*)data,
			*size, *frequency);
		//errorCheck();  
		//clean up and return true if successful  
		fclose(soundFile);
		return true;
	}
	catch (std::string error) {

		//clean up memory if wave loading fails  
		if (soundFile != NULL)
			fclose(soundFile);
		//return false to indicate the failure to load wave  
		return false;
	}
}
int main(void)
{
	alcMakeContextCurrent(alcCreateContext(alcOpenDevice(NULL), NULL));
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1920, 1080, "hogwardz", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// We would expect width and height to be 1024 and 768
	int windowWidth = 1920;
	int windowHeight = 1080;
	// But on MacOS X with a retina screen it'll be 1024*2 and 768*2, so we get the actual framebuffer size:
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	std::cout << windowWidth << std::endl;
	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	// Hide the mouse and enable unlimited mouvement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	//depth
	glClearDepth(1.0f);
	glClearStencil(0);
	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);//去掉背面的显示

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	/*========================================用于显示结果的着色器的定义=====================================*/
	GLuint quad_programID = LoadShaders("shader/Passthrough.vertexshader", "shader/SimpleTexture.fragmentshader");
	GLuint coltexID = glGetUniformLocation(quad_programID, "simpletexture"); 
	GLuint inftexID = glGetUniformLocation(quad_programID, "infortexture"); 
	GLuint britexID = glGetUniformLocation(quad_programID, "brightexture");
	
	GLuint endtexID = glGetUniformLocation(quad_programID, "endtexture");
	GLuint SpeedinQuadID = glGetUniformLocation(quad_programID, "speed"); 
	GLuint MadnessID = glGetUniformLocation(quad_programID, "madness");

	/*========================================用于阴影映射的着色器的定义=====================================*/
	GLuint depthProgramID = LoadShaders("shader/DepthRTT.vertexshader", "shader/DepthRTT.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint depthMatrixID = glGetUniformLocation(depthProgramID, "depthMVP");

	/*========================================用于高斯水平模糊的着色器的定义=====================================*/
	GLuint HGBProgramID = LoadShaders("shader/Passthrough.vertexshader", "shader/HorizGB.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint srcGBtexID = glGetUniformLocation(HGBProgramID, "srcGBtexture");
	GLuint blurBiasID = glGetUniformLocation(HGBProgramID, "horizBias");
	GLuint blurDirecID = glGetUniformLocation(HGBProgramID, "blurDire");
	/*========================================用于绘制场景的着色器的定义=====================================*/
	GLuint programID = LoadShaders("shader/StandardShading.vertexshader", "shader/StandardShading.fragmentshader");
	// Get a handle for our "MVP" uniform
	GLuint MVPMatrixID = glGetUniformLocation(programID, "MVP");
	GLuint DepthBiasMVPID = glGetUniformLocation(programID, "DepthBiasMVP");
	GLuint DepthMVPID = glGetUniformLocation(programID, "DepthMVP");
	GLuint DepthMVID = glGetUniformLocation(programID, "DepthMV");
	GLuint ZNearID = glGetUniformLocation(programID, "ZNear");
	GLuint ZFarID = glGetUniformLocation(programID, "ZFar");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");
	GLuint ModelView3x3MatrixID = glGetUniformLocation(programID, "MV3x3");

	GLuint isShaderID = glGetUniformLocation(programID, "isShader");
	GLuint haveTorchID = glGetUniformLocation(programID, "haveTorch");
	GLuint classNrID = glGetUniformLocation(programID, "classNr"); 
	GLuint lightPowerID = glGetUniformLocation(programID, "lightPower");
	// Get a handle for our buffers
	GLuint vertexPosition_modelspaceID = glGetAttribLocation(programID, "vertexPosition_modelspace");
	GLuint vertexUVID = glGetAttribLocation(programID, "vertexUV");
	GLuint vertexNormal_modelspaceID = glGetAttribLocation(programID, "vertexNormal_modelspace");
	GLuint EyePosID = glGetUniformLocation(programID, "eyePosWP");
	GLuint DiffuseTextureID = glGetUniformLocation(programID, "DiffuseTextureSampler");
	GLuint NormalTextureID = glGetUniformLocation(programID, "NormalTextureSampler");
	GLuint SpecularTextureID = glGetUniformLocation(programID, "SpecularTextureSampler");
	GLuint ShadowMapID = glGetUniformLocation(programID, "shadowMap");
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	GLuint CameraID = glGetUniformLocation(programID, "CameraPos_worldspace");
	GLuint TorchID = glGetUniformLocation(programID, "TorchPosition_worldspace");
	GLuint TorchDirecID = glGetUniformLocation(programID, "TorchDirection_worldspace");
	GLuint SpeedID = glGetUniformLocation(programID, "speed");
	GLuint specDegreeID = glGetUniformLocation(programID, "specDegree");
	GLuint diffDegreeID = glGetUniformLocation(programID, "diffDegree");
	GLuint distScanID = glGetUniformLocation(programID, "distScan");

	/*============================================用于画鬼的着色器的定义===============================*/
	// Create and compile our GLSL program from the shaders
	GLuint particleprogramID = LoadShaders("shader/Particle.vertexshader", "shader/Particle.fragmentshader");

	// Vertex shader
	GLuint CameraRight_worldspace_ID = glGetUniformLocation(particleprogramID, "CameraRight_worldspace");
	GLuint CameraUp_worldspace_ID = glGetUniformLocation(particleprogramID, "CameraUp_worldspace");
	GLuint CameraPos_worldspace_ID = glGetUniformLocation(particleprogramID, "CameraPos_worldspace");
	GLuint Center_worldspace_ID = glGetUniformLocation(particleprogramID, "Center_worldspace");
	GLuint ViewProjMatrixID = glGetUniformLocation(particleprogramID, "VP");
	// fragment shader
	GLuint particleTextureID = glGetUniformLocation(particleprogramID, "myTextureSampler");
	
	static const GLfloat g_vertex_buffer_data[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f,
		0.5f,  0.5f, 0.0f,
	};
	GLuint billboard_vertex_buffer;
	glGenBuffers(1, &billboard_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);



	/*============================================结束画鬼的着色器的定义===============================*/
	//GLuint Texture = loadDDS("particle.DDS");


	// Load the texture
	//GLuint DiffuseTexture_floor = loadBMP_custom("floor_COLOR.bmp");
	GLuint DiffuseTexture_floor = loadBMP_custom("texture/floor_COLOR.bmp");
	GLuint NormalTexture_floor = loadBMP_custom("texture/floor_NRM.bmp");
	GLuint SpecularTexture_floor = loadBMP_custom("texture/floor_SPEC.bmp");

	GLuint DiffuseTexture_wall = loadBMP_custom("texture/wall_COLOR.bmp");
	GLuint NormalTexture_wall = loadBMP_custom("texture/wall_NRM.bmp");
	GLuint SpecularTexture_wall = loadBMP_custom("texture/wall_SPEC.bmp");

	GLuint TextureCar = loadBMP_custom("texture/purecar.bmp");
	GLuint TextureTyre = loadBMP_custom("texture/tyre.bmp");
	GLuint TextureLightgun = loadBMP_custom("texture/purewhite.bmp");
	GLuint TextureParticle = loadBMP_custom("texture/skull.bmp");
	GLuint TextureEnd = loadBMP_custom("texture/end.bmp");
	//glBindFragDataLocation(programID, 0, "color");
	//glBindFragDataLocation(programID, 1, "information");
	// Read our hogwards.obj file
	std::vector<glm::vec3> vertices_floor;
	std::vector<glm::vec2> uvs_floor;
	std::vector<glm::vec3> normals_floor;
	//bool res = loadOBJ("colorplayground.obj", false, vertices_floor, uvs_floor, normals_floor);
	bool res = loadOBJ("obj/floor.obj", false, vertices_floor, uvs_floor, normals_floor);

	std::vector<glm::vec3> tangents;
	std::vector<glm::vec3> bitangents;
	computeTangentBasis(
		vertices_floor, uvs_floor, normals_floor, // input
		tangents, bitangents    // output
		);

	//models[1].GenerateAnything("colorplayground.obj");
	std::vector<unsigned short> indices_floor;
	std::vector<glm::vec3> indexed_vertices_floor;
	std::vector<glm::vec2> indexed_uvs_floor;
	std::vector<glm::vec3> indexed_normals_floor;
	std::vector<glm::vec3> indexed_tangents_floor;
	std::vector<glm::vec3> indexed_bitangents_floor;
	indexVBO_TBN(
		vertices_floor, uvs_floor, normals_floor, tangents, bitangents,
		indices_floor, indexed_vertices_floor, indexed_uvs_floor, indexed_normals_floor, indexed_tangents_floor, indexed_bitangents_floor
		);

	
	/*================================================================================load floor.obj ^========================================================*/
	std::vector<glm::vec3> vertices_wall;
	std::vector<glm::vec2> uvs_wall;
	std::vector<glm::vec3> normals_wall;
	bool res_2 = loadOBJ("obj/map_wall.obj", false, vertices_wall, uvs_wall, normals_wall);

	std::vector<glm::vec3> tangents_wall;
	std::vector<glm::vec3> bitangents_wall;
	computeTangentBasis(
		vertices_wall, uvs_wall, normals_wall, // input
		tangents_wall, bitangents_wall    // output
		);

	//models[1].GenerateAnything("colorplayground.obj");
	std::vector<unsigned short> indices_wall;
	std::vector<glm::vec3> indexed_vertices_wall;
	std::vector<glm::vec2> indexed_uvs_wall;
	std::vector<glm::vec3> indexed_normals_wall;
	std::vector<glm::vec3> indexed_tangents_wall;
	std::vector<glm::vec3> indexed_bitangents_wall;
	indexVBO_TBN(
		vertices_wall, uvs_wall, normals_wall, tangents_wall, bitangents_wall,
		indices_wall, indexed_vertices_wall, indexed_uvs_wall, indexed_normals_wall, indexed_tangents_wall, indexed_bitangents_wall
		);
	/*==================================================================================load wall.obj ^=========================================================*/

	// Read our car.obj file
	std::vector<glm::vec3> vertices_car;
	std::vector<glm::vec2> uvs_car;
	std::vector<glm::vec3> normals_car;
	bool res_car = loadOBJ("obj/car.obj", false, vertices_car, uvs_car, normals_car);
	//	models[0].GenerateAnything("car.obj");
	std::vector<unsigned short> indices_car;
	std::vector<glm::vec3> indexed_vertices_car;
	std::vector<glm::vec2> indexed_uvs_car;
	std::vector<glm::vec3> indexed_normals_car;
	indexVBO(vertices_car, uvs_car, normals_car, indices_car, indexed_vertices_car, indexed_uvs_car, indexed_normals_car);

	std::vector<glm::vec3> vertices_tyre;
	std::vector<glm::vec2> uvs_tyre;
	std::vector<glm::vec3> normals_tyre;
	bool res_tyre = loadOBJ("obj/tyre.obj", false, vertices_tyre, uvs_tyre, normals_tyre);
	//	models[0].GenerateAnything("tyre.obj");
	std::vector<unsigned short> indices_tyre;
	std::vector<glm::vec3> indexed_vertices_tyre;
	std::vector<glm::vec2> indexed_uvs_tyre;
	std::vector<glm::vec3> indexed_normals_tyre;
	indexVBO(vertices_tyre, uvs_tyre, normals_tyre, indices_tyre, indexed_vertices_tyre, indexed_uvs_tyre, indexed_normals_tyre);

	std::vector<glm::vec3> vertices_lightgun;
	std::vector<glm::vec2> uvs_lightgun;
	std::vector<glm::vec3> normals_lightgun;
	bool res_lightgun = loadOBJ("obj/lightgun.obj", true, vertices_lightgun, uvs_lightgun, normals_lightgun);
	//	models[0].GenerateAnything("lightgun.obj");
	std::vector<unsigned short> indices_lightgun;
	std::vector<glm::vec3> indexed_vertices_lightgun;
	std::vector<glm::vec2> indexed_uvs_lightgun;
	std::vector<glm::vec3> indexed_normals_lightgun;
	indexVBO(vertices_lightgun, uvs_lightgun, normals_lightgun, indices_lightgun, indexed_vertices_lightgun, indexed_uvs_lightgun, indexed_normals_lightgun);
	/*------------------------------------------------------------------------------------*/
	glm::vec3 sunPos = glm::vec3(50, 10, 50);
	glm::vec3 lightPos;
	vec3 lightPos_modelSpace;
	initText2D("texture/Holstein.DDS");
	float lastTime = glfwGetTime();
	float lastTime2 = glfwGetTime();
	int nbFrames = 0;
	int nbF4motionblur = 0;
	int motionblurDeep = 20;
	float currentTime = 0.0, spf = 100.0;
	float loopTime = 0.0;
	float subTime = 0.0;
	float startTime = 0.0;
	float endTime = 0.0;
	int hit_times = 0;
	float OldXpos = 0, OldYpos = 0;
	float zFarCmra = 0.0;
	float zNearCmra = 0.0;
	glm::vec3 position = glm::vec3(-22, 1, 0);
	vec3 camera_org;
	glm::mat4 ProjectionMatrix;
	glm::mat4 ViewMatrix;
	glm::mat4 ViewMatrix_backup;
	mat4 ModelMatrix_car;
	mat4 ModelMatrixLightGun;
	mat4 MVP_car;
	mat4 MVP_lightgun;

	mat4 * ModelMatrixTyreArray = new mat4[4];
	mat4 * MVPtyreArray = new mat4[4];


	vec3 v_car;
	bool lightOn = false;
	bool useSV = false;
	std::vector<glm::vec3> normal_surface;

	bool existSkull = false;
	spirit skull;
	GLuint particles_position_buffer;
	GLuint particles_color_buffer;
	
	/*------------------------------------------------------------------------------------*/

	// Load it into a VBO

	GLuint vertexbuffer_floor;
	GLuint uvbuffer_floor;
	GLuint normalbuffer_floor;
	GLuint tangentbuffer_floor;
	GLuint bitangentbuffer_floor;
	GLuint elementbuffer_floor;
	glGenBuffers(1, &vertexbuffer_floor);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_floor);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices_floor.size() * sizeof(glm::vec3), &indexed_vertices_floor[0], GL_STATIC_DRAW);


	
	glGenBuffers(1, &uvbuffer_floor);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_floor);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs_floor.size() * sizeof(glm::vec2), &indexed_uvs_floor[0], GL_STATIC_DRAW);

	
	glGenBuffers(1, &normalbuffer_floor);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_floor);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals_floor.size() * sizeof(glm::vec3), &indexed_normals_floor[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices_floor as well
	glGenBuffers(1, &tangentbuffer_floor);
	glBindBuffer(GL_ARRAY_BUFFER, tangentbuffer_floor);
	glBufferData(GL_ARRAY_BUFFER, indexed_tangents_floor.size() * sizeof(glm::vec3), &indexed_tangents_floor[0], GL_STATIC_DRAW);

	
	glGenBuffers(1, &bitangentbuffer_floor);
	glBindBuffer(GL_ARRAY_BUFFER, bitangentbuffer_floor);
	glBufferData(GL_ARRAY_BUFFER, indexed_bitangents_floor.size() * sizeof(glm::vec3), &indexed_bitangents_floor[0], GL_STATIC_DRAW);

	glGenBuffers(1, &elementbuffer_floor);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_floor);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_floor.size() * sizeof(unsigned short), &indices_floor[0], GL_STATIC_DRAW);



	GLuint vertexbuffer_wall;
	GLuint uvbuffer_wall;
	GLuint normalbuffer_wall;
	GLuint tangentbuffer_wall;
	GLuint bitangentbuffer_wall;
	GLuint elementbuffer_wall;
	glGenBuffers(1, &vertexbuffer_wall);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_wall);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices_wall.size() * sizeof(glm::vec3), &indexed_vertices_wall[0], GL_STATIC_DRAW);



	glGenBuffers(1, &uvbuffer_wall);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_wall);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs_wall.size() * sizeof(glm::vec2), &indexed_uvs_wall[0], GL_STATIC_DRAW);


	glGenBuffers(1, &normalbuffer_wall);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_wall);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals_wall.size() * sizeof(glm::vec3), &indexed_normals_wall[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices_wall as well
	glGenBuffers(1, &tangentbuffer_wall);
	glBindBuffer(GL_ARRAY_BUFFER, tangentbuffer_wall);
	glBufferData(GL_ARRAY_BUFFER, indexed_tangents_wall.size() * sizeof(glm::vec3), &indexed_tangents_wall[0], GL_STATIC_DRAW);


	glGenBuffers(1, &bitangentbuffer_wall);
	glBindBuffer(GL_ARRAY_BUFFER, bitangentbuffer_wall);
	glBufferData(GL_ARRAY_BUFFER, indexed_bitangents_wall.size() * sizeof(glm::vec3), &indexed_bitangents_wall[0], GL_STATIC_DRAW);

	glGenBuffers(1, &elementbuffer_wall);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_wall);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_wall.size() * sizeof(unsigned short), &indices_wall[0], GL_STATIC_DRAW);

	/*==================================================================wall======================================================*/


	GLuint vertexbuffer_car;
	glGenBuffers(1, &vertexbuffer_car);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_car);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices_car.size() * sizeof(glm::vec3), &indexed_vertices_car[0], GL_DYNAMIC_DRAW);
	GLuint uvbuffer_car;
	glGenBuffers(1, &uvbuffer_car);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_car);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs_car.size() * sizeof(glm::vec2), &indexed_uvs_car[0], GL_DYNAMIC_DRAW);

	GLuint normalbuffer_car;
	glGenBuffers(1, &normalbuffer_car);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_car);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals_car.size() * sizeof(glm::vec3), &indexed_normals_car[0], GL_DYNAMIC_DRAW);

	// Generate a buffer for the indices_floor as well
	GLuint elementbuffer_car;
	glGenBuffers(1, &elementbuffer_car);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_car);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_car.size() * sizeof(unsigned short), &indices_car[0], GL_DYNAMIC_DRAW);

	//Generate buffers for the tyre as well/////////////////
	GLuint vertexbuffer_tyre;
	glGenBuffers(1, &vertexbuffer_tyre);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_tyre);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices_tyre.size() * sizeof(glm::vec3), &indexed_vertices_tyre[0], GL_DYNAMIC_DRAW);



	GLuint uvbuffer_tyre;
	glGenBuffers(1, &uvbuffer_tyre);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_tyre);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs_tyre.size() * sizeof(glm::vec2), &indexed_uvs_tyre[0], GL_DYNAMIC_DRAW);

	GLuint normalbuffer_tyre;
	glGenBuffers(1, &normalbuffer_tyre);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_tyre);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals_tyre.size() * sizeof(glm::vec3), &indexed_normals_tyre[0], GL_DYNAMIC_DRAW);

	// Generate a buffer for the indices_floor as well
	GLuint elementbuffer_tyre;
	glGenBuffers(1, &elementbuffer_tyre);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_tyre);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_tyre.size() * sizeof(unsigned short), &indices_tyre[0], GL_DYNAMIC_DRAW);

	GLuint vertexbuffer_lightgun;
	glGenBuffers(1, &vertexbuffer_lightgun);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_lightgun);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices_lightgun.size() * sizeof(glm::vec3), &indexed_vertices_lightgun[0], GL_DYNAMIC_DRAW);

	GLuint uvbuffer_lightgun;
	glGenBuffers(1, &uvbuffer_lightgun);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_lightgun);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs_lightgun.size() * sizeof(glm::vec2), &indexed_uvs_lightgun[0], GL_DYNAMIC_DRAW);

	GLuint normalbuffer_lightgun;
	glGenBuffers(1, &normalbuffer_lightgun);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_lightgun);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals_lightgun.size() * sizeof(glm::vec3), &indexed_normals_lightgun[0], GL_DYNAMIC_DRAW);

	// Generate a buffer for the indices_floor as well
	GLuint elementbuffer_lightgun;
	glGenBuffers(1, &elementbuffer_lightgun);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_lightgun);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_lightgun.size() * sizeof(unsigned short), &indices_lightgun[0], GL_DYNAMIC_DRAW);
	//shadowV/////////////////////////////////////////////////////////////////////////////////////////////////////
	GLuint shadowVB_car;
	glGenBuffers(1, &shadowVB_car);
	glBindBuffer(GL_ARRAY_BUFFER, shadowVB_car);
	ghost carBody(vec3(0, 2, 0), vertices_floor.size() / 3 + vertices_wall.size() / 3);
	/*=======================================================阴影帧缓冲创建======================================================*/
	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	GLuint FB4Depth = 0;
	glGenFramebuffers(1, &FB4Depth);
	glBindFramebuffer(GL_FRAMEBUFFER, FB4Depth);

	// Depth texture. Slower than a depth buffer, but you can sample it later in your shader
	GLuint depthTexture;
	int depthTextureSize = 1024;
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, depthTextureSize, depthTextureSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);

	// No color output in the bound framebuffer, only depth.
	glDrawBuffer(GL_NONE);

	/******************************************创建结果帧缓存*********************************************************/
	GLuint FramebufferName;
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	int framewidth = 1920;
	int frameheight = 1080;

	GLuint rboID = 0;
	glGenRenderbuffers(1, &rboID);
	glBindRenderbuffer(GL_RENDERBUFFER, rboID);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, framewidth, frameheight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboID);

	GLuint ResTexture;
	glGenTextures(1, &ResTexture);

	glBindTexture(GL_TEXTURE_2D, ResTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framewidth, frameheight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ResTexture, 0);

	GLuint InforTexture;
	glGenTextures(1, &InforTexture);
	glBindTexture(GL_TEXTURE_2D, InforTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framewidth, frameheight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, InforTexture, 0);

	GLuint BrightTexture;
	glGenTextures(1, &BrightTexture);
	glBindTexture(GL_TEXTURE_2D, BrightTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framewidth, frameheight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, BrightTexture, 0);
	/*********************************************结果帧缓存创建完毕**************************************************/



	/******************************************创建水平高斯模糊用的帧缓存*********************************************************/
	GLuint FB4HGB;
	glGenFramebuffers(1, &FB4HGB);
	glBindFramebuffer(GL_FRAMEBUFFER, FB4HGB);

	GLuint HGBTexture;
	glGenTextures(1, &HGBTexture);
	glBindTexture(GL_TEXTURE_2D, HGBTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framewidth, frameheight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, HGBTexture, 0);

	GLuint HVGBTexture;
	glGenTextures(1, &HVGBTexture);
	glBindTexture(GL_TEXTURE_2D, HVGBTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framewidth, frameheight, 0, GL_RGBA, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, HVGBTexture, 0);
	

	/*********************************************水平高斯模糊用的帧缓存创建完毕**************************************************/

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	static const GLfloat g_quad_vertex_buffer_data[] = {
		-1, -1, 0.0f,
		1, -1, 0.0f,
		-1,  1, 0.0f,
		-1,  1, 0.0f,
		1, -1, 0.0f,
		1, 1,  0.0f,
	};
	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	std::vector<glm::vec3> vertices_envi;
	for (int i = 0; i < vertices_floor.size() / 3; i++)
	{
		vertices_envi.push_back(vertices_floor[3 * i + 0]);
		vertices_envi.push_back(vertices_floor[3 * i + 1]);
		vertices_envi.push_back(vertices_floor[3 * i + 2]);
		vec3 AB = vertices_floor[3 * i + 1] - vertices_floor[3 * i + 0];
		vec3 BC = vertices_floor[3 * i + 2] - vertices_floor[3 * i + 1];
		normal_surface.push_back(normalize(cross(AB, BC)));
	}

	for (int i = 0; i < vertices_wall.size() / 3; i++)
	{
		vertices_envi.push_back(vertices_wall[3 * i + 0]);
		vertices_envi.push_back(vertices_wall[3 * i + 1]);
		vertices_envi.push_back(vertices_wall[3 * i + 2]);
		vec3 AB = vertices_wall[3 * i + 1] - vertices_wall[3 * i + 0];
		vec3 BC = vertices_wall[3 * i + 2] - vertices_wall[3 * i + 1];
		normal_surface.push_back(normalize(cross(AB, BC)));
	}
	
	loadWavFile("music/peaceful.wav", &alSource, &size2, &frequent, &format);
	alGenSources(1, &source);
	alSourcei(source, AL_BUFFER, alSource);
	alSourcePlay(source);

	do {
		
		computeMatricesFromInputs_my(normal_surface, vertices_envi, &carBody);
		double currentTime2 = glfwGetTime();
		float delta = currentTime2 - lastTime2;
		lastTime2 = currentTime2;


		// We will need the camera's position in order to sort the particles
		// w.r.t the camera's distance.
		// There should be a getCameraPosition() function in common/controls.cpp, 
		// but this works too.






		float scanDistance = getScanDistRT();
		glBindFramebuffer(GL_FRAMEBUFFER, FB4Depth);
		glViewport(0, 0, depthTextureSize, depthTextureSize); // Render on the whole framebuffer, complete from the lower left corner to the upper right

															  // We don't use bias in the shader, but instead we draw back faces, 
															  // which are already separated from the front faces by a small distance 
															  // (if your geometry is made this way)
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT); // Cull back-facing triangles -> draw only front-facing triangles

							  // Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(depthProgramID);
		ModelMatrix_car = getModelMatrix_car();
		ModelMatrixLightGun = getModMtrxLightGun();
		ModelMatrixTyreArray[0] = getModMtrxTyre1();
		ModelMatrixTyreArray[1] = getModMtrxTyre2();
		ModelMatrixTyreArray[2] = getModMtrxTyre3();
		ModelMatrixTyreArray[3] = getModMtrxTyre4();
		//glm::vec3 lightInvDir = normalize(sunPos);
		glm::vec3 lightInvDir = normalize(sunPos - carBody.position);
		glm::mat4 depthProjectionMatrix;
		glm::mat4 depthViewMatrix;
		zFarCmra = carBody.size * 100.0f;
		zNearCmra = -carBody.size * 0.1f;
		depthProjectionMatrix = glm::ortho<float>(-0.1, 0.1, -0.1, 0.1, zNearCmra, zFarCmra);
		//
		depthViewMatrix = glm::lookAt(carBody.position + lightInvDir, carBody.position, glm::vec3(0, 1, 0));
		// or, for spot light :
		//glm::vec3 lightPos(5, 20, 20);
		//glm::mat4 depthProjectionMatrix = glm::perspective<float>(45.0f, 1.0f, 2.0f, 50.0f);
		//glm::mat4 depthViewMatrix = glm::lookAt(lightPos, lightPos-lightInvDir, glm::vec3(0,1,0));

		// Compute the MVP matrix from the light's point of view

		glm::mat4 depthMVP_car = depthProjectionMatrix * depthViewMatrix * ModelMatrix_car;
		mat4 * depthMVPtyreArray = new mat4[4];
		depthMVPtyreArray[0] = depthProjectionMatrix * depthViewMatrix * ModelMatrixTyreArray[0];
		depthMVPtyreArray[1] = depthProjectionMatrix * depthViewMatrix * ModelMatrixTyreArray[1];
		depthMVPtyreArray[2] = depthProjectionMatrix * depthViewMatrix * ModelMatrixTyreArray[2];
		depthMVPtyreArray[3] = depthProjectionMatrix * depthViewMatrix * ModelMatrixTyreArray[3];

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(depthMatrixID, 1, GL_FALSE, &depthMVP_car[0][0]);

		// 1rst attribute buffer : vertices_floor
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_car);
		glVertexAttribPointer(
			0,  // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_car);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices_car.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
			);

		glDisableVertexAttribArray(0);

		// 1rst attribute buffer : vertices_floor
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_tyre);
		glVertexAttribPointer(
			0,  // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_tyre);
		for (int i = 0; i < 4; i++)
		{
			// Send our transformation to the currently bound shader, 
			// in the "MVP" uniform
			glUniformMatrix4fv(depthMatrixID, 1, GL_FALSE, &depthMVPtyreArray[i][0][0]);



			// Draw the triangles !
			glDrawElements(
				GL_TRIANGLES,      // mode
				indices_tyre.size(),    // count
				GL_UNSIGNED_SHORT, // type
				(void*)0           // element array buffer offset
				);
		}
		glDisableVertexAttribArray(0);

		glm::mat4 depthMVP_env = depthProjectionMatrix * depthViewMatrix * mat4();
		glCullFace(GL_BACK);
		/*========================================================生成阴影贴图完成============================================*/
		/*========================================================开始在帧缓冲中离屏渲染====================================*/
		glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		GLenum TBOattaches[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, TBOattaches);
		glViewport(0, 0, framewidth, frameheight);
		startTime = glfwGetTime();
		std::vector<glm::vec3> indexed_vertices_SVcar;
		std::vector<unsigned short> indices_SVcar;
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		//depth
		glClearDepth(1.0f);
		glClearStencil(0);
		// Enable depth test
		glEnable(GL_DEPTH_TEST);
		if (useSV)
			glEnable(GL_STENCIL_TEST);
		// Accept fragment if it closer to the camera than the former one
		glDepthFunc(GL_LESS);

		// Cull triangles which normal is not towards the camera
		glEnable(GL_CULL_FACE);//去掉背面的显示
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// Cull back-facing triangles -> draw only front-facing triangles
		// Clear the screen

		// Use our shader
		glUseProgram(programID);
		lightSwitch(&lightOn);
		glUniform1f(distScanID, scanDistance);
		glUniform1f(lightPowerID, currentTime < 50.f ? 5000.0f : clamp(5000.0f - (currentTime - 50.0f) * 900.f,500.0f,5000.0f));
		glUniform1f(ZFarID, zFarCmra);
		
		glUniform1f(ZNearID, zNearCmra);
		glUniform1i(classNrID, 1);
		glUniform1i(haveTorchID, lightOn ? 1 : 0);
		glUniform1f(specDegreeID, 0.5);
		glUniform1f(diffDegreeID, 1.0);
		glUniform3f(LightID, sunPos.x, sunPos.y, sunPos.z);
		glUniform3f(EyePosID, camera_org.x, camera_org.y, camera_org.z);
		glUniform3f(TorchID, carBody.position.x, carBody.position.y, carBody.position.z);
		camera_org = getCameraOrg();
		glUniform3f(CameraID, camera_org.x, camera_org.y, camera_org.z);
		glUniform1f(SpeedID, carBody.speed);

		ProjectionMatrix = getProjectionMatrix();
		ViewMatrix = getViewMatrix();
		glm::mat4 ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		glm::mat4 ModelMatrix = mat4();
		glm::mat4 ModelViewMatrix = ViewMatrix * ModelMatrix;
		glm::mat3 ModelView3x3Matrix = glm::mat3(ModelViewMatrix);
		glm::mat4 MVP;
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		//glm::mat4 RotationMatrix_car = glm::mat4();
		glm::vec3 CarDirection = getViewDirection();
		CarDirection = normalize(CarDirection);
		//mat4 ScaleMatrix_car = scale(ModelMatrix, vec3(0.1,0.1,0.1));
		vec3 old_car_pos;
		old_car_pos = carBody.position;
		carBody.position = CarDirection + camera_org;
		v_car = carBody.position - old_car_pos;
		//float realTimeSpeed = length(v_car);

		MVP_car = ProjectionMatrix * ViewMatrix * ModelMatrix_car;
		MVP_lightgun = ProjectionMatrix * ViewMatrix * ModelMatrixLightGun;
		MVPtyreArray[0] = ProjectionMatrix * ViewMatrix * ModelMatrixTyreArray[0];
		MVPtyreArray[1] = ProjectionMatrix * ViewMatrix * ModelMatrixTyreArray[1];
		MVPtyreArray[2] = ProjectionMatrix * ViewMatrix * ModelMatrixTyreArray[2];
		MVPtyreArray[3] = ProjectionMatrix * ViewMatrix * ModelMatrixTyreArray[3];
		//if (useSV)
		//{
		//	/***************************************************************开始绘制暗影下的赛道***************************************************/
		//	glUniform1i(isShaderID, 1);
		//	glActiveTexture(GL_TEXTURE0);
		//	glBindTexture(GL_TEXTURE_2D, TextureEnvi);
		//	glUniform1i(DiffuseTextureID, 0);
		//	glEnableVertexAttribArray(0);
		//	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_floor);
		//	glVertexAttribPointer(
		//		0,                  // attribute
		//		3,                  // size
		//		GL_FLOAT,           // type
		//		GL_FALSE,           // normalized?
		//		0,                  // stride
		//		(void*)0            // array buffer offset
		//		);


		glm::mat4 biasMatrix(
			0.5, 0.0, 0.0, 0.0,
			0.0, 0.5, 0.0, 0.0,
			0.0, 0.0, 0.5, 0.0,
			0.5, 0.5, 0.5, 1.0
			);
		glm::mat4 depthBiasMVP = biasMatrix*depthMVP_env;
		glm::mat4 depthMV_env = depthViewMatrix * mat4();
		///////////////////////////////////////////////////////绘制第二次背景
		glUniform1i(classNrID, 1);//3代表液体；
		glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniformMatrix3fv(ModelView3x3MatrixID, 1, GL_FALSE, &ModelView3x3Matrix[0][0]);

		glUniformMatrix4fv(DepthBiasMVPID, 1, GL_FALSE, &depthBiasMVP[0][0]);
		glUniformMatrix4fv(DepthMVPID, 1, GL_FALSE, &depthMVP_env[0][0]);
		glUniformMatrix4fv(DepthMVID, 1, GL_FALSE, &depthMV_env[0][0]);
		glUniform3f(TorchDirecID, CarDirection.x, CarDirection.y, CarDirection.z);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, DiffuseTexture_floor);
		glUniform1i(DiffuseTextureID, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, NormalTexture_floor);
		glUniform1i(NormalTextureID, 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, DiffuseTexture_floor);
		glUniform1i(DiffuseTextureID, 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, SpecularTexture_floor);
		glUniform1i(SpecularTextureID, 3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glUniform1i(ShadowMapID, 4);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_floor);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_floor);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);

		// 3rd attribute buffer : normals_floor
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_floor);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);
		glEnableVertexAttribArray(3);
		glBindBuffer(GL_ARRAY_BUFFER, tangentbuffer_floor);
		glVertexAttribPointer(
			3,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);

		// 5th attribute buffer : bitangents
		glEnableVertexAttribArray(4);
		glBindBuffer(GL_ARRAY_BUFFER, bitangentbuffer_floor);
		glVertexAttribPointer(
			4,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);
		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_floor);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices_floor.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
			);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		/*==============================================================finish drawing the floor=================================================*/

		glUniform1i(classNrID, 1);//1代表实物；
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, DiffuseTexture_wall);
		glUniform1i(DiffuseTextureID, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, NormalTexture_wall);
		glUniform1i(NormalTextureID, 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, DiffuseTexture_wall);
		glUniform1i(DiffuseTextureID, 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, SpecularTexture_wall);
		glUniform1i(SpecularTextureID, 3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glUniform1i(ShadowMapID, 4);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_wall);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_wall);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);

		// 3rd attribute buffer : normals_wall
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_wall);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // typevertexbuffer_wall
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);
		glEnableVertexAttribArray(3);
		glBindBuffer(GL_ARRAY_BUFFER, tangentbuffer_wall);
		glVertexAttribPointer(
			3,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);

		// 5th attribute buffer : bitangents
		glEnableVertexAttribArray(4);
		glBindBuffer(GL_ARRAY_BUFFER, bitangentbuffer_wall);
		glVertexAttribPointer(
			4,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);
		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_wall);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices_wall.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
			);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		/*========================================================================finish drawing the wall====================================================*/

		/*==========================================begin to draw man============================================*/

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureCar);
		glUniform1i(DiffuseTextureID, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_car);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_car);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		// 3rd attribute buffer : normals_floor
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_car);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_car);
		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &MVP_car[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix_car[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices_car.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
			);

		glBindTexture(GL_TEXTURE_2D, TextureTyre);
		glUniform1i(DiffuseTextureID, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_tyre);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_tyre);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		// 3rd attribute buffer : normals_floor
		glEnableVertexAttribArray(2);


		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_tyre);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
			);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_tyre);
		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform

		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniform1f(specDegreeID, 3.0);
		glUniform1f(diffDegreeID, 0.0);
		// Draw the triangles !
		for (int i = 0; i < 4; i++)
		{
			glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &MVPtyreArray[i][0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrixTyreArray[i][0][0]);
			glDrawElements(
				GL_TRIANGLES,      // mode
				indices_tyre.size(),    // count
				GL_UNSIGNED_SHORT, // type
				(void*)0           // element array buffer offset
				);
		}


		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		
		if (lightOn)
		{
			glDepthMask(0);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glUniform1i(classNrID, 2);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, TextureLightgun);
			glUniform1i(DiffuseTextureID, 0);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_lightgun);
			glVertexAttribPointer(
				0,                  // attribute
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
				);
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_lightgun);
			glVertexAttribPointer(
				1,                                // attribute
				2,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
				);

			// 3rd attribute buffer : normals_floor
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_lightgun);
			glVertexAttribPointer(
				2,                                // attribute
				3,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
				);

			// Index buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_lightgun);
			// Draw the triangles !
		glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &MVP_lightgun[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrixLightGun[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

			glDrawElements(
				GL_TRIANGLES,      // mode
				indices_lightgun.size(),    // count
				GL_UNSIGNED_SHORT, // type
				(void*)0           // element array buffer offset
				);
			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			glDisable(GL_BLEND);
			glDepthMask(1);
		}
		/*===================================开始画鬼=============================================*/
		if (currentTime > 123 && !existSkull)
		{
			existSkull = true;
			skull = spirit(vec3(0, 0, 0));
			// The VBO containing the positions and sizes of the particles
			glGenBuffers(1, &particles_position_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
			// Initialize with empty (NULL) buffer : it will be updated later, each frame.
			glBufferData(GL_ARRAY_BUFFER, skull.MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

			// The VBO containing the colors of the particles
			glGenBuffers(1, &particles_color_buffer);
			glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
			// Initialize with empty (NULL) buffer : it will be updated later, each frame.
			glBufferData(GL_ARRAY_BUFFER, skull.MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);
		}
		if (existSkull)
		{
			carBody.getHurt(delta, skull.pos);
			vec3 aboveCar = carBody.position + vec3(0, 0.5, 0);
			vec3 carToSpirit = skull.pos - aboveCar;
			float distanceToCamera = length(carToSpirit);
			if (distanceToCamera < 1.0)
				distanceToCamera = 1.0;
			float repulsion = 4.0 / pow(distanceToCamera, 3);
			repulsion = 0.0;
			float gravitation = lightOn ? -dot(CarDirection, normalize(carToSpirit)) / pow(distanceToCamera, 0.5) : 0.5 * pow(distanceToCamera, 0.5);
			skull.move(delta, normalize(carToSpirit)*(repulsion - gravitation));
			vec3 cameraRight = vec3(ViewMatrix[0][0], ViewMatrix[1][0], ViewMatrix[2][0]);
			skull.GenerateNewParticles(delta, cross(cameraRight, normalize(carToSpirit)));
			int ParticlesCount = skull.updateParticles(delta, aboveCar);
			glUseProgram(particleprogramID);
			glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
			glBufferData(GL_ARRAY_BUFFER, skull.MaxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
			glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLfloat) * 4, skull.g_particule_position_size_data);

			glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
			glBufferData(GL_ARRAY_BUFFER, skull.MaxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW); // Buffer orphaning, a common way to improve streaming perf. See above link for details.
			glBufferSubData(GL_ARRAY_BUFFER, 0, ParticlesCount * sizeof(GLubyte) * 4, skull.g_particule_color_data);


			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


			// Bind our texture in Texture Unit 0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, TextureParticle);
			// Set our "myTextureSampler" sampler to user Texture Unit 0
			glUniform1i(particleTextureID, 0);

			// Same as the billboards tutorial
			glUniform3f(CameraRight_worldspace_ID, cameraRight.x, cameraRight.y, cameraRight.z);
			glUniform3f(CameraUp_worldspace_ID, ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);
			glUniform3f(CameraPos_worldspace_ID, aboveCar.x, aboveCar.y, aboveCar.z);
			glUniform3f(Center_worldspace_ID, skull.pos.x, skull.pos.y, skull.pos.z);
			glUniformMatrix4fv(ViewProjMatrixID, 1, GL_FALSE, &ViewProjectionMatrix[0][0]);

			// 1rst attribute buffer : vertices
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
			glVertexAttribPointer(
				0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
				);

			// 2nd attribute buffer : positions of particles' centers
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
			glVertexAttribPointer(
				1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
				4,                                // size : x + y + z + size => 4
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
				);

			// 3rd attribute buffer : particles' colors
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
			glVertexAttribPointer(
				2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
				4,                                // size : r + g + b + a => 4
				GL_UNSIGNED_BYTE,                 // type
				GL_TRUE,                          // normalized?    *** YES, this means that the unsigned char[4] will be accessible with a vec4 (floats) in the shader ***
				0,                                // stride
				(void*)0                          // array buffer offset
				);

			// These functions are specific to glDrawArrays*Instanced*.
			// The first parameter is the attribute buffer we're talking about.
			// The second parameter is the "rate at which generic vertex attributes advance when rendering multiple instances"
			// http://www.opengl.org/sdk/docs/man/xhtml/glVertexAttribDivisor.xml
			glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
			glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
			glVertexAttribDivisor(2, 1); // color : one per quad                                  -> 1

										 // Draw the particules !
										 // This draws many times a small triangle_strip (which looks like a quad).
										 // This is equivalent to :
										 // for(i in ParticlesCount) : glDrawArrays(GL_TRIANGLE_STRIP, 0, 4), 
										 // but faster.
			glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, ParticlesCount);

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
			glVertexAttribDivisor(1, 0); // positions : one per quad (its center)                 -> 1
			glVertexAttribDivisor(2, 0); // color : one per quad                                  -> 1
			glDisable(GL_BLEND);
		}
	
		/*====================================================结束绘制鬼================================*/
		/*====================================开始画透明的物体==================================*/
		//{
		//	glUseProgram(programID);
		//	glEnable(GL_BLEND);
		//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//	glUniform1i(classNrID, 0);
		//	glActiveTexture(GL_TEXTURE0);
		//	glBindTexture(GL_TEXTURE_2D, TextureCar);
		//	glUniform1i(DiffuseTextureID, 0);
		//	glEnableVertexAttribArray(0);
		//	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_car);
		//	glVertexAttribPointer(
		//		0,                  // attribute
		//		3,                  // size
		//		GL_FLOAT,           // type
		//		GL_FALSE,           // normalized?
		//		0,                  // stride
		//		(void*)0            // array buffer offset
		//		);
		//	glEnableVertexAttribArray(2);
		//	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer_car);
		//	glVertexAttribPointer(
		//		2,                                // attribute
		//		3,                                // size
		//		GL_FLOAT,                         // type
		//		GL_FALSE,                         // normalized?
		//		0,                                // stride
		//		(void*)0                          // array buffer offset
		//		);

		//	// Index buffer
		//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_car);
		//	// Send our transformation to the currently bound shader, 
		//	// in the "MVP" uniform
		//	mat4 modelMatrix_ghost = translate(mat4(), vec3(0, 0.2, -2)) * scale(mat4(), vec3(0.2, 0.2, 0.2));
		//	mat4 testMVP = ProjectionMatrix * ViewMatrix * modelMatrix_ghost;
		//	glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, &testMVP[0][0]);
		//	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &modelMatrix_ghost[0][0]);
		//	glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		//	// Draw the triangles !
		//	glDrawElements(
		//		GL_TRIANGLES,      // mode
		//		indices_car.size(),    // count
		//		GL_UNSIGNED_SHORT, // type
		//		(void*)0           // element array buffer offset
		//		);
		//	glDisable(GL_BLEND);
		//}
		/*==============================================================开始在FBO4HGB上绘制============================*/
		glBindFramebuffer(GL_FRAMEBUFFER, FB4HGB);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(HGBProgramID);
		glUniform1f(blurBiasID, 0.002); 
		glUniform1f(blurDirecID, true);
		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, BrightTexture);
		// Set our "renderedTexture" sampler to user Texture Unit 0
		glUniform1i(srcGBtexID, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
		//glViewport(0, 0, 946, 526);
		// Draw the triangle !
		//	 You have to disable GL_COMPARE_R_TO_TEXTURE above in order to see anything 
		glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices_floor starting at 0 -> 2 triangles	

		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		glUniform1f(blurDirecID, false);
		glBindTexture(GL_TEXTURE_2D, HGBTexture);
		// Set our "renderedTexture" sampler to user Texture Unit 0
		glUniform1i(srcGBtexID, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices_floor starting at 0 -> 2 triangles	
		glDisableVertexAttribArray(1);

		/*==============================================================开始在真正的屏幕上绘制============================*/

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, 1920, 1080);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(quad_programID);
		glUniform1f(SpeedinQuadID, carBody.speed);
		glUniform1f(MadnessID, 0.01 + 0.001 * clamp((60.0f - carBody.life),0.0f,60.0f));
		cout << 0.01 + 0.001 * clamp((60.0f - carBody.life), 0.0f, 60.0f) << endl;
		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ResTexture);
		// Set our "renderedTexture" sampler to user Texture Unit 0
		glUniform1i(coltexID, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, InforTexture);
		// Set our "renderedTexture" sampler to user Texture Unit 0
		glUniform1i(inftexID, 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, HVGBTexture);
		// Set our "renderedTexture" sampler to user Texture Unit 0
		glUniform1i(britexID, 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, TextureEnd);
		// Set our "renderedTexture" sampler to user Texture Unit 0
		glUniform1i(endtexID, 3);
		

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
		//glViewport(0, 0, 946, 526);
		// Draw the triangle !
		//	 You have to disable GL_COMPARE_R_TO_TEXTURE above in order to see anything 
		glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices_floor starting at 0 -> 2 triangles		
		glDisableVertexAttribArray(1);
		/*===========================================观察高斯模糊结果=========================*/
		if (bool wannaLookBrightNess = false || (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS))
		{
			glUseProgram(HGBProgramID);
			glViewport(0, 0, 960, 540);
			glDepthFunc(GL_LEQUAL);
			glUniform1f(blurBiasID, 0.0);
			// Bind our texture in Texture Unit 0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, HVGBTexture);
			// Set our "renderedTexture" sampler to user Texture Unit 0
			glUniform1i(srcGBtexID, 0);

			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
			glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
				);
			//glViewport(0, 0, 946, 526);
			// Draw the triangle !
			//	 You have to disable GL_COMPARE_R_TO_TEXTURE above in order to see anything 
			glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices_floor starting at 0 -> 2 triangles		
			glDisableVertexAttribArray(0);
		}
		
		/*==========================================================输出参数======================================*/
		glViewport(0, 0, 1920, 1080);
		glDepthFunc(GL_LEQUAL);
		currentTime = glfwGetTime();
		nbFrames++;
		char text[256];
		//sprintf(text, "%.2f %.2f %.2f", spf, loopTime, subTime);
		sprintf(text, "%.2f C:%d R:%d B:%d %.2f", spf, carBody.isCrash ? 1 : 0, carBody.onRoad ? 1 : 0, carBody.isBack ? 1 : 0, carBody.life);
		printText2D(text, 10, 550, 30);
		/*==========================================================输出参数完毕======================================*/

		if (bool needDepthInformation = false)
		{
			// Optionally render the shadowmap (for debug only)
			glDisable(GL_COMPARE_R_TO_TEXTURE);//?????????????????
											   // Render only on a corner of the window (or we we won't see the real rendering...)
			glViewport(0, 0, 512, 512);

			// Use our shader
			glUseProgram(quad_programID);

			// Bind our texture in Texture Unit 0
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			// Set our "renderedTexture" sampler to user Texture Unit 0
			glUniform1i(coltexID, 0);

			// 1rst attribute buffer : vertices_floor
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
			glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
				);

			// Draw the triangle !
			// You have to disable GL_COMPARE_R_TO_TEXTURE above in order to see anything !
			glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices_floor starting at 0 -> 2 triangles
			glDisableVertexAttribArray(0);
		}



		glfwSwapBuffers(window);
		float midTime = glfwGetTime();
		glfwPollEvents();

		endTime = glfwGetTime();
		if (currentTime - lastTime >= 1.0) { // If last prinf() was more than 1sec ago
											 // printf and reset
			spf = 1000.0 / double(nbFrames);
			nbFrames = 0;
			lastTime += 1.0;
			loopTime = 1000.0*(endTime - startTime);
			subTime = 1000.0*(midTime - startTime);
		}
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer_floor);
	glDeleteBuffers(1, &uvbuffer_floor);
	glDeleteBuffers(1, &normalbuffer_floor);
	glDeleteBuffers(1, &elementbuffer_floor);
	glDeleteBuffers(1, &vertexbuffer_wall);
	glDeleteBuffers(1, &uvbuffer_wall);
	glDeleteBuffers(1, &normalbuffer_wall);
	glDeleteBuffers(1, &elementbuffer_wall);
	glDeleteBuffers(1, &vertexbuffer_car);
	glDeleteBuffers(1, &uvbuffer_car);
	glDeleteBuffers(1, &normalbuffer_car);
	glDeleteBuffers(1, &elementbuffer_car);
	glDeleteProgram(programID);
	glDeleteTextures(1, &DiffuseTexture_floor);
	glDeleteTextures(1, &SpecularTexture_floor);
	glDeleteTextures(1, &NormalTexture_floor);
	glDeleteTextures(1, &DiffuseTexture_wall);
	glDeleteTextures(1, &SpecularTexture_wall);
	glDeleteTextures(1, &NormalTexture_wall);
	glDeleteTextures(1, &ResTexture);
	glDeleteTextures(1, &InforTexture);
	glDeleteFramebuffers(1, &FramebufferName);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Delete the text's VBO, the shader and the texture
	cleanupText2D();

	// Close OpenGL window and terminate GLFW
	//TwTerminate();
	glfwTerminate();

	return 0;
}

