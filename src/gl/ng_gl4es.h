#ifndef NG_GL4ES
#define NG_GL4ES
#define GL_NEGATIVE_ONE_TO_ONE 0x935E
#define GL_ZERO_TO_ONE 0x935F
#define GL_TEXTURE_3D_MULTISAMPLE 0x9103
#define GL_ACTIVE_ATOMIC_COUNTER_BUFFERS 0x92D9
#define GL_MAX_NAME_LENGTH 0x92F6
#define GL_ACTIVE_RESOURCES 0x92F5
#define GL_LOCATION 0x930E
#define GL_ARRAY_SIZE 0x92FB
#define GL_TYPE 0x92FA
#define GL_BLOCK_INDEX 0x92FD
#define GL_OFFSET 0x92FC
#define GL_UNIFORM 0x92E1
#define GL_NAME_LENGTH 0x92f9
#define GL_PROGRAM_BINARY_FORMAT_MESA     0x875F

#include <GL/gl.h>
#include <vector>
#include <functional>
#include <map>
#include <unordered_map>
#include <mutex>
#include <unordered_set>
#include "framebuffers.h"
#include "texture.h"
#include "wrap/gles.h"
#include "gles.h"
#include "glstate.h"
#include "const.h"
#include "gl4es.h"
#include "wrap/gl4es.h"

enum DebugMessageSeverity {
    DEBUG_SEVERITY_NOTIFICATION,
    DEBUG_SEVERITY_LOW,
    DEBUG_SEVERITY_MEDIUM,
    DEBUG_SEVERITY_HIGH,
};
enum MemoryBarrierType {
    MEMORY_BARRIER_VERTEX_ATTRIB_ARRAY = 1 << 0,
    MEMORY_BARRIER_ELEMENT_ARRAY = 1 << 1,
    MEMORY_BARRIER_UNIFORM_BUFFER = 1 << 2,
    MEMORY_BARRIER_TEXTURE_FETCH = 1 << 3,
    MEMORY_BARRIER_FRAMEBUFFER = 1 << 4,
    MEMORY_BARRIER_SHADER_IMAGE_ACCESS = 1 << 5,
};


enum FramebufferAttachment {
    COLOR_ATTACHMENT0 = 0,
    COLOR_ATTACHMENT1,
    COLOR_ATTACHMENT2,
    DEPTH_ATTACHMENT,
    STENCIL_ATTACHMENT,
};

typedef std::function<void(GLuint x, GLuint y, GLuint z, std::vector<float>& data, int width, int height)> ComputeShaderCallback;

typedef std::function<void(DebugMessageSeverity severity, const std::string& message)> DebugMessageCallback;
extern GLuint computeTexture;
extern std::vector<float> computeResults;

extern GLDEBUGPROCARB debugCallback;

struct SyncObject {
    GLuint fbo;
    GLuint texture;
    GLint status;
};

#endif