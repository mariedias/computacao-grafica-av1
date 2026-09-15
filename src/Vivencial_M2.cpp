/*
 * Atividade Vivencial M2
 * Leitura de arquivo .OBJ (geometria) + renderizacao 3D com camera perspectiva
 * 
 * Alunas: Eduarda Fernandes e Maria Eduarda Dias
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

const GLuint WIDTH = 1000, HEIGHT = 800;

struct Object3D
{
    GLuint VAO;
    int nVertices;
    glm::vec3 position;   // translacao
    glm::vec3 scale;      // escala
    glm::vec3 rotation;   // angulos em graus, um por eixo
    glm::vec3 posicaoInicial;
};

vector<Object3D> cena;
int selecionado = 0;
char modo = 'T'; // 'T' -> translação, 'R' -> rotação, 'S' -> escala 
bool escalaUniforme = false;

// shaders

const GLchar *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vColor;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    vColor = color;
}
)";

const GLchar *fragmentShaderSource = R"(
#version 330 core
in vec3 vColor;
uniform int isSelected;
out vec4 color;

void main()
{
    vec3 c = vColor;
    if (isSelected == 1)
        c = vec3(0.60, 0.76, 0.44);   // verde para o objeto selecionado
    color = vec4(c, 1.0);
}
)";

// callbacks

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_P)
    {
        static bool wireframe = false;
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    }

    // seleção
    if (key == GLFW_KEY_TAB)
    {
        selecionado = (selecionado + 1) % cena.size();
        cout << "Objeto selecionado: " << selecionado << endl;
    }

    // escolha do modo
    if (key == GLFW_KEY_R) { modo = 'R'; cout << "Modo: ROTACAO" << endl; }
    if (key == GLFW_KEY_T) { modo = 'T'; cout << "Modo: TRANSLACAO" << endl; }
    if (key == GLFW_KEY_S) { modo = 'S'; cout << "Modo: ESCALA" << endl; }

    if (key == GLFW_KEY_U)
    {
        escalaUniforme = !escalaUniforme;
        cout << "Escala uniforme: " << (escalaUniforme ? "ON" : "OFF") << endl;
    }

    // aplica no objeto selecionado
    Object3D &obj = cena[selecionado];

    // SHIFT inverte o sentido da transformação
    float sentido = (mode & GLFW_MOD_SHIFT) ? -1.0f : 1.0f;

    float passoRot = 5.0f * sentido;
    float passoTrans = 0.1f * sentido;
    float passoEscala = 1.0f + 0.05f * sentido;

    glm::vec3 eixo(0.0f);
    if (key == GLFW_KEY_X) eixo.x = 1.0f;
    if (key == GLFW_KEY_Y) eixo.y = 1.0f;
    if (key == GLFW_KEY_Z) eixo.z = 1.0f;

    if (eixo != glm::vec3(0.0f))
    {
        if (modo == 'R')
            obj.rotation += eixo * passoRot;
        else if (modo == 'T')
            obj.position += eixo * passoTrans;
        else if (modo == 'S')
        {
            if (escalaUniforme)
                obj.scale *= passoEscala;
            else
                obj.scale *= glm::mix(glm::vec3(1.0f), glm::vec3(passoEscala), eixo);
        }
    }

    // translação alternativa pelas setas (X e Y)
    if (modo == 'T')
    {
        if (key == GLFW_KEY_LEFT)  obj.position.x -= 0.1f;
        if (key == GLFW_KEY_RIGHT) obj.position.x += 0.1f;
        if (key == GLFW_KEY_UP)    obj.position.y += 0.1f;
        if (key == GLFW_KEY_DOWN)  obj.position.y -= 0.1f;
    }

        if (key == GLFW_KEY_SPACE)
    {
        for (size_t i = 0; i < cena.size(); i++)
        {
            cena[i].position = cena[i].posicaoInicial;
            cena[i].scale = glm::vec3(1.0f);
            cena[i].rotation = glm::vec3(0.0f);
        }
        cout << "Cena resetada" << endl;
    }
}

// shader

int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cerr << "ERRO no vertex shader:\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cerr << "ERRO no fragment shader:\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cerr << "ERRO ao linkar o shader:\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// leitor OBJ

/*
 * Lê um arquivo .OBJ recuperando apenas a GEOMETRIA (linhas 'v' e os índices
 * de vértice das linhas 'f').
 *
 * Retorna o identificador do VAO, ou -1 em caso de erro.
 * nVertices recebe (por referência) o número de vértices do buffer.
 */
int loadSimpleOBJ(string filePath, int &nVertices, glm::vec3 color = glm::vec3(0.97f, 0.45f, 0.68f))
{
    vector<glm::vec3> vertices;
    vector<GLfloat> vBuffer;

    ifstream arqEntrada(filePath.c_str());
    if (!arqEntrada.is_open())
    {
        cerr << "Erro ao tentar ler o arquivo " << filePath << endl;
        return -1;
    }

    string line;
    while (getline(arqEntrada, line))
    {
        istringstream ssline(line);
        string word;
        ssline >> word;

        if (word == "v")
        {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        }
        else if (word == "f")
        {
            // guarda os índices de vértice desta face
            vector<int> faceIndices;

            while (ssline >> word)
            {
                int vi = 0;
                istringstream ss(word);
                string index;

                // formato: v/vt/vn  (pegamos apenas o primeiro campo)
                if (getline(ss, index, '/') && !index.empty())
                    vi = stoi(index) - 1; // o .OBJ indexa a partir de 1

                if (vi >= 0 && vi < (int)vertices.size())
                    faceIndices.push_back(vi);
            }

            // triangulação em leque: cobre faces com mais de 3 vértices
            for (size_t i = 1; i + 1 < faceIndices.size(); i++)
            {
                int tri[3] = {faceIndices[0], faceIndices[i], faceIndices[i + 1]};
                for (int k = 0; k < 3; k++)
                {
                    glm::vec3 v = vertices[tri[k]];
                    vBuffer.push_back(v.x);
                    vBuffer.push_back(v.y);
                    vBuffer.push_back(v.z);
                    vBuffer.push_back(color.r);
                    vBuffer.push_back(color.g);
                    vBuffer.push_back(color.b);
                }
            }
        }
    }

    arqEntrada.close();

    cout << "Modelo carregado: " << filePath << endl;
    cout << "  vertices lidos (v): " << vertices.size() << endl;

    GLuint VBO, VAO;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // atributo 0: posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // atributo 1: cor (r, g, b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 6; // x, y, z, r, g, b
    cout << "  vertices no buffer: " << nVertices << " (" << nVertices / 3 << " triangulos)" << endl;

    return VAO;
}

/*  
    GLM faz a multiplicação pela direita, então a ordem que o vértice 
    é afetado é a inversa da escrita: scale -> rotate -> translate.
    a razão da ordem escolhida é porque a rotação e escalação 
    acontecem em torno da origem. se mudarmos a posição do objeto
    na tela, a origem é alterada. ex.: caso a ordem fosse alterada,
    se transladar primeiro, o objeto estaria longe da origem, 
    então ao rotacionar, ele orbitaria a "origem" ao invés de 
    girar sobre o próprio eixo.
*/

glm::mat4 getModelMatrix(const Object3D &obj)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, obj.position);
    model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, obj.scale);
    return model;
}

// main

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Atividade Vivencial M2", nullptr, nullptr);
    if (!window)
    {
        cerr << "Falha ao criar a janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cerr << "Falha ao inicializar o GLAD" << endl;
        return -1;
    }

    cout << "Placa de video: " << glGetString(GL_RENDERER) << endl;
    cout << "Versao do OpenGL: " << glGetString(GL_VERSION) << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();

    // ---- carrega o modelo
    int nVertices = 0;
    GLuint suzanneVAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVertices);
    if ((int)suzanneVAO == -1)
    {
        cerr << "Nao foi possivel carregar o modelo." << endl;
        return -1;
    }

    Object3D obj1;
    obj1.VAO = suzanneVAO;
    obj1.nVertices = nVertices;
    obj1.position = glm::vec3(-1.8f, 0.0f, 0.0f);
    obj1.posicaoInicial = glm::vec3(-1.8f, 0.0f, 0.0f);
    obj1.scale = glm::vec3(1.0f, 1.0f, 1.0f);
    obj1.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    cena.push_back(obj1);

    Object3D obj2 = obj1;                          // mesma geometria
    obj2.position = glm::vec3(1.8f, 0.0f, 0.0f);   // outra posição
    obj2.posicaoInicial = glm::vec3(1.8f, 0.0f, 0.0f);
    cena.push_back(obj2);

    glUseProgram(shaderID);

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint projLoc = glGetUniformLocation(shaderID, "projection");
    GLint selectedLoc = glGetUniformLocation(shaderID, "isSelected");

    // câmera (matriz de view) e projeção perspectiva
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 7.0f), // posição da câmera
        glm::vec3(0.0f, 0.0f, 0.0f), // para onde olha
        glm::vec3(0.0f, 1.0f, 0.0f)  // vetor "up"
    );

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)width / (float)height,
        0.1f, 100.0f);

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_DEPTH_TEST);

    // loop principal
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.10f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // desenha cada objeto da cena com sua própria matriz de modelo
        for (size_t i = 0; i < cena.size(); i++)
        {
            glUniform1i(selectedLoc, (int)i == selecionado ? 1 : 0);

            glm::mat4 model = getModelMatrix(cena[i]);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(cena[i].VAO);
            glDrawArrays(GL_TRIANGLES, 0, cena[i].nVertices);
        }
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &suzanneVAO);
    glfwTerminate();
    return 0;
}