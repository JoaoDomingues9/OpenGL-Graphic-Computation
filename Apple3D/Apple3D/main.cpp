// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow *window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "../common/shader.hpp"
#include "../common/texture.hpp"
#include "../common/controls.hpp"
#include "../common/objloader.hpp"
#include "../common/vboindexer.hpp"

#define height 768
#define width 1024

int main(void)
{
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
	window = glfwCreateWindow(width, height, "Apple 3D", NULL, NULL);
	if (window == NULL)
	{
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	// Hide the mouse and enable unlimited mouvement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, width / 2, height / 2);

	// white blue background
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// array of vertices
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID); // vertex array object (gera um identificador para o VAO)
	glBindVertexArray(VertexArrayID);	  // bind vertex array object (associar ao contexto atual)

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader");

	// Get a handle for our "MVP" uniform(localização da variável uniforme no programa)
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// Load the texture
	GLuint Texture = loadDDS("uvmap.DDS");

	// Read our .obj file
	std::vector<glm::vec3> vertices; // to store 3D coordinates
	std::vector<glm::vec2> uvs;		 // to store 2D of texture coordinates
	std::vector<glm::vec3> normals;	 // to store normals of each vertex
	bool res = loadOBJ("applew.obj", vertices, uvs, normals);

	std::vector<unsigned short> indices;													   // to store indices of each vertex
	std::vector<glm::vec3> indexed_vertices;												   // Declara um vetor para armazenar os vértices indexados
	std::vector<glm::vec2> indexed_uvs;														   // Declara um vetor para armazenar as coordenadas de textura (UVs) indexadas
	std::vector<glm::vec3> indexed_normals;													   // Declara um vetor para armazenar as normais indexadas.
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals); // usa os índices para criar versões indexadas dos vetores de vértices, UVs e normais
	// aumenta o desempenho e reduz o custo de memória de armazenamento

	// Load it into a VBO
	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);																					  // gerar um identificador para o VBO
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);																	  // associar ao contexto atual
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW); // carregar os dados para o VBO GL_STATIC_DRAW: os dados não serão modificados

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);																				// gerar um identificador para os uvs
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);																// associar ao contexto atual
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW); //

	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);				 // gerar um identificador para as normais
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer); // associar ao contexto atual
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);					  // gerar um identificador para os indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer); // associar ao contexto atual
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

	// Get a handle for our "LightPosition" uniform
	glUseProgram(programID);
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	do
	{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix(); // Obtém a matriz de projeção 3d -> 2d
		glm::mat4 ViewMatrix = getViewMatrix();				// matriz de visualização é usada para transformar coordenadas do mundo para o sistema de coordenadas da câmera.
		glm::mat4 ModelMatrix = glm::mat4(1.0);				// matriz de modelo é usada para transformar coordenadas do objeto para o sistema de coordenadas do mundo
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader,
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		glm::vec3 lightPos = glm::vec3(4, 4, 4);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0); // permite o atributo de vertices de indice 0 para serem rendizerados  em forma de Arrays (envia para a GPU)
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,		  // attribute match with shader
			3,		  // size (x,y,z)
			GL_FLOAT, // type float
			GL_FALSE, // normalized? no cordenadas de posição
			0,		  // strid valor de 0 indica que os atributos estão na memória,
					  //  ou seja, não há espaço entre eles. Isso é comum quando os atributos são armazenados sequencialmente em um único buffer.
			(void *)0 // array buffer offset leitura no inicio
		);

		// 2nd attribute buffer : UVs texturas
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,		  // attribute
			2,		  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized?
			0,		  // stride
			(void *)0 // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
			2,		  // attribute
			3,		  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized?
			0,		  // stride
			(void *)0 // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer); // associar ao contexto atual GL_ELEMENT_ARRAY_BUFFER Indica que o buffer contém dados de índices, que serão utilizados para indexar os vértices

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,	   // mode
			indices.size(),	   // count indica o número total de elementos (índices) a serem desenhados
			GL_UNSIGNED_SHORT, // type  especifica o tipo dos elementos nos dados do índice
			(void *)0		   // element array buffer offset renderização deve começar a partir do início do buffer de índices.
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Swap buffers
		// Swap front and back buffers para atualizar a tela
		glfwSwapBuffers(window);
		// Poll for and process events (teclas)
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}
