// https://www.desmos.com/calculator/3rlswo4wq1?lang=es
//  Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <iostream>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow *window;

// GLM header file
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

// shaders header file
#include "common/shader.hpp"

// Vertex array object (VAO)
GLuint VertexArrayID; // array de vertices

// Vertex buffer object (VBO)
GLuint vertexbuffer; // armazena dados (vertices)

// color buffer object (CBO)
GLuint colorbuffer; // armazena dados (cores)

// GLSL program from the shaders
GLuint programID;

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 900

const float D = 1.4; // is the starting point on the square based on degrees from the center
const int B = 4;      // lados do poligono
const float R = 1;    // raio
const double A = 0.707106781187;
const double a = 3.92699081699;
//--------------------------------------------------------------------------------
float postB(float x)
{
    return -((2 * M_PI) / B) * floor((x * B) / (2 * M_PI));
}
float postC(float x)
{
    return 2 * A * floor((x * B) / (2 * M_PI));
}
float postD(float x)
{
    return x - ((2 * M_PI) / B) * floor((x * B) / (2 * M_PI));
}

float preTransformationF(float x)
{
    float up = R * cos(M_PI / B);
    float down = cos(x - a - ((2 * M_PI * floor(((B * (x - a)) / (2 * M_PI)) - B * floor((1 / (2 * M_PI)) * (x - a))) + M_PI) / B));
    return up / down * cos(x);
}
float preTransformationG(float x)
{
    float up = R * cos(M_PI / B);
    float down = cos(x - a - ((2 * M_PI * floor(((B * (x - a)) / (2 * M_PI)) - B * floor((1 / (2 * M_PI)) * (x - a))) + M_PI) / B));
    return up / down * sin(x) + A;
}
float calcX(float x)
{
    return (preTransformationF(D + postB(x)) - A) * cos(postD(x)) + preTransformationG(D + postB(x)) * sin(postD(x)) + A + postC(x);
}
float calcY(float x)
{
    return preTransformationG(D + postB(x)) * cos(postD(x)) - (preTransformationF(D + postB(x)) - A) * sin(postD(x));
}
//--------------------------------------------------------------------------------
std::vector<GLfloat> *cyclogonPoints(GLfloat xmin, GLfloat xmax, GLfloat step)
{

    float length = xmax - xmin; // xmin é o valor mínimo de x no intervalo da curva, xmax o máximo
    int N = (int)length / step; // number of points on the curve

    std::vector<GLfloat> *array = new std::vector<GLfloat>(N * 3); // tamanho N * 3 pois x,y,z
    float x = xmin;

    for (int i = 0; i < N; i++)
    {
        (*array)[i * 3] = calcX(x);       // x value
        (*array)[(i * 3) + 1] = calcY(x); // y value
        (*array)[(i * 3) + 2] = 0;        // z value
        x += step;
    }
    return array;
}

//--------------------------------------------------------------------------------
int transferDataToGPUMemory(void)
{
    GLfloat xmin = -20.0f; // xmin of the domain
    GLfloat xmax = 20.0f;  // xmin of the domain
    float step = 0.1f;     // Define o tamanho do passo entre os pontos da curva.
    int n;                 // number of points on the curve

    // gera um identificador para o VAO
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID); // associa o identificador ao contexto atual

    // Create and commpile our GLSL program from the shaders
    programID = LoadShaders(
        "SimpleVertexShader.vertexshader",
        "SimpleFragmentShader.fragmentshader");

    std::vector<GLfloat> *vertex_data = cyclogonPoints(xmin, xmax, step); // cria os dados dos vertices da curva
    n = (int)vertex_data->size() / 3;                                     // number of points on the curve (/3 -> x,y,z)

    std::cout << "n = " << n << std::endl;

    std::vector<GLfloat> *color_data = new std::vector<GLfloat>(3 * n); // vetor com a informação das cores
    for (int i = 0; i < n; i++)
    {
        (*color_data)[i * 3] = 0.0f;       // vermelho
        (*color_data)[(i * 3) + 1] = 0.0f; // verde
        (*color_data)[(i * 3) + 2] = 1.0f; // azul
    }

    // std::cout << "n bytes = " << sizeof(GLfloat) * n * 3 << std::endl; // número de bytes ocupados pelos dados de vértices

    // Move vertex data to video memory; specifically to VBO called vertexbuffer
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);                                                                    // associa o VBO ao contexto atual
    glBufferData(GL_ARRAY_BUFFER, (int)vertex_data->size() * sizeof(GLfloat), vertex_data->data(), GL_STATIC_DRAW); // Isso copia os dados dos vértices(vetor) para a memória da GPU.

    // Move color data to video memory; specifically to CBO called colorbuffer
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, (int)color_data->size() * sizeof(GLfloat), color_data->data(), GL_STATIC_DRAW);

    return n;
}

//--------------------------------------------------------------------------------
void cleanupDataFromGPU()
{
    // libertar a memoria alocada na GPU (placa grafica) relacionados à renderização gráfica. (vertices, cores, buffers)
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &colorbuffer);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteProgram(programID);
}

//--------------------------------------------------------------------------------
void draw(int n)
{
    // std::cout<< "n = "<<n<<std::endl;

    // Clear the  buffer color on the screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Use our shader
    glUseProgram(programID);

    // create domain in R^2 matriz 4x4
    glm::mat4 mvp = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f);
    // retrieve the matrix uniform locations
    unsigned int matrix = glGetUniformLocation(programID, "mvp"); // obtem a localização do identificador
    // a matriz mvp, permite que seja usada na transformação dos vértices
    glUniformMatrix4fv(matrix, 1, GL_FALSE, &mvp[0][0]);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);                // permite o atributo de vertices de indice 0 para serem rendizerados  em forma de Arrays (envia para a GPU)
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer); // associa o vertexbuffer ao contexto atual
    // configuração como os dados de vértices são interpretados
    glVertexAttribPointer(
        0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,        // size (x,y,z)
        GL_FLOAT, // type
        GL_FALSE, // normalized? no coordenadas de posição
        0,        // stride Um valor de 0 indica que os atributos estão na memória,
                  //  ou seja, não há espaço entre eles. Isso é comum quando os atributos são armazenados sequencialmente em um único buffer.
        (void *)0 // array buffer offset
    );

    // 2nd attribute buffer : colors
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(
        1,        // attribute. No particular reason for 1, but must match the layout in the shader.
        3,        // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0,        // stride
        (void *)0 // array buffer offset leitura começa no inicio
    );

    glDrawArrays(GL_LINE_STRIP, 0, n); // 3 indices starting at 0 -> 1
    // Isto desenha os dados usando o modo GL_LINE_STRIP
    //  que renderiza uma linha unindo os pontos na ordem
    //  em que são especificados. n é o número de pontos a serem desenhados

    glDisableVertexAttribArray(0); // desativa os atributos após a renderização
    glDisableVertexAttribArray(1);
}
//--------------------------------------------------------------------------------

int main(void)
{
    GLFWwindow *window;

    // Initialize the library
    if (!glfwInit())
    {
        return -1;
    }
    // Define as opções da janela GLFW
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context //Cria uma janela GLFW
    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Cyclogon Generated by a square", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Needed for core profile
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    // Configura o modo de entrada para capturar teclas pressionadas
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    glClearColor(1.0f, 1.0f, 1.0f, 0.0f); // Define a cor de fundo

    // transfer my data (vertices, colors, and shaders) to GPU side
    int n = transferDataToGPUMemory();
    std::cout << "n = " << n << std::endl;

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {

        draw(n);

        // Swap front and back buffers para atualizar a tela
        glfwSwapBuffers(window);

        // Poll for and process events (teclas)
        glfwPollEvents();
    }

    // Cleanup VAO, VBOs, and shaders from GPU
    cleanupDataFromGPU();

    glfwTerminate();

    return 0;
}
