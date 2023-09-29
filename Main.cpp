// used the module 4 tutorial as a base and made changes as necessary

#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#include <vector>
#include <cmath>
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Brian Engel 3D Triangle"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];     // Handles for the vertex buffer objects
        GLuint nIndices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    //GLMesh gMesh;
    // vector of all meshs
    vector<GLMesh> meshs;
    // Shader program
    GLuint gProgramId;
    // Texture
    GLuint gTextureId0;
    GLuint gTextureId1;

    // variables for prism
    const int prismRad = 1;
    const int prismHeight = 1;
    const int prismSides = 30;
    GLfloat prismVertex[(prismSides + 1) * 2 * 7];
    GLushort prismIndex[prismSides * 12];

    // variables for plane
    GLfloat baseVertex[4*9];
    GLushort baseIndex[6];

    // camera positions
    glm::vec3 gCameraPos = glm::vec3(0.0f, 0.0f, 15.0f);
    glm::vec3 gCameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 gCameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -90.0f;  // Initial yaw angle (facing to the left)
    float pitch = 0.0f;  // Initial pitch angle (level with the horizon)
    float mouseSensitivity = 0.1f;  // Adjust as needed

    // time
    float gDeltaTime = 0.0f; // Time between current frame and last frame
    float gLastFrame = 0.0f;

    // camera speed
    float cameraSpeed = 2.5f;

    // Initial projection type
    bool perspectiveProjection = true;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UCreatePrismMesh(GLMesh& mesh);
void UCreateBaseMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
void GeneratePrismVertices();
void GeneratePrismIndices();
void GenerateBaseVertices();
void GenerateBaseIndices();

void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

bool UCreateTexture(const char* filename, GLuint& textureId);
//bool UCreateSecondTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}
/* Vertex Shader Source Code*/
//const GLchar* vertexShaderSource = GLSL(330,
//    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
//    layout(location = 1) in vec4 color;  // Color data from Vertex Attrib Pointer 1
//
//    out vec4 vertexColor; // variable to transfer color data to the fragment shader
//
//    //Global variables for the  transform matrices
//    uniform mat4 model;
//    uniform mat4 view;
//    uniform mat4 projection;
//
//    void main()
//    {
//        gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
//        vertexColor = color; // references incoming color data
//    }
//);

/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(330,
    layout(location = 0) in vec3 position;     // Position attribute
    layout(location = 1) in vec4 color;        // Color attribute
    layout(location = 2) in vec2 texCoord;     // Texture coordinate attribute

    out vec4 vertexColor;                          // Output color to fragment shader
    out vec2 fragTexCoord;                       // Output texture coordinate to fragment shader

    uniform mat4 model;                          // Model matrix
    uniform mat4 view;                           // View matrix
    uniform mat4 projection;                     // Projection matrix

    void main()
    {
        mat4 mvp = projection * view * model;    // Model-View-Projection matrix
        gl_Position = mvp * vec4(position, 1.0);

        vertexColor = color;                      // Pass color to fragment shader
        fragTexCoord = texCoord;                // Pass texture coordinate to fragment shader
    }
);

/* Fragment Shader Source Code*/
//const GLchar* fragmentShaderSource = GLSL(330,
//    in vec4 vertexColor; // Variable to hold incoming color data from vertex shader
//
//out vec4 fragmentColor;
//
//void main()
//{
//    fragmentColor = vec4(vertexColor);
//}
//);

/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(330,
    in vec4 vertexColor;                           // Input color from vertex shader
    in vec2 fragTexCoord;                        // Input texture coordinate from vertex shader

    out vec4 finalColor;                         // Final output color

    uniform sampler2D textureSampler;            // Texture sampler
    uniform sampler2D textureSampler2;

    uniform vec4 overrideColor;         // Override color

    void main()
    {
        // Use override color if provided, otherwise use base color
        finalColor.rgb = (overrideColor.a > 0.0) ? overrideColor.rgb : vertexColor.rgb;

        // Sample texture using texture coordinates if textureSampler is set
        vec4 texColor = texture(textureSampler, fragTexCoord);
        vec4 texColor2 = texture(textureSampler2, fragTexCoord);

        // If texture is available, use it; otherwise, use the vertex color
        finalColor.rgb = (texColor.rgb == vec3(0.0)) ? finalColor.rgb : texColor.rgb;
        finalColor.rgb = (texColor2.rgb == vec3(0.0)) ? finalColor.rgb : texColor2.rgb;

        // Set the alpha value
        finalColor.a = 1.0; // Or use a different alpha value if needed
    }
);

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;
    GLMesh cylinderMesh;
    GeneratePrismVertices();
    GeneratePrismIndices();
    UCreatePrismMesh(cylinderMesh); // Calls the function to create the Vertex Buffer Object
    meshs.push_back(cylinderMesh);

    GLMesh baseMesh;
    GenerateBaseVertices();
    GenerateBaseIndices();
    UCreateBaseMesh(baseMesh);
    meshs.push_back(baseMesh);
    

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    // Load textures
    const char* texFilename = "stb/gray_fabric.jpg";
    if (!UCreateTexture(texFilename, gTextureId0))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    const char* texFilename2 = "stb/combinedBottle.jpg";
    if (!UCreateTexture(texFilename2, gTextureId1))
    {
        cout << "Failed to load texture " << texFilename2 << endl;
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "textureSampler"), 0);

    // We set the texture as texture unit 1
    glUniform1i(glGetUniformLocation(gProgramId, "textureSampler2"), 1);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // set time variables
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }
    for (GLMesh mesh : meshs) {
        // Release mesh data
        UDestroyMesh(mesh);
    }

    // Release texture
    UDestroyTexture(gTextureId0);
    // Release texture
    UDestroyTexture(gTextureId1);

    // Release shader program
    UDestroyShaderProgram(gProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);

    
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    //static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool keypress = false;

    float cameraOffset = cameraSpeed * gDeltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        gCameraPos += cameraOffset * gCameraFront;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        gCameraPos -= cameraOffset * gCameraFront;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        gCameraPos -= glm::normalize(glm::cross(gCameraFront, gCameraUp)) * cameraOffset;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        gCameraPos += glm::normalize(glm::cross(gCameraFront, gCameraUp)) * cameraOffset;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        gCameraPos += gCameraUp * cameraOffset; // Move up
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        gCameraPos -= gCameraUp * cameraOffset; // Move down
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        perspectiveProjection = !perspectiveProjection;
    }
}



// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// Functioned called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");
    GLint overrideColorLoc = glGetUniformLocation(gProgramId, "overrideColor");

    // I scaled, then translated, then rotated each of the cylinders seperately
    glm::mat4 model = glm::rotate(0.0f, glm::vec3(1.0, 0.0f, 0.0f)) * glm::rotate(0.0f, glm::vec3(0.0, 1.0f, 0.0f)) * glm::rotate(0.0f, glm::vec3(0.0, 0.0f, 1.0f)) * glm::translate(glm::vec3(0.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    
    // updated to use the wasd keys to move
    glm::mat4 view = glm::lookAt(gCameraPos, gCameraPos + gCameraFront, gCameraUp);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Creates a perspective projection
    glm::mat4 projection;
    if (perspectiveProjection) {
        projection = glm::perspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 100.0f);
    }
    //glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    //glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Draws the triangles

    // draw bottom of shampoo bottle
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gTextureId1);
    model = glm::rotate(-1.57f, glm::vec3(1.0, 0.0f, 0.0f)) * glm::rotate(0.0f, glm::vec3(0.0, 1.0f, 0.0f)) * glm::rotate(0.0f, glm::vec3(0.0, 0.0f, 1.0f)) * glm::translate(glm::vec3(0.0f, 0.0f, -2.0f)) * glm::scale(glm::vec3(1.0f, 1.0f, 6.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(meshs.at(0).vao);
    glDrawElements(GL_TRIANGLES, meshs.at(0).nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle
    glBindTexture(GL_TEXTURE_2D, 0);

    // draw top of shampoo bottle
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    model = glm::rotate(-1.57f, glm::vec3(1.0, 0.0f, 0.0f)) * glm::rotate(0.0f, glm::vec3(0.0, 1.0f, 0.0f)) * glm::rotate(0.0f, glm::vec3(0.0, 0.0f, 1.0f)) * glm::translate(glm::vec3(0.0f, 0.0f, 4.0f)) * glm::scale(glm::vec3(0.33f, 0.33f, 0.8f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(overrideColorLoc, 1, GL_FALSE, glm::value_ptr(color));
    glBindVertexArray(meshs.at(0).vao);
    glDrawElements(GL_TRIANGLES, meshs.at(0).nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    // draw base
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureId0);
    model = glm::rotate(0.0f, glm::vec3(1.0, 0.0f, 0.0f)) * glm::rotate(0.0f, glm::vec3(0.0, 1.0f, 0.0f)) * glm::rotate(0.0f, glm::vec3(0.0, 0.0f, 1.0f)) * glm::translate(glm::vec3(0.0f, -2.0f, 0.0f)) * glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(meshs.at(1).vao);
    glDrawElements(GL_TRIANGLES, meshs.at(1).nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle
    glBindTexture(GL_TEXTURE_2D, 0);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreatePrismMesh(GLMesh& mesh)
{
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerColor = 4;

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    size_t prismVertsSize = (prismSides + 1) * 7 * 2 * sizeof(GLfloat);
    glGenBuffers(1, &mesh.vbos[0]);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, prismVertsSize, prismVertex, GL_STATIC_DRAW);
   
    size_t numIndices = sizeof(prismIndex) / sizeof(prismIndex[0]);
    mesh.nIndices = sizeof(prismIndex);
    glGenBuffers(1, &mesh.vbos[1]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * numIndices, prismIndex, GL_STATIC_DRAW);


    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(GLfloat) * floatsPerVertex));
    glEnableVertexAttribArray(1);


}

void UCreateBaseMesh(GLMesh& mesh) {
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerColor = 4;
    const GLuint floatsPerCoord = 2;

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    size_t baseVertsSize = 9 * 4 * sizeof(GLfloat);
    glGenBuffers(1, &mesh.vbos[0]);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, baseVertsSize, baseVertex, GL_STATIC_DRAW);

    size_t numIndices = sizeof(baseIndex) / sizeof(baseIndex[0]);
    mesh.nIndices = sizeof(baseIndex);
    glGenBuffers(1, &mesh.vbos[1]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * numIndices, baseIndex, GL_STATIC_DRAW);


    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor + floatsPerCoord);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(GLfloat) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerCoord, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(GLfloat) * (floatsPerVertex+floatsPerColor)));
    glEnableVertexAttribArray(2);
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, mesh.vbos);
}

bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // Set the texture wrapping parameters.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Set texture filtering parameters.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture.

        return true;
    }

    // Error loading the image
    return false;
}

void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

// I created this function to automate the vertex positions for a prism with differing radius, height, and sides
// and assign it one of 6 colors unless it is the center top which is white, or center botttom which is black
void GeneratePrismVertices() {
    
    vector<GLfloat> prismData;
    const int numColors = 6;
    prismData.insert(prismData.end(), { 0.0f, 0.0f, 0.0f, 1.0f,1.0f,1.0f,1.0f });
    prismData.insert(prismData.end(), { 0.0f, 0.0f, prismHeight, 0.0f,0.0f,0.0f,1.0f });
    for (int i = 0; i < prismSides; ++i) {
        GLfloat theta = 2.0f * M_PI * static_cast<float>(i) / prismSides;
        GLfloat x = prismRad * cos(theta);
        GLfloat y = prismRad * sin(theta);
        prismData.push_back((x));
        prismData.push_back((y));
        prismData.push_back(0.0f);

        // Assign colors with alpha to vertices based on the segment
        glm::vec4 color;
        int colorIndex = i % numColors;
        if (colorIndex == 0) {
            prismData.insert(prismData.end(), { 1.0f, 0.0f, 0.0f, 1.0f }); 
        }
        else if (colorIndex == 1) {
            prismData.insert(prismData.end(), { 0.0f, 1.0f, 0.0f, 1.0f }); 
        }
        else if (colorIndex == 2) {
            prismData.insert(prismData.end(), { 0.0f, 0.0f, 1.0f, 1.0f });
        }
        else if (colorIndex == 3) {
            prismData.insert(prismData.end(), { 1.0f, 1.0f, 0.0f, 1.0f });
        }
        else if (colorIndex == 4) {
            prismData.insert(prismData.end(), { 0.0f, 1.0f, 1.0f, 1.0f });
        }
        else {
            prismData.insert(prismData.end(), { 1.0f, 0.0f, 1.0f, 1.0f });
        }
        prismData.push_back(x);
        prismData.push_back(y);
        prismData.push_back(static_cast<GLfloat>(prismHeight));
        if (colorIndex == 0) {
            prismData.insert(prismData.end(), { 1.0f, 0.0f, 0.0f, 1.0f });
        }
        else if (colorIndex == 1) {
            prismData.insert(prismData.end(), { 0.0f, 1.0f, 0.0f, 1.0f });
        }
        else if (colorIndex == 2) {
            prismData.insert(prismData.end(), { 0.0f, 0.0f, 1.0f, 1.0f });
        }
        else if (colorIndex == 3) {
            prismData.insert(prismData.end(), { 1.0f, 1.0f, 0.0f, 1.0f });
        }
        else if (colorIndex == 4) {
            prismData.insert(prismData.end(), { 0.0f, 1.0f, 1.0f, 1.0f });
        }
        else {
            prismData.insert(prismData.end(), { 1.0f, 0.0f, 1.0f, 1.0f });
        }
    }

    for (int i = 0; i < prismData.size(); i++) {
        prismVertex[i] = prismData[i];
    }
}



// I created this function to get indices for the prism
void GeneratePrismIndices() {
    std::vector<GLushort> prismData;
    // create top and bottom
    int vert = (prismSides + 1) * 2;
    for (int i = 2; i < vert; i++) {
        if (i == 2) {
            prismData.insert(prismData.end(), { 0, static_cast<GLushort>(i), static_cast<GLushort>(vert-2) });
        }
        else if (i == 3) {
            prismData.insert(prismData.end(), { 1, static_cast<GLushort>(i), static_cast<GLushort>(vert-1) });
        }
        else if (i % 2 == 0) {
            prismData.insert(prismData.end(), { 0, static_cast<GLushort>(i), static_cast<GLushort>(i - 2) });
        }
        else {
            prismData.insert(prismData.end(), { 1, static_cast<GLushort>(i), static_cast<GLushort>(i - 2) });
        }
    }
    // create sides
    for (int i = 2; i < vert; i++) {
        if (i == 2) {
            prismData.insert(prismData.end(), { 2, static_cast<GLushort>(vert-2), static_cast<GLushort>(vert-1) });
        }
        else if (i == 3) {
            prismData.insert(prismData.end(), { 2, 3, static_cast<GLushort>(vert - 1) });
        }
        else {
            prismData.insert(prismData.end(), { static_cast<GLushort>(i), static_cast<GLushort>(i-1), static_cast<GLushort>(i-2) });
        }
    }

    
    for (int i = 0; i < prismData.size(); i++) {
        prismIndex[i] = prismData[i];
    }
}

void GenerateBaseVertices() {
    GLfloat values[] = {
        10.0f, 0.0f,  10.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
       -10.0f, 0.0f,  10.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
        10.0f, 0.0f, -10.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
       -10.0f, 0.0f, -10.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f
    };
    for (int i = 0; i < 4 * 9; i++) {
        baseVertex[i] = values[i];
    }
}
void GenerateBaseIndices() {
    GLushort values[] = {
        0, 1, 2,
        1, 2, 3
    };
    for (int i = 0; i < 6; i++) {
        baseIndex[i] = values[i];
    }
}

void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    // variables for camera position
    static double lastX = xpos;
    static double lastY = ypos;
    static bool firstMouse = true;

    // sets up initial position
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // difference between current and last position
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top

    // set current position to last position
    lastX = xpos;
    lastY = ypos;

    // scale the offset
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    // add the offset to the current yaw and pitch
    yaw += xoffset;
    pitch += yoffset;

    // so camera doesn't flip over
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // convert the new pitch and yaw to a new matrix
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    gCameraFront = glm::normalize(front);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// updated to control the camera speed. has minimum of 0.0f and maximum of 10.0f
// updated to control mouse sensitivity as well. has minimum of 0.0f and maximun of 1.0f
// when the values are both 0.0f the object is locked in place
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset < 0) {
        cameraSpeed -= 0.5f;
    }
    else if (yoffset > 0) {
        cameraSpeed += 0.5f;
    }
    if (cameraSpeed < 0.0f) {
        cameraSpeed = 0.0f;
    }
    if (cameraSpeed > 10.0f) {
        cameraSpeed = 10.0f;
    }


    if (yoffset < 0) {
        mouseSensitivity -= 0.05f;
    }
    else if (yoffset > 0) {
        mouseSensitivity += 0.05f;
    }
    if (mouseSensitivity < 0.0f) {
        mouseSensitivity = 0.0f;
    }
    if (mouseSensitivity > 1.0f) {
        mouseSensitivity = 1.0f;
    }
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS) {
            //cout << "Left mouse button pressed" << endl;
        }
        else {
            //cout << "Left mouse button released" << endl;
        }
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS) {
            //cout << "Middle mouse button pressed" << endl;
        }
        else {
            //cout << "Middle mouse button released" << endl;
        }
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS) {
            //cout << "Right mouse button pressed" << endl;
        }
        else {
            //cout << "Right mouse button released" << endl;
        }
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}
