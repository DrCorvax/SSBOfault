#include <stdio.h>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// OpenGL debugging
const char* DebugType[] =
{
    "Error",               //GL_DEBUG_TYPE_ERROR 0x824C
    "Deprecated behavior", //GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
    "Undefined behavior",  //GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
    "Portability",         //GL_DEBUG_TYPE_PORTABILITY 0x824F
    "Perfomance",          //GL_DEBUG_TYPE_PERFORMANCE 0x8250
    "Other",               //GL_DEBUG_TYPE_OTHER 0x8251
    "Marker",              //GL_DEBUG_TYPE_MARKER 0x8268
    "Push",                //GL_DEBUG_TYPE_PUSH_GROUP 0x8269
    "Pop",                 //GL_DEBUG_TYPE_POP_GROUP 0x826A
};

void GLAPIENTRY OnDebug ( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam )
{
    printf ( "%s: %s\n", DebugType[type - GL_DEBUG_TYPE_ERROR], message );
}

// Compute shader source code
// NOTE: increase repetitions number to get driver exception
const char * shader_src =
{
    "#version 430 core\n"\
    "layout ( local_size_x = 1, local_size_y = 1, local_size_z = 1 ) in;\n"\
    "layout ( std430, binding = 0 ) buffer Output\n"\
    "{\n"\
    "    uint value;\n"\
    "};\n"\
    "void main ()\n"\
    "{\n"\
    "    for ( uint i = 0u; i < 110000000u; ++i )\n"\
    "    {\n"\
    "        value = i;\n"\
    "    }\n"\
    "}\n"
};


int main ()
{
    // GLFW initialization
    glfwInit ();

    // debug context
    glfwWindowHint ( GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE );

    // window creation
    GLFWwindow * window = glfwCreateWindow ( 800, 600, "SSBO Fault", NULL, NULL );

    // context
    glfwMakeContextCurrent ( window );

    // GLEW initialization
    glewInit ();

    // OpenGL debug output
    glEnable ( GL_DEBUG_OUTPUT );
    glEnable ( GL_DEBUG_OUTPUT_SYNCHRONOUS );
    glDebugMessageCallback ( OnDebug, nullptr );

    // version info
    printf ( "OpenGL version is %s\n", glGetString ( GL_VERSION ) );
    printf ( "GLFW version is %s\n", glfwGetVersionString () );
    printf ( "GLEW version is %s\n", glewGetString ( GLEW_VERSION ) );

    // shader creation
    GLuint shader_id = glCreateShader ( GL_COMPUTE_SHADER );
    glShaderSource ( shader_id, 1, &shader_src, nullptr );

    // shader compilation
    glCompileShader ( shader_id );
    GLint result;
    glGetShaderiv ( shader_id, GL_COMPILE_STATUS, &result );
    if ( result == GL_FALSE )
    {
        int len;
        glGetShaderiv ( shader_id, GL_INFO_LOG_LENGTH, &len );
        char * err = new char[len];
        glGetShaderInfoLog ( shader_id, len, &len, err );
        printf ( "Shader compilation error: %s\n", err );
        delete [] err;
        return -1;
    }

    // program creation and linking
    GLuint prog_id = glCreateProgram ();
    glAttachShader ( prog_id, shader_id );
    glLinkProgram ( prog_id );
    glGetProgramiv ( prog_id, GL_LINK_STATUS, &result );
    if ( result == GL_FALSE )
    {
        GLint log_len;
        glGetProgramiv ( prog_id, GL_INFO_LOG_LENGTH, &log_len );
        char * log = new char[log_len];
        GLsizei written;
        glGetProgramInfoLog ( prog_id, log_len, &written, log );
        printf ( "Program linking error: %s\n", log );
        delete [] log;
        return -1;
    }

    // program validation
    glValidateProgram ( prog_id );
    glGetProgramiv ( prog_id, GL_VALIDATE_STATUS, &result );
    if ( result == GL_FALSE )
    {
        printf ( "Program validation error.\n" );
        return -1;
    }

    // SSBO creation and binding
    GLuint value = 0u;
    GLuint ssbo;
    glGenBuffers ( 1, &ssbo );
    glBindBuffer ( GL_SHADER_STORAGE_BUFFER, ssbo );
    glBufferData ( GL_SHADER_STORAGE_BUFFER, sizeof ( GLuint  ), &value, GL_DYNAMIC_DRAW );
    glBindBufferBase ( GL_SHADER_STORAGE_BUFFER, 0u, ssbo );

    // shader invocation
    glUseProgram ( prog_id );
    glDispatchCompute ( 1u, 1u, 1u );
    glMemoryBarrier ( GL_ALL_BARRIER_BITS );

    // SSBO readback
    GLuint output;
    glGetNamedBufferSubData ( ssbo, 0, sizeof ( GLuint ), static_cast <void*> ( &output ) );
    printf ( "SSBO output value is %u\n", output );

    // GLFW termination
    glfwTerminate ();

    return 0;
}
