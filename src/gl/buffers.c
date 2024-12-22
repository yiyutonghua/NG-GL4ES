#include <GLES/gl3.h>
#include "buffers.h"

#include "khash.h"
#include "../glx/hardext.h"
#include "attributes.h"
#include "debug.h"
#include "gl4es.h"
#include "glstate.h"
#include "logs.h"
#include "init.h"
#include "loader.h"

//#define DEBUG
#ifdef DEBUG
#define DBG(a) a
#else
#define DBG(a)
#endif


KHASH_MAP_IMPL_INT(buff, glbuffer_t*);
KHASH_MAP_IMPL_INT(glvao, glvao_t*);

static GLuint lastbuffer = 1;

// Utility function to bind / unbind a particular buffer

glbuffer_t** BUFF(GLenum target) {
    switch (target) {
    case GL_ARRAY_BUFFER:
        return &glstate->vao->vertex;
        break;
    case GL_ELEMENT_ARRAY_BUFFER:
        return &glstate->vao->elements;
        break;
    case GL_PIXEL_PACK_BUFFER:
        return &glstate->vao->pack;
        break;
    case GL_PIXEL_UNPACK_BUFFER:
        return &glstate->vao->unpack;
        break;
    default:
        LOGD("Warning, unknown buffer target 0x%04X\n", target);
    }
    return (glbuffer_t**)NULL;
}

void unbind_buffer(GLenum target) {
    glbuffer_t** t = BUFF(target);
    if (t)
        *t = (glbuffer_t*)NULL;
}
void bind_buffer(GLenum target, glbuffer_t* buff) {
    glbuffer_t** t = BUFF(target);
    if (t)
        *t = buff;
}
glbuffer_t* getbuffer_buffer(GLenum target) {
    glbuffer_t** t = BUFF(target);
    if (t)
        return *t;
    return NULL;
}
glbuffer_t* getbuffer_id(GLuint buffer) {
    if (!buffer)
        return NULL;
    khint_t k;
    int ret;
    khash_t(buff)* list = glstate->buffers;
    k = kh_get(buff, list, buffer);
    if (k == kh_end(list))
        return NULL;
    return kh_value(list, k);
}

int buffer_target(GLenum target) {
    if (target == GL_ARRAY_BUFFER)
        return 1;
    if (target == GL_ELEMENT_ARRAY_BUFFER)
        return 1;
    if (target == GL_PIXEL_PACK_BUFFER)
        return 1;
    if (target == GL_PIXEL_UNPACK_BUFFER)
        return 1;
    return 0;
}

void rebind_real_buff_arrays(int old_buffer, int new_buffer) {
    for (int j = 0; j < hardext.maxvattrib; j++) {
        if (glstate->vao->vertexattrib[j].real_buffer == old_buffer) {
            glstate->vao->vertexattrib[j].real_buffer = new_buffer;
            if (!new_buffer)
                glstate->vao->vertexattrib[j].real_pointer = 0;
        }
    }
}

void gl4es_glGenBuffers(GLsizei n, GLuint* buffers) {
    DBG(SHUT_LOGD("glGenBuffers(%i, %p)\n", n, buffers);)
        noerrorShim();
    if (n < 1) {
        errorShim(GL_INVALID_VALUE);
        return;
    }
    khash_t(buff)* list = glstate->buffers;
    for (int i = 0; i < n; i++) {   // create buffer, and check uniqueness...
        int b;
        while (getbuffer_id(b = lastbuffer++));
        buffers[i] = b;
        // create the buffer
        khint_t k;
        int ret;
        k = kh_put(buff, list, b, &ret);
        glbuffer_t* buff = kh_value(list, k) = malloc(sizeof(glbuffer_t));
        buff->buffer = b;
        buff->type = 0; // no target for now
        buff->data = NULL;
        buff->usage = GL_STATIC_DRAW;
        buff->size = 0;
        buff->access = GL_READ_WRITE;
        buff->mapped = 0;
        buff->real_buffer = 0;
    }
}

void  __attribute__((visibility("default")))  glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    DBG(SHUT_LOGD("glBindBufferBase(%s, %u, %u)\n", PrintEnum(target), index, buffer);)
        FLUSH_BEGINEND;

    // Check if the target is valid
    if (!buffer_target(target)) {
        errorShim(GL_INVALID_ENUM);
        return;
    }

    // If buffer is 0, unbind the buffer
    if (buffer == 0) {
        // unbind buffer from the specified target and index
        glstate->vao->vertexattrib[index].real_buffer = 0;
        glstate->vao->vertexattrib[index].real_pointer = NULL;
    }
    else {
        // Search for the existing buffer or create a new one if not found
        khint_t k;
        int ret;
        glbuffer_t* buff = NULL;
        khash_t(buff)* list = glstate->buffers;
        k = kh_get(buff, list, buffer);
        if (k == kh_end(list)) {
            // Create a new buffer if not found
            k = kh_put(buff, list, buffer, &ret);
            buff = kh_value(list, k) = malloc(sizeof(glbuffer_t));
            buff->buffer = buffer;
            buff->type = target;
            buff->data = NULL;
            buff->usage = GL_STATIC_DRAW;
            buff->size = 0;
            buff->access = GL_READ_WRITE;
            buff->mapped = 0;
            buff->real_buffer = 0;
        }
        else {
            buff = kh_value(list, k);
        }

        // Bind the buffer to the specified index and target
        glstate->vao->vertexattrib[index].real_buffer = buffer;
        glstate->vao->vertexattrib[index].real_pointer = NULL; // Real buffer pointer can be updated later
        bind_buffer(target, buff);
    }

    noerrorShim();
}

void __attribute__((visibility("default"))) glBindBuffersRange(GLenum target, GLuint first, GLsizei count, const GLuint* buffers, const GLintptr* offsets, const GLsizeiptr* sizes) {
    DBG(SHUT_LOGD("glBindBuffersRange(%s, %u, %d, %p, %p, %p)\n", PrintEnum(target), first, count, buffers, offsets, sizes);)
        FLUSH_BEGINEND;

    // Check if the target is valid
    if (!buffer_target(target)) {
        errorShim(GL_INVALID_ENUM);
        return;
    }

    for (GLsizei i = 0; i < count; ++i) {
        GLuint index = first + i;
        GLuint buffer = buffers[i];
        GLintptr offset = offsets[i];
        GLsizeiptr size = sizes[i];

        // If buffer is 0, unbind the buffer from the specified target and index
        if (buffer == 0) {
            glstate->vao->vertexattrib[index].real_buffer = 0;
            glstate->vao->vertexattrib[index].real_pointer = NULL;
            glstate->vao->vertexattrib[index].stride = 0;
        }
        else {
            // Search for the existing buffer or create a new one if not found
            khint_t k;
            int ret;
            glbuffer_t* buff = NULL;
            khash_t(buff)* list = glstate->buffers;
            k = kh_get(buff, list, buffer);
            if (k == kh_end(list)) {
                // Create a new buffer if not found
                k = kh_put(buff, list, buffer, &ret);
                buff = kh_value(list, k) = malloc(sizeof(glbuffer_t));
                buff->buffer = buffer;
                buff->type = target;
                buff->data = NULL;
                buff->usage = GL_STATIC_DRAW;
                buff->size = 0;
                buff->access = GL_READ_WRITE;
                buff->mapped = 0;
                buff->real_buffer = 0;
            }
            else {
                buff = kh_value(list, k);
            }

            // Bind the buffer with the specified offset and size
            glstate->vao->vertexattrib[index].real_buffer = buffer;
            glstate->vao->vertexattrib[index].real_pointer = (const GLvoid*)(intptr_t)offset;
            glstate->vao->vertexattrib[index].stride = size;
            bind_buffer(target, buff);
        }
    }

    noerrorShim();
}


void __attribute__((visibility("default"))) glBindBuffersBase(GLenum target, GLuint first, GLsizei count, const GLuint* buffers) {
    DBG(SHUT_LOGD("glBindBuffersBase(%s, %u, %d, %p)\n", PrintEnum(target), first, count, buffers);)
        FLUSH_BEGINEND;

    // Check if the target is valid
    if (!buffer_target(target)) {
        errorShim(GL_INVALID_ENUM);
        return;
    }

    for (GLsizei i = 0; i < count; ++i) {
        GLuint index = first + i;
        GLuint buffer = buffers[i];

        // If buffer is 0, unbind the buffer from the specified target and index
        if (buffer == 0) {
            glstate->vao->vertexattrib[index].real_buffer = 0;
            glstate->vao->vertexattrib[index].real_pointer = NULL;
        }
        else {
            // Search for the existing buffer or create a new one if not found
            khint_t k;
            int ret;
            glbuffer_t* buff = NULL;
            khash_t(buff)* list = glstate->buffers;
            k = kh_get(buff, list, buffer);
            if (k == kh_end(list)) {
                // Create a new buffer if not found
                k = kh_put(buff, list, buffer, &ret);
                buff = kh_value(list, k) = malloc(sizeof(glbuffer_t));
                buff->buffer = buffer;
                buff->type = target;
                buff->data = NULL;
                buff->usage = GL_STATIC_DRAW;
                buff->size = 0;
                buff->access = GL_READ_WRITE;
                buff->mapped = 0;
                buff->real_buffer = 0;
            }
            else {
                buff = kh_value(list, k);
            }

            // Bind the buffer to the specified index and target
            glstate->vao->vertexattrib[index].real_buffer = buffer;
            glstate->vao->vertexattrib[index].real_pointer = NULL;
            bind_buffer(target, buff);
        }
    }

    noerrorShim();
}


void __attribute__((visibility("default"))) glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    DBG(SHUT_LOGD("glBindBufferRange(%s, %u, %u, %ld, %ld)\n", PrintEnum(target), index, buffer, offset, size);)
        FLUSH_BEGINEND;

    // Check if the target is valid
    if (!buffer_target(target)) {
        errorShim(GL_INVALID_ENUM);
        return;
    }

    // If buffer is 0, unbind the buffer from the specified target and index
    if (buffer == 0) {
        glstate->vao->vertexattrib[index].real_buffer = 0;
        glstate->vao->vertexattrib[index].real_pointer = NULL;
    }
    else {
        // Search for the existing buffer or create a new one if not found
        khint_t k;
        int ret;
        glbuffer_t* buff = NULL;
        khash_t(buff)* list = glstate->buffers;
        k = kh_get(buff, list, buffer);
        if (k == kh_end(list)) {
            // Create a new buffer if not found
            k = kh_put(buff, list, buffer, &ret);
            buff = kh_value(list, k) = malloc(sizeof(glbuffer_t));
            buff->buffer = buffer;
            buff->type = target;
            buff->data = NULL;
            buff->usage = GL_STATIC_DRAW;
            buff->size = 0;
            buff->access = GL_READ_WRITE;
            buff->mapped = 0;
            buff->real_buffer = 0;
        }
        else {
            buff = kh_value(list, k);
        }

        // Bind the buffer with specified offset and size
        glstate->vao->vertexattrib[index].real_buffer = buffer;
        glstate->vao->vertexattrib[index].real_pointer = NULL; // Real buffer pointer can be updated later
        glstate->vao->vertexattrib[index].pointer = (const GLvoid*)(intptr_t)offset;
        glstate->vao->vertexattrib[index].stride = size;  // Set the stride to the specified size

        // Bind the buffer with offset and size
        bind_buffer(target, buff);
    }

    noerrorShim();
}



void gl4es_glBindBuffer(GLenum target, GLuint buffer) {
    DBG(SHUT_LOGD("glBindBuffer(%s, %u)\n", PrintEnum(target), buffer);)
        // this flush is probably not needed as long as real VBO are not used
        FLUSH_BEGINEND;

    khint_t k;
    int ret;
    khash_t(buff)* list = glstate->buffers;
    if (!buffer_target(target)) {
        errorShim(GL_INVALID_ENUM);
        return;
    }
    // if buffer = 0 => unbind buffer!
    if (buffer == 0) {
        // unbind buffer
        unbind_buffer(target);
    }
    else {
        // search for an existing buffer
        k = kh_get(buff, list, buffer);
        glbuffer_t* buff = NULL;
        if (k == kh_end(list)) {
            k = kh_put(buff, list, buffer, &ret);
            buff = kh_value(list, k) = malloc(sizeof(glbuffer_t));
            buff->buffer = buffer;
            buff->type = target;
            buff->data = NULL;
            buff->usage = GL_STATIC_DRAW;
            buff->size = 0;
            buff->access = GL_READ_WRITE;
            buff->mapped = 0;
            buff->real_buffer = 0;
        }
        else {
            buff = kh_value(list, k);
            buff->type = target;    //TODO: check if old binding?
        }
        bind_buffer(target, buff);
    }
    noerrorShim();
}

void gl4es_glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) {
    DBG(SHUT_LOGD("glBufferData(%s, %i, %p, %s)\n", PrintEnum(target), size, data, PrintEnum(usage));)
        if (!buffer_target(target)) {
            errorShim(GL_INVALID_ENUM);
            return;
        }
    glbuffer_t* buff = getbuffer_buffer(target);
    if (buff == NULL) {
        errorShim(GL_INVALID_OPERATION);
        LOGE("Warning, null buffer for target=0x%04X for glBufferData\n", target);
        return;
    }
    if (target == GL_ARRAY_BUFFER)
        VaoSharedClear(glstate->vao);

    int go_real = 0;
    if ((target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER)
        && (usage == GL_STREAM_DRAW || usage == GL_STATIC_DRAW || usage == GL_DYNAMIC_DRAW) && globals4es.usevbo)
        go_real = 1;

    if (buff->real_buffer && !go_real) {
        rebind_real_buff_arrays(buff->real_buffer, 0);
        LOAD_GLES(glDeleteBuffers);
        gles_glDeleteBuffers(1, &buff->real_buffer);
        // what about VA already pointing there?
        buff->real_buffer = 0;
    }
    if (go_real) {
        if (!buff->real_buffer) {
            LOAD_GLES(glGenBuffers);
            gles_glGenBuffers(1, &buff->real_buffer);
        }
        LOAD_GLES(glBufferData);
        LOAD_GLES(glBindBuffer);
        gles_glBindBuffer(target, buff->real_buffer);
        gles_glBufferData(target, size, data, usage);
        gles_glBindBuffer(target, 0);
        DBG(SHUT_LOGD(" => real VBO %d\n", buff->real_buffer);)
    }

    if (buff->data && buff->size < size) {
        free(buff->data);
        buff->data = NULL;
    }
    if (!buff->data)
        buff->data = malloc(size);
    buff->size = size;
    buff->usage = usage;
    DBG(SHUT_LOGD("\t buff->data = %p (size=%d)\n", buff->data, size);)
        buff->access = GL_READ_WRITE;
    if (data)
        memcpy(buff->data, data, size);
    noerrorShim();
}

void gl4es_glNamedBufferData(GLuint buffer, GLsizeiptr size, const GLvoid* data, GLenum usage) {
    DBG(SHUT_LOGD("glNamedBufferData(%u, %i, %p, %s)\n", buffer, size, data, PrintEnum(usage));)
        glbuffer_t* buff = getbuffer_id(buffer);
    if (buff == NULL) {
        DBG(SHUT_LOGD("Named Buffer not found\n");)
            errorShim(GL_INVALID_OPERATION);
        return;
    }
    if (buff->data) {
        free(buff->data);

    }
    int go_real = 0;
    if ((buff->type == GL_ARRAY_BUFFER || buff->type == GL_ELEMENT_ARRAY_BUFFER)
        && (usage == GL_STREAM_DRAW || usage == GL_STATIC_DRAW || usage == GL_DYNAMIC_DRAW) && globals4es.usevbo)
        go_real = 1;

    if (buff->real_buffer && !go_real) {
        LOAD_GLES(glDeleteBuffers);
        gles_glDeleteBuffers(1, &buff->real_buffer);
        // what about VA already pointing there?
        buff->real_buffer = 0;
    }
    if (go_real) {
        if (!buff->real_buffer) {
            LOAD_GLES(glGenBuffers);
            gles_glGenBuffers(1, &buff->real_buffer);
        }
        LOAD_GLES(glBufferData);
        LOAD_GLES(glBindBuffer);
        gles_glBindBuffer(buff->type, buff->real_buffer);
        gles_glBufferData(buff->type, size, data, usage);
        gles_glBindBuffer(buff->type, 0);
    }

    buff->size = size;
    buff->usage = usage;
    buff->data = malloc(size);
    buff->access = GL_READ_WRITE;
    if (data)
        memcpy(buff->data, data, size);
    noerrorShim();
}

__attribute__((visibility("default"))) void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
    DBG(SHUT_LOGD("glCopyBufferSubData(%s, %s, %p, %p, %i)\n", PrintEnum(readTarget), PrintEnum(writeTarget), readOffset, writeOffset, size));
    if (!buffer_target(readTarget) || !buffer_target(writeTarget)) {
        errorShim(GL_INVALID_ENUM);
        return;
    }

    // 获取读写目标缓冲区
    glbuffer_t* readBuff = getbuffer_buffer(readTarget);
    glbuffer_t* writeBuff = getbuffer_buffer(writeTarget);

    if (readBuff == NULL || writeBuff == NULL) {
        errorShim(GL_INVALID_OPERATION);
        SHUT_LOGE("LIBGL: Warning, null buffer for target=0x%04X or 0x%04X in glCopyBufferSubData\n", readTarget, writeTarget);
        return;
    }

    // 检查偏移量和大小是否合法
    if (readOffset < 0 || writeOffset < 0 || size < 0 ||
        readOffset + size > readBuff->size || writeOffset + size > writeBuff->size) {
        errorShim(GL_INVALID_VALUE);
        return;
    }

    // 如果源目标和目标缓冲区的类型相同
    if (readBuff->real_buffer && writeBuff->real_buffer) {
        SHUT_LOGE("LIBGL: Info, can use glCopyBufferSubData\n");
        LOAD_GLES2(glCopyBufferSubData);
        LOAD_GLES2(glBindBuffer);
        gles_glBindBuffer(readTarget, readBuff->real_buffer);
        gles_glBindBuffer(writeTarget, writeBuff->real_buffer);
        gles_glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
        gles_glBindBuffer(readTarget, 0);
        gles_glBindBuffer(writeTarget, 0);
    } else {
        SHUT_LOGE("LIBGL: Err, cannot use glCopyBufferSubData\n");

        // 如果实际缓冲区不存在，我们将使用 memcpy 进行模拟（此时只能在内存中处理）
        if (readBuff->data && writeBuff->data) {
            memcpy(writeBuff->data + writeOffset, readBuff->data + readOffset, size);
        }
    }

    noerrorShim();
}



void gl4es_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data) {
    DBG(SHUT_LOGD("glBufferSubData(%s, %p, %i, %p)\n", PrintEnum(target), offset, size, data);)
        if (!buffer_target(target)) {
            errorShim(GL_INVALID_ENUM);
            return;
        }
    glbuffer_t* buff = getbuffer_buffer(target);
    if (buff == NULL) {
        errorShim(GL_INVALID_OPERATION);
        DBG(SHUT_LOGD("LIBGL: Warning, null buffer for target=0x%04X for glBufferSubData\n", target);)
            return;
    }

    if (target == GL_ARRAY_BUFFER)
        VaoSharedClear(glstate->vao);

    if (offset < 0 || size<0 || offset + size>buff->size) {
        errorShim(GL_INVALID_VALUE);
        return;
    }

    if ((target == GL_ARRAY_BUFFER || target == GL_ELEMENT_ARRAY_BUFFER) && buff->real_buffer) {
        LOAD_GLES(glBufferSubData);
        LOAD_GLES(glBindBuffer);
        gles_glBindBuffer(target, buff->real_buffer);
        gles_glBufferSubData(target, offset, size, data);
        gles_glBindBuffer(target, 0);

    }

    memcpy(buff->data + offset, data, size);
    noerrorShim();
}
void gl4es_glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const GLvoid* data) {
    DBG(SHUT_LOGD("glNamedBufferSubData(%u, %p, %i, %p)\n", buffer, offset, size, data);)
        glbuffer_t* buff = getbuffer_id(buffer);
    if (buff == NULL) {
        errorShim(GL_INVALID_OPERATION);
        return;
    }

    if (offset < 0 || size<0 || offset + size>buff->size) {
        errorShim(GL_INVALID_VALUE);
        return;
    }

    if ((buff->type == GL_ARRAY_BUFFER || buff->type == GL_ELEMENT_ARRAY_BUFFER) && buff->real_buffer) {
        LOAD_GLES(glBufferSubData);
        LOAD_GLES(glBindBuffer);
        gles_glBindBuffer(buff->type, buff->real_buffer);
        gles_glBufferSubData(buff->type, offset, size, data);
        gles_glBindBuffer(buff->type, 0);
    }
    memcpy(buff->data + offset, data, size);
    noerrorShim();
}

void gl4es_glDeleteBuffers(GLsizei n, const GLuint* buffers) {
    DBG(SHUT_LOGD("glDeleteBuffers(%i, %p)\n", n, buffers);)
        if (!glstate) return;
    FLUSH_BEGINEND;

    VaoSharedClear(glstate->vao);
    khash_t(buff)* list = glstate->buffers;
    if (list) {
        khint_t k;
        glbuffer_t* buff;
        for (int i = 0; i < n; i++) {
            GLuint t = buffers[i];
            DBG(SHUT_LOGD("\t deleting %d\n", t);)
                if (t) {    // don't allow to remove default one
                    k = kh_get(buff, list, t);
                    if (k != kh_end(list)) {
                        buff = kh_value(list, k);
                        if (buff->real_buffer) {
                            rebind_real_buff_arrays(buff->real_buffer, 0);  // unbind
                            LOAD_GLES(glDeleteBuffers);
                            gles_glDeleteBuffers(1, &buff->real_buffer);
                        }
                        if (glstate->vao->vertex == buff)
                            glstate->vao->vertex = NULL;
                        if (glstate->vao->elements == buff)
                            glstate->vao->elements = NULL;
                        if (glstate->vao->pack == buff)
                            glstate->vao->pack = NULL;
                        if (glstate->vao->unpack == buff)
                            glstate->vao->unpack = NULL;
                        for (int j = 0; j < hardext.maxvattrib; j++)
                            if (glstate->vao->vertexattrib[j].buffer == buff)
                                glstate->vao->vertexattrib[j].buffer = NULL;
                        DBG(SHUT_LOGD("\t buff->data = %p\n", buff->data);)
                            if (buff->data) free(buff->data);
                        kh_del(buff, list, k);
                        free(buff);
                    }
                }
        }
    }
    DBG(SHUT_LOGD("\t done\n");)
        noerrorShim();
}

GLboolean gl4es_glIsBuffer(GLuint buffer) {
    DBG(SHUT_LOGD("glIsBuffer(%u)\n", buffer);)
        khash_t(buff)* list = glstate->buffers;
    khint_t k;
    noerrorShim();
    if (list) {
        k = kh_get(buff, list, buffer);
        if (k != kh_end(list)) {
            return GL_TRUE;
        }
    }
    return GL_FALSE;
}


static void bufferGetParameteriv(glbuffer_t* buff, GLenum value, GLint* data) {
    noerrorShim();
    switch (value) {
    case GL_BUFFER_ACCESS:
        data[0] = buff->access;
        break;
    case GL_BUFFER_ACCESS_FLAGS:
        data[0] = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        break;
    case GL_BUFFER_MAPPED:
        data[0] = (buff->mapped) ? GL_TRUE : GL_FALSE;
        break;
    case GL_BUFFER_MAP_LENGTH:
        data[0] = (buff->mapped) ? buff->size : 0;
        break;
    case GL_BUFFER_MAP_OFFSET:
        data[0] = 0;
        break;
    case GL_BUFFER_SIZE:
        data[0] = buff->size;
        break;
    case GL_BUFFER_USAGE:
        data[0] = buff->usage;
        break;
    default:
        SHUT_LOGD("[ERROR] bufferGetParameteriv Unexpected: %s", PrintEnum(value));
        errorShim(GL_INVALID_ENUM);
        /* TODO Error if something else */
    }
}

void gl4es_glGetBufferParameteriv(GLenum target, GLenum value, GLint* data) {
    DBG(SHUT_LOGD("glGetBufferParameteriv(%s, %s, %p)\n", PrintEnum(target), PrintEnum(value), data);)
        if (!buffer_target(target)) {
            errorShim(GL_INVALID_ENUM);
            return;
        }
    glbuffer_t* buff = getbuffer_buffer(target);
    if (buff == NULL) {
        errorShim(GL_INVALID_OPERATION);
        return;		// Should generate an error!
    }
    bufferGetParameteriv(buff, value, data);
}
void gl4es_glGetNamedBufferParameteriv(GLuint buffer, GLenum value, GLint* data) {
    DBG(SHUT_LOGD("glGetNamedBufferParameteriv(%u, %s, %p)\n", buffer, PrintEnum(value), data);)
        glbuffer_t* buff = getbuffer_id(buffer);
    if (buff == NULL) {
        errorShim(GL_INVALID_OPERATION);
        return;		// Should generate an error!
    }
    bufferGetParameteriv(buff, value, data);
}

void* gl4es_glMapBuffer(GLenum target, GLenum access) {
    DBG(SHUT_LOGD("glMapBuffer(%s, %s)\n", PrintEnum(target), PrintEnum(access));)
        if (!buffer_target(target)) {
            errorShim(GL_INVALID_ENUM);
            return (void*)NULL;
        }

    if (target == GL_ARRAY_BUFFER)
        VaoSharedClear(glstate->vao);

    glbuffer_t* buff = getbuffer_buffer(target);
    if (buff == NULL) {
        errorShim(GL_INVALID_VALUE);
        return NULL;
    }
    if (buff->mapped) {
        errorShim(GL_INVALID_OPERATION);
        return NULL;
    }
    buff->access = access;	// not used
    buff->mapped = 1;
    buff->ranged = 0;
    noerrorShim();
    return buff->data;		// Not nice, should do some copy or something probably
}
void* gl4es_glMapNamedBuffer(GLuint buffer, GLenum access) {
    DBG(SHUT_LOGD("glMapNamedBuffer(%u, %s)\n", buffer, PrintEnum(access));)

        glbuffer_t* buff = getbuffer_id(buffer);
    if (buff == NULL) {
        errorShim(GL_INVALID_VALUE);
        return NULL;
    }
    if (buff->mapped) {
        errorShim(GL_INVALID_OPERATION);
        return NULL;
    }
    buff->access = access;	// not used
    buff->mapped = 1;
    buff->ranged = 0;
    noerrorShim();
    return buff->data;		// Not nice, should do some copy or something probably
}

GLboolean gl4es_glUnmapBuffer(GLenum target) {
    DBG(SHUT_LOGD("glUnmapBuffer(%s)\n", PrintEnum(target));)
        if (glstate->list.compiling) { errorShim(GL_INVALID_OPERATION); return GL_FALSE; }
    FLUSH_BEGINEND;

    if (!buffer_target(target)) {
        errorShim(GL_INVALID_ENUM);
        return GL_FALSE;
    }

    if (target == GL_ARRAY_BUFFER)
        VaoSharedClear(glstate->vao);

    glbuffer_t* buff = getbuffer_buffer(target);
    if (buff == NULL) {
        errorShim(GL_INVALID_VALUE);
        return GL_FALSE;
    }
    noerrorShim();
    if (buff->real_buffer && (buff->type == GL_ARRAY_BUFFER || buff->type == GL_ELEMENT_ARRAY_BUFFER) && buff->mapped && !buff->ranged && (buff->access == GL_WRITE_ONLY || buff->access == GL_READ_WRITE)) {
        LOAD_GLES(glBufferSubData);
        LOAD_GLES(glBindBuffer);
        gles_glBindBuffer(buff->type, buff->real_buffer);
        gles_glBufferSubData(buff->type, 0, buff->size, buff->data);
        gles_glBindBuffer(buff->type, 0);
    }
    if (buff->real_buffer && (buff->type == GL_ARRAY_BUFFER || buff->type == GL_ELEMENT_ARRAY_BUFFER) && buff->mapped && buff->ranged && (buff->access & GL_MAP_WRITE_BIT_EXT) && !(buff->access & GL_MAP_FLUSH_EXPLICIT_BIT_EXT)) {
        LOAD_GLES(glBufferSubData);
        LOAD_GLES(glBindBuffer);
        gles_glBindBuffer(buff->type, buff->real_buffer);
        gles_glBufferSubData(buff->type, buff->offset, buff->length, (void*)((uintptr_t)buff->data + buff->offset));
        gles_glBindBuffer(buff->type, 0);
    }
    if (buff->mapped) {
        buff->mapped = 0;
        buff->ranged = 0;
        return GL_TRUE;
    }
    return GL_FALSE;
}
GLboolean gl4es_glUnmapNamedBuffer(GLuint buffer) {
    DBG(SHUT_LOGD("glUnmapNamedBuffer(%u)\n", buffer);)
        if (glstate->list.compiling) { errorShim(GL_INVALID_OPERATION); return GL_FALSE; }
    FLUSH_BEGINEND;

    glbuffer_t* buff = getbuffer_id(buffer);
    if (buff == NULL)
        return GL_FALSE;		// Should generate an error!
    noerrorShim();
    if (buff->real_buffer && (buff->type == GL_ARRAY_BUFFER || buff->type == GL_ELEMENT_ARRAY_BUFFER) && buff->mapped && (buff->access == GL_WRITE_ONLY || buff->access == GL_READ_WRITE)) {
        LOAD_GLES(glBufferSubData);
        LOAD_GLES(glBindBuffer);
        gles_glBindBuffer(buff->type, buff->real_buffer);
        gles_glBufferSubData(buff->type, 0, buff->size, buff->data);
        gles_glBindBuffer(buff->type, 0);
    }
    if (buff->real_buffer && (buff->type == GL_ARRAY_BUFFER || buff->type == GL_ELEMENT_ARRAY_BUFFER) && buff->mapped && buff->ranged && (buff->access & GL_MAP_WRITE_BIT_EXT) && !(buff->access & GL_MAP_FLUSH_EXPLICIT_BIT_EXT)) {
        LOAD_GLES(glBufferSubData);
        LOAD_GLES(glBindBuffer);
        gles_glBindBuffer(buff->type, buff->real_buffer);
        gles_glBufferSubData(buff->type, buff->offset, buff->length, (void*)((uintptr_t)buff->data + buff->offset));
        gles_glBindBuffer(buff->type, 0);
    }
    if (buff->mapped) {
        buff->mapped = 0;
        buff->ranged = 0;
        return GL_TRUE;
    }
    return GL_FALSE;
}

void gl4es_glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid* data) {
    DBG(SHUT_LOGD("glGetBufferSubData(%s, %p, %i, %p)\n", PrintEnum(target), offset, size, data);)
        if (!buffer_target(target)) {
            errorShim(GL_INVALID_ENUM);
            return;
        }
    glbuffer_t* buff = getbuffer_buffer(target);

    if (buff == NULL)
        return;		// Should generate an error!
    // TODO, check parameter consistancie
    memcpy(data, buff->data + offset, size);
    noerrorShim();
}
void gl4es_glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, GLvoid* data) {
    DBG(SHUT_LOGD("glGetNamedBufferSubData(%u, %p, %i, %p)\n", buffer, offset, size, data);)
        glbuffer_t* buff = getbuffer_id(buffer);

    if (buff == NULL)
        return;		// Should generate an error!
    // TODO, check parameter consistancie
    memcpy(data, buff->data + offset, size);
    noerrorShim();
}

void gl4es_glGetBufferPointerv(GLenum target, GLenum pname, GLvoid** params) {
    DBG(SHUT_LOGD("glGetBufferPointerv(%s, %s, %p)\n", PrintEnum(target), PrintEnum(pname), params);)
        if (!buffer_target(target)) {
            errorShim(GL_INVALID_ENUM);
            return;
        }
    glbuffer_t* buff = getbuffer_buffer(target);
    if (buff == NULL)
        return;		// Should generate an error!
    if (pname != GL_BUFFER_MAP_POINTER) {
        errorShim(GL_INVALID_ENUM);
        return;
    }
    if (!buff->mapped) {
        params[0] = NULL;
    }
    else {
        params[0] = buff->data;
    }
}
void gl4es_glGetNamedBufferPointerv(GLuint buffer, GLenum pname, GLvoid** params) {
    DBG(SHUT_LOGD("glGetNamedBufferPointerv(%u, %s, %p)\n", buffer, PrintEnum(pname), params);)
        glbuffer_t* buff = getbuffer_id(buffer);
    if (buff == NULL)
        return;		// Should generate an error!
    if (pname != GL_BUFFER_MAP_POINTER) {
        errorShim(GL_INVALID_ENUM);
        return;
    }
    if (!buff->mapped) {
        params[0] = NULL;
    }
    else {
        params[0] = buff->data;
    }
}

void* gl4es_glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    DBG(SHUT_LOGD("glMapBufferRange(%s, %p, %d, 0x%x)\n", PrintEnum(target), offset, length, access);)
        if (!buffer_target(target)) {
            errorShim(GL_INVALID_ENUM);
            return NULL;
        }

    glbuffer_t* buff = getbuffer_buffer(target);
    if (buff == NULL) {
        errorShim(GL_INVALID_VALUE);
        return NULL;		// Should generate an error!
    }
    if (buff->mapped) {
        errorShim(GL_INVALID_OPERATION);
        return NULL;
    }
    buff->access = access;
    buff->mapped = 1;
    buff->ranged = 1;
    buff->offset = offset;
    buff->length = length;
    noerrorShim();
    uintptr_t ret = (uintptr_t)buff->data;
    ret += offset;
    return (void*)ret;
}
void gl4es_glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length)
{
    DBG(SHUT_LOGD("glFlushMappedBufferRange(%s, %p, %d)\n", PrintEnum(target), offset, length);)
        if (!buffer_target(target)) {
            errorShim(GL_INVALID_ENUM);
            return;
        }

    if (target == GL_ARRAY_BUFFER)
        VaoSharedClear(glstate->vao);

    glbuffer_t* buff = getbuffer_buffer(target);
    if (!buff) {
        errorShim(GL_INVALID_VALUE);
        return;
    }
    if (!buff->mapped || !buff->ranged || !(buff->access & GL_MAP_FLUSH_EXPLICIT_BIT_EXT)) {
        errorShim(GL_INVALID_OPERATION);
        return;
    }

    if (buff->real_buffer && (buff->type == GL_ARRAY_BUFFER || buff->type == GL_ELEMENT_ARRAY_BUFFER) && (buff->access & GL_MAP_WRITE_BIT_EXT)) {
        LOAD_GLES(glBufferSubData);
        LOAD_GLES(glBindBuffer);
        gles_glBindBuffer(buff->type, buff->real_buffer);
        gles_glBufferSubData(buff->type, buff->offset + offset, length, (void*)((uintptr_t)buff->data + buff->offset + offset));
        gles_glBindBuffer(buff->type, 0);
    }
}


//Direct wrapper
void glGenBuffers(GLsizei n, GLuint* buffers) AliasExport("gl4es_glGenBuffers");
void glBindBuffer(GLenum target, GLuint buffer) AliasExport("gl4es_glBindBuffer");
void glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) AliasExport("gl4es_glBufferData");
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data) AliasExport("gl4es_glBufferSubData");
void glDeleteBuffers(GLsizei n, const GLuint* buffers) AliasExport("gl4es_glDeleteBuffers");
GLboolean glIsBuffer(GLuint buffer) AliasExport("gl4es_glIsBuffer");
void glGetBufferParameteriv(GLenum target, GLenum value, GLint* data) AliasExport("gl4es_glGetBufferParameteriv");
void* glMapBuffer(GLenum target, GLenum access) AliasExport("gl4es_glMapBuffer");
GLboolean glUnmapBuffer(GLenum target) AliasExport("gl4es_glUnmapBuffer");
void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid* data) AliasExport("gl4es_glGetBufferSubData");
void glGetBufferPointerv(GLenum target, GLenum pname, GLvoid** params) AliasExport("gl4es_glGetBufferPointerv");

void* glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) AliasExport("gl4es_glMapBufferRange");
void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) AliasExport("gl4es_glFlushMappedBufferRange");

//ARB wrapper
#ifndef AMIGAOS4
void glGenBuffersARB(GLsizei n, GLuint* buffers) AliasExport("gl4es_glGenBuffers");
void glBindBufferARB(GLenum target, GLuint buffer) AliasExport("gl4es_glBindBuffer");
void glBufferDataARB(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) AliasExport("gl4es_glBufferData");
void glBufferSubDataARB(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data) AliasExport("gl4es_glBufferSubData");
void glDeleteBuffersARB(GLsizei n, const GLuint* buffers) AliasExport("gl4es_glDeleteBuffers");
GLboolean glIsBufferARB(GLuint buffer) AliasExport("gl4es_glIsBuffer");
void glGetBufferParameterivARB(GLenum target, GLenum value, GLint* data) AliasExport("gl4es_glGetBufferParameteriv");
void* glMapBufferARB(GLenum target, GLenum access) AliasExport("gl4es_glMapBuffer");
GLboolean glUnmapBufferARB(GLenum target) AliasExport("gl4es_glUnmapBuffer");
void glGetBufferSubDataARB(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid* data) AliasExport("gl4es_glGetBufferSubData");
void glGetBufferPointervARB(GLenum target, GLenum pname, GLvoid** params) AliasExport("gl4es_glGetBufferPointerv");
#endif

//Direct Access
void glNamedBufferData(GLuint buffer, GLsizeiptr size, const GLvoid* data, GLenum usage) AliasExport("gl4es_glNamedBufferData");
void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const GLvoid* data) AliasExport("gl4es_glNamedBufferSubData");
void glGetNamedBufferParameteriv(GLuint buffer, GLenum value, GLint* data) AliasExport("gl4es_glGetNamedBufferParameteriv");
void* glMapNamedBuffer(GLuint buffer, GLenum access) AliasExport("gl4es_glMapNamedBuffer");
GLboolean glUnmapNamedBuffer(GLuint buffer) AliasExport("gl4es_glUnmapNamedBuffer");
void glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, GLvoid* data) AliasExport("gl4es_glGetNamedBufferSubData");
void glGetNamedBufferPointerv(GLuint buffer, GLenum pname, GLvoid** params) AliasExport("gl4es_glGetNamedBufferPointerv");

void glNamedBufferDataEXT(GLuint buffer, GLsizeiptr size, const GLvoid* data, GLenum usage) AliasExport("gl4es_glNamedBufferData");
void glNamedBufferSubDataEXT(GLuint buffer, GLintptr offset, GLsizeiptr size, const GLvoid* data) AliasExport("gl4es_glNamedBufferSubData");
void glGetNamedBufferParameterivEXT(GLuint buffer, GLenum value, GLint* data) AliasExport("gl4es_glGetNamedBufferParameteriv");
void* glMapNamedBufferEXT(GLuint buffer, GLenum access) AliasExport("gl4es_glMapNamedBuffer");
GLboolean glUnmapNamedBufferEXT(GLuint buffer) AliasExport("gl4es_glUnmapNamedBuffer");
void glGetNamedBufferSubDataEXT(GLuint buffer, GLintptr offset, GLsizeiptr size, GLvoid* data) AliasExport("gl4es_glGetNamedBufferSubData");
void glGetNamedBufferPointervEXT(GLuint buffer, GLenum pname, GLvoid** params) AliasExport("gl4es_glGetNamedBufferPointerv");


// VAO ****************
static GLuint lastvao = 1;

void gl4es_glGenVertexArrays(GLsizei n, GLuint* arrays) {
    DBG(SHUT_LOGD("glGenVertexArrays(%i, %p)\n", n, arrays);)
        noerrorShim();
    if (n < 1) {
        errorShim(GL_INVALID_VALUE);
        return;
    }
    for (int i = 0; i < n; i++) {   // TODO: create VAO here and check unicity
        arrays[i] = lastvao++;
    }
}
void gl4es_glBindVertexArray(GLuint array) {
    DBG(SHUT_LOGD("glBindVertexArray(%u)\n", array);)
        FLUSH_BEGINEND;

    khint_t k;
    int ret;
    khash_t(glvao)* list = glstate->vaos;
    // if array = 0 => unbind buffer!
    if (array == 0) {
        // unbind buffer
        glstate->vao = glstate->defaultvao;
    }
    else {
        // search for an existing buffer
        k = kh_get(glvao, list, array);
        glvao_t* glvao = NULL;
        if (k == kh_end(list)) {
            k = kh_put(glvao, list, array, &ret);
            glvao = kh_value(list, k) = malloc(sizeof(glvao_t));
            // new vao is binded to nothing
            VaoInit(glvao);
            // TODO: check if should copy status of current VAO instead of cleanning everything

            // just put is number
            glvao->array = array;
        }
        else {
            glvao = kh_value(list, k);
        }
        glstate->vao = glvao;
    }

    noerrorShim();
}
void gl4es_glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {
    DBG(SHUT_LOGD("glDeleteVertexArrays(%i, %p)\n", n, arrays);)
        if (!glstate) return;
    FLUSH_BEGINEND;

    khash_t(glvao)* list = glstate->vaos;
    if (list) {
        khint_t k;
        glvao_t* glvao;
        for (int i = 0; i < n; i++) {
            GLuint t = arrays[i];
            if (t) {    // don't allow to remove the default one
                k = kh_get(glvao, list, t);
                if (k != kh_end(list)) {
                    glvao = kh_value(list, k);
                    VaoSharedClear(glvao);
                    kh_del(glvao, list, k);
                    //free(glvao);  //let the use delete those
                }
            }
        }
    }
    noerrorShim();
}
GLboolean gl4es_glIsVertexArray(GLuint array) {
    DBG(SHUT_LOGD("glIsVertexArray(%u)\n", array);)
        if (!glstate)
            return GL_FALSE;
    khash_t(glvao)* list = glstate->vaos;
    khint_t k;
    noerrorShim();
    if (list) {
        k = kh_get(glvao, list, array);
        if (k != kh_end(list)) {
            return GL_TRUE;
        }
    }
    return GL_FALSE;
}

void VaoSharedClear(glvao_t* vao) {
    if (vao == NULL || vao->shared_arrays == NULL)
        return;
    if (!(--(*vao->shared_arrays))) {
        free(vao->vert.ptr);
        free(vao->color.ptr);
        free(vao->secondary.ptr);
        free(vao->normal.ptr);
        for (int i = 0; i < hardext.maxtex; i++)
            free(vao->tex[i].ptr);
        free(vao->shared_arrays);
    }
    vao->vert.ptr = NULL;
    vao->color.ptr = NULL;
    vao->secondary.ptr = NULL;
    vao->normal.ptr = NULL;
    for (int i = 0; i < hardext.maxtex; i++)
        vao->tex[i].ptr = NULL;
    vao->shared_arrays = NULL;
}

void VaoInit(glvao_t* vao) {
    memset(vao, 0, sizeof(glvao_t));
    for (int i = 0; i < hardext.maxvattrib; i++) {
        vao->vertexattrib[i].size = 4;
        vao->vertexattrib[i].type = GL_FLOAT;
    }
}

//Direct wrapper
void glGenVertexArrays(GLsizei n, GLuint* arrays) AliasExport("gl4es_glGenVertexArrays");
void glBindVertexArray(GLuint array) AliasExport("gl4es_glBindVertexArray");
void glDeleteVertexArrays(GLsizei n, const GLuint* arrays) AliasExport("gl4es_glDeleteVertexArrays");
GLboolean glIsVertexArray(GLuint array) AliasExport("gl4es_glIsVertexArray");

