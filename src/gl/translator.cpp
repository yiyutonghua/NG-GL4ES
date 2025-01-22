#include "ng_gl4es.h"
#include "loader.h"
#include <GL/gl.h>
#include <vector>
#include <thread>
#include <functional>
#include <iostream>
#include <cassert>
#include <mutex>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <dlfcn.h>
#include <sys/time.h>
#include <unistd.h>
#include <sstream>

GLuint computeTexture;
std::vector<float> computeResults;
GLDEBUGPROCARB debugCallback;
int textureWidth;
int textureHeight;
std::unordered_set<FramebufferAttachment> invalidatedAttachments;
unsigned int currentBarrierState;
std::mutex barrierMutex;
void* handle;

extern "C" {
    VISIBLE void threadComputeTask(GLuint startX, GLuint endX, GLuint num_groups_y, GLuint num_groups_z, ComputeShaderCallback computeShader, std::vector<float>& results, int width, int height);
    VISIBLE void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
    VISIBLE void reportUnsupportedFunction(const char* funcName);
    VISIBLE void glMemoryBarrier(GLbitfield barriers);
    VISIBLE void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
    VISIBLE void glBindProgramPipeline(GLuint pipeline);
    VISIBLE void APIENTRY glDebugMessageCallback(GLDEBUGPROCARB callback, const void* userParam);
    VISIBLE void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments);
    //VISIBLE void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);
    //VISIBLE void glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values);
    //VISIBLE void glDeleteSync(GLsync sync);
    //VISIBLE GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);
    //VISIBLE GLboolean glIsSync(GLsync sync);
    //VISIBLE GLsync glFenceSync(GLenum condition, GLbitfield flags);
    VISIBLE void glDispatchComputeIndirect(GLuint indirectBuffer);
    //NEEDTOBEDONE
    //void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program);
    //void glCreateTextures(GLenum target, GLsizei n, GLuint* textures);
    //void glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer);
    //void glTextureBufferRange(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
    //void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width);
    //void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
    //void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
    //void glTextureStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
    //void glTextureStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
    //void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void* pixels);
    //void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
    //void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels);
    //void glCompressedTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data);
    //void glCompressedTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data);
    //void glCompressedTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data);
    //void glCopyTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
    //void glCopyTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
    //void glCopyTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, GLsizei depth);
    //void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat* params);
    //void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint* params);
    //void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint* params);
    //void glTextureParameteriv(GLuint texture, GLenum pname, const GLint* params);
    //void glGenerateTextureMipmap(GLuint texture);
    //void glBindTextureUnit(GLuint unit, GLuint texture);
    //void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels);
    //void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat* params);
    //void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint* params);
    //void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint* params);
    //void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint* params);
    //void glGetTexParameterIuiv(GLuint texture, GLenum pname, GLuint* params);
    //void glGetTexParameterIiv(GLuint texture, GLenum pname, GLint* params);
    //void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer);
    //void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
    //void glTexStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
    //void glTexImage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
    //void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint* params);
    //void glTexParameterIiv(GLenum target, GLenum pname, const GLint* params);
    //void glTexStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
    //void glGetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void* pixels);
    //void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level);
    //void glTextureView(GLuint texture, GLenum target, GLuint origTexture, GLenum internalformat, GLint minLevel, GLint numLevels, GLint minLayer, GLint numLayers);
    //void glBindTextures(GLuint first, GLsizei count, const GLuint* textures);
    //void glBindImageTextures(GLuint first, GLsizei count, const GLuint* textures);
    //void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer);
    //void glGetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void* pixels);
    //void glGetCompressedTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void* pixels);
    //void glTextureBarrier();
    //void glGenerateTextureMipmapEXT(GLuint texture);
    //void glNamedFramebufferTexture1DEXT(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
    //void glNamedFramebufferTexture2DEXT(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
    //void glNamedFramebufferTexture3DEXT(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint zlayer);
    //void glColorMaskIndexedEXT(GLuint buf, GLboolean r, GLboolean g, GLboolean b, GLboolean a);
    //void glSecondaryColor3bEXT(GLbyte red, GLbyte green, GLbyte blue);
    //void glSecondaryColor3sEXT(GLshort red, GLshort green, GLshort blue);
    //void glSecondaryColor3iEXT(GLint red, GLint green, GLint blue);
    //void glSecondaryColor3dEXT(GLdouble red, GLdouble green, GLdouble blue);
    //void glSecondaryColor3ubEXT(GLubyte red, GLubyte green, GLubyte blue);
    //void glSecondaryColor3usEXT(GLushort red, GLushort green, GLushort blue);
    //void glSecondaryColor3uiEXT(GLuint red, GLuint green, GLuint blue);
    //void glSecondaryColor3bvEXT(const GLbyte* v);
    //void glSecondaryColor3svEXT(const GLshort* v);
    //void glSecondaryColor3ivEXT(const GLint* v);
    //void glSecondaryColor3dvEXT(const GLdouble* v);
    //void glSecondaryColor3ubvEXT(const GLubyte* v);
    //void glSecondaryColor3usvEXT(const GLushort* v);
    //void glSecondaryColor3uivEXT(const GLuint* v);
    //GLvoid* glMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access);
    //void glFlushMappedNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length);
    //GLvoid* glMapNamedBufferRangeEXT(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access);
    //void glFlushMappedNamedBufferRangeEXT(GLuint buffer, GLintptr offset, GLsizeiptr length);
    //void glVertexAttribI1i(GLuint index, GLint x);
    //void glVertexAttribI2i(GLuint index, GLint x, GLint y);
    //void glVertexAttribI3i(GLuint index, GLint x, GLint y, GLint z);
    //void glVertexAttribI1ui(GLuint index, GLuint x);
    //void glVertexAttribI2ui(GLuint index, GLuint x, GLuint y);
    //void glVertexAttribI3ui(GLuint index, GLuint x, GLuint y, GLuint z);
    //void glVertexAttribI1iv(GLuint index, const GLint* v);
    //void glVertexAttribI2iv(GLuint index, const GLint* v);
    //void glVertexAttribI3iv(GLuint index, const GLint* v);
    //void glVertexAttribI1uiv(GLuint index, const GLuint* v);
    //void glVertexAttribI2uiv(GLuint index, const GLuint* v);
    //void glVertexAttribI3uiv(GLuint index, const GLuint* v);
    //void glVertexAttribI4bv(GLuint index, const GLbyte* v);
    //void glVertexAttribI4sv(GLuint index, const GLshort* v);
    //void glVertexAttribI4ubv(GLuint index, const GLubyte* v);
    //void glVertexAttribI4usv(GLuint index, const GLushort* v);
    //void glBindFragDataLocation(GLuint program, GLuint colorNumber, const GLchar* name);
    //void glBeginConditionalRender(GLuint query, GLenum mode);
    //void glEndConditionalRender();
    //void glPrimitiveRestartIndex(GLuint index);
    //void glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformName);
    //void glProvokingVertex(GLenum mode);
    //void glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
    //void glMultiTexCoordP1uiv(GLenum target, GLuint textureUnit, const GLuint* coords);
    //void glMultiTexCoordP2uiv(GLenum target, GLuint textureUnit, const GLuint* coords);
    //void glMultiTexCoordP3uiv(GLenum target, GLuint textureUnit, const GLuint* coords);
    //void glMultiTexCoordP4uiv(GLenum target, GLuint textureUnit, const GLuint* coords);
    //void glNormalP3ui(GLenum type, GLuint coords);
    //void glNormalP3uiv(GLenum type, const GLuint* coords);
    //void glColorP3ui(GLenum type, GLuint color);
    //void glColorP4ui(GLenum type, GLuint color);
    //void glColorP3uiv(GLenum type, const GLuint* color);
    //void glColorP4uiv(GLenum type, const GLuint* color);
    //void glVertexP2ui(GLenum type, GLuint coords);
    //void glVertexP3ui(GLenum type, GLuint coords);
    //void glVertexP4ui(GLenum type, GLuint coords);
    //void glVertexP2uiv(GLenum type, const GLuint* coords);
    //void glVertexP3uiv(GLenum type, const GLuint* coords);
    //void glVertexP4uiv(GLenum type, const GLuint* coords);
    //void glTexCoordP1ui(GLenum type, GLuint coord);
    //void glTexCoordP2ui(GLenum type, GLuint coord);
    //void glTexCoordP3ui(GLenum type, GLuint coord);
    //void glTexCoordP4ui(GLenum type, GLuint coord);
    //void glTexCoordP1uiv(GLenum type, const GLuint* coord);
    //void glTexCoordP2uiv(GLenum type, const GLuint* coord);
    //void glTexCoordP3uiv(GLenum type, const GLuint* coord);
    //void glTexCoordP4uiv(GLenum type, const GLuint* coord);
    //void glSecondaryColorP3ui(GLenum type, GLuint color);
    //void glSecondaryColorP3uiv(GLenum type, const GLuint* color);
    //void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const GLchar* name);
    //GLuint glGetFragDataIndex(GLuint program, const GLchar* name);
    //void glVertexAttribP1ui(GLuint index, GLenum type, GLboolean normalized, GLuint value);
    //void glVertexAttribP2ui(GLuint index, GLenum type, GLboolean normalized, GLuint value);
    //void glVertexAttribP3ui(GLuint index, GLenum type, GLboolean normalized, GLuint value);
    //void glVertexAttribP4ui(GLuint index, GLenum type, GLboolean normalized, GLuint value);
    //void glVertexAttribP1uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint* values);
    //void glVertexAttribP2uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint* values);
    //void glVertexAttribP3uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint* values);
    //void glVertexAttribP4uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint* values);
    //GLint glGetFragDataLocation(GLuint program, const char* name);
    /*
    void glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint* params);
    void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program);
    GLboolean glIsProgramPipeline(GLuint pipeline);
    void glDeleteProgramPipelines(GLsizei n, const GLuint* pipelinesArray);
    void glCreateProgramPipelines(GLsizei n, GLuint* pipelinesArray);
    GLuint glSpecializeShader(GLuint shader, const GLchar* entry, GLsizei numSpecializationConstants, const GLuint* specializationConstants, const GLint* specializationValues);
    GLuint glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar** strings);
    void glGetShaderProgramiv(GLuint program, GLenum pname, GLint* params);
    GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar* name);
    void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params);
    void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName);
    GLuint glGetUniformBlockIndex(GLuint program, const GLchar* uniformBlockName);
    void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint bindingPoint);
    void glBindBufferBase(GLenum target, GLuint index, GLuint buffer);
    void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
    void glUniformSubroutinesuiv(GLenum shaderType, GLsizei count, const GLuint* indices);
    void glGetUniformSubroutineuiv(GLenum shaderType, GLuint index, GLuint* param);
    void glProgramParameteri(GLuint program, GLenum pname, GLint value);
    void glBindProgramPipeline(GLuint pipeline);
    void glProgramBinary(GLuint program, GLenum binaryFormat, const void* binary, GLsizei length);
    void glProgramParameterfv(GLuint program, GLenum pname, const GLfloat* params);
    void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params);
    GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar* name);
    void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei* length, GLchar* name);
    void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei bufSize, GLsizei* length, GLint* params);
    void glLinkProgram(GLuint program);
    void glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params);
    void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
    void glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformName);
    void glGetUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params);
    void glGetUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName);
    / GLint gl4es_glGetUniformLocationIndex(GLuint program, const GLchar* name);
    void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
    void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
    void glDeleteProgram(GLuint program);
    void glDeleteShader(GLuint shader);
    void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, void* binary);
    //GLint gl4es_glGetUniformLocation(GLuint program, const GLchar* name);
    GLuint glCreateShader(GLenum type);
    GLuint glCreateProgram();
    void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
    void glCompileShader(GLuint shader);
    void glAttachShader(GLuint program, GLuint shader);
    void glValidateProgram(GLuint program);
    void glUseProgram(GLuint program);
    void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei* count, GLuint* shadersOut);
    void glVertexAttribL1d(GLuint index, GLdouble x);
    void glVertexAttribL2d(GLuint index, GLdouble x, GLdouble y);
    void glVertexAttribL3d(GLuint index, GLdouble x, GLdouble y, GLdouble z);
    void glVertexAttribL4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
    void glVertexAttribL1dv(GLuint index, const GLdouble* v);
    void glVertexAttribL2dv(GLuint index, const GLdouble* v);
    void glVertexAttribL3dv(GLuint index, const GLdouble* v);
    void glVertexAttribL4dv(GLuint index, const GLdouble* v);
    void glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer);
    void glGetVertexAttribLdv(GLuint index, GLenum pname, GLdouble* params);
    void glProgramUniform1d(GLuint program, GLint location, GLdouble v0);
    void glProgramUniform2d(GLuint program, GLint location, GLdouble v0, GLdouble v1);
    void glProgramUniform3d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2);
    void glProgramUniform4d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3);
    void glProgramUniform1dv(GLuint program, GLint location, GLsizei count, const GLdouble* value);
    void glProgramUniform2dv(GLuint program, GLint location, GLsizei count, const GLdouble* value);
    void glProgramUniform3dv(GLuint program, GLint location, GLsizei count, const GLdouble* value);
    void glProgramUniform4dv(GLuint program, GLint location, GLsizei count, const GLdouble* value);
    void glProgramUniformMatrix2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix2x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix3x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix2x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix4x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix3x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix4x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glGetUniformfv(GLuint program, GLint location, GLfloat* params);
    void glGetUniformiv(GLuint program, GLint location, GLint* params);
    void glGetUniformuiv(GLuint program, GLint location, GLuint* params);
    size_t getMatrixComponentCount(GLenum type);
    void glActiveTexture(GLenum texture);
    void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
    void glGenTextures(GLsizei n, GLuint* textures);
    void glBindTexture(GLenum target, GLuint texture);
    void glUniform1i(GLint location, GLint v0);
    void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    void glUniform1f(GLint location, GLfloat v0);
    GLint gl4es_glGetUniformLocation(GLuint program, const char* name);
    void glGenVertexArray(GLuint* array);
    void glGenVertexArrays(GLsizei n, GLuint* arrays);
    GLuint glCreateVertexArray();
    void glCreateVertexArrays(GLsizei n, GLuint* arrays);
    void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
    void glBindBuffer(GLenum target, GLuint buffer);
    GLint glGetAttribLocation(GLuint program, const char* name);
    void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
    void gl4es_glEnableVertexAttribArray(GLuint index);
    void glProgramUniform1d(GLuint program, GLint location, GLdouble v0);
    void glProgramUniform2d(GLuint program, GLint location, GLdouble v0, GLdouble v1);
    void glProgramUniform3d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2);
    void glProgramUniform4d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3);

    void glProgramUniform1dv(GLuint program, GLint location, GLsizei count, const GLdouble* value);
    void glProgramUniform2dv(GLuint program, GLint location, GLsizei count, const GLdouble* value);
    void glProgramUniform3dv(GLuint program, GLint location, GLsizei count, const GLdouble* value);
    void glProgramUniform4dv(GLuint program, GLint location, GLsizei count, const GLdouble* value);

    void glProgramUniformMatrix2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix2x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix3x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix2x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix4x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix3x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    void glProgramUniformMatrix4x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble* value);
    */

    void threadComputeTask(GLuint startX, GLuint endX, GLuint num_groups_y, GLuint num_groups_z, ComputeShaderCallback computeShader, std::vector<float>& results, int width, int height) {
        for (GLuint z = 0; z < num_groups_z; ++z) {
            for (GLuint y = 0; y < num_groups_y; ++y) {
                for (GLuint x = startX; x < endX; ++x) {
                    computeShader(x, y, z, results, width, height);
                }
            }
        }
    }
    void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
        /*
        computeResults.resize(textureWidth * textureHeight * 4);
        unsigned int numThreads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        GLuint groupSizeX = num_groups_x / numThreads;
        for (unsigned int i = 0; i < numThreads; ++i) {
            GLuint startX = i * groupSizeX;
            GLuint endX = (i == numThreads - 1) ? num_groups_x : startX + groupSizeX;
            //need to add computeShader
            threads.emplace_back(threadComputeTask, startX, endX, num_groups_y, num_groups_z, computeShader, std::ref(computeResults), textureWidth, textureHeight);
        }
        for (auto& thread : threads) {
            thread.join();
        }

        gl4es_glBindTexture(GL_TEXTURE_2D, computeTexture);
        gl4es_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureWidth, textureHeight, GL_RGBA, GL_FLOAT, computeResults.data());
        gl4es_glBindTexture(GL_TEXTURE_2D, 0);  // 解除绑定*/
        LOAD_GLES3(glDispatchCompute);
        gles_glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
    }

    void glDispatchComputeIndirect(GLuint indirectBuffer) {
        /*
        GLuint bufferData[3];
        gl4es_glBindBuffer(GL_ARRAY_BUFFER, indirectBuffer);
        gl4es_glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bufferData), bufferData);
        gl4es_glBindBuffer(GL_ARRAY_BUFFER, 0);

        GLuint num_groups_x = bufferData[0];
        GLuint num_groups_y = bufferData[1];
        GLuint num_groups_z = bufferData[2];

        glDispatchCompute(num_groups_x, num_groups_y, num_groups_z, computeShader);
         */

        LOAD_GLES3(glDispatchComputeIndirect);
        gles_glDispatchComputeIndirect(indirectBuffer);
    }

    void reportUnsupportedFunction(const char* funcName) {
        std::cout << "[Warning] Unsuppoted function: " << funcName << std::endl;
    }

    void glCreateBuffers(GLsizei n, GLuint* buffers) {
        gl4es_glGenBuffers(n, buffers);
    }

    void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
        gl4es_glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        gl4es_glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, renderbuffertarget, renderbuffer);
        gl4es_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
        gl4es_glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);
        gl4es_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFramebuffer);
        gl4es_glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
        gl4es_glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        gl4es_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) {
        gl4es_glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        gl4es_glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, level);
        gl4es_glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void glMemoryBarrier(GLbitfield barriers) {
        /*
        std::lock_guard<std::mutex> lock(barrierMutex);
        // Not completed
        if (barriers & MEMORY_BARRIER_VERTEX_ATTRIB_ARRAY) {
            std::cout << "[MemoryBarrier] Ensuring vertex attribute array consistency." << std::endl;
        }
        if (barriers & MEMORY_BARRIER_ELEMENT_ARRAY) {
            std::cout << "[MemoryBarrier] Ensuring element array consistency." << std::endl;
        }
        if (barriers & MEMORY_BARRIER_UNIFORM_BUFFER) {
            std::cout << "[MemoryBarrier] Ensuring uniform buffer consistency." << std::endl;
        }
        if (barriers & MEMORY_BARRIER_TEXTURE_FETCH) {
            std::cout << "[MemoryBarrier] Ensuring texture fetch consistency." << std::endl;
        }
        if (barriers & MEMORY_BARRIER_FRAMEBUFFER) {
            std::cout << "[MemoryBarrier] Ensuring framebuffer consistency." << std::endl;
        }
        if (barriers & MEMORY_BARRIER_SHADER_IMAGE_ACCESS) {
            std::cout << "[MemoryBarrier] Ensuring shader image access consistency." << std::endl;
        }
        currentBarrierState |= barriers;
        std::cout << "[MemoryBarrier] Updated current barrier state: " << currentBarrierState << std::endl;
         */
    }

    void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
        reportUnsupportedFunction("glBindImageTexture");
        gl4es_glActiveTexture(GL_TEXTURE0 + unit);
        gl4es_glBindTexture(GL_TEXTURE_2D, texture);
    }

    void APIENTRY glDebugMessageCallback(GLDEBUGPROCARB callback, const void* userParam) {
        debugCallback = callback;
    }

    void glTriggerDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message) {
        if (debugCallback) {
            debugCallback(source, type, id, severity, length, message, nullptr);
        }
    }

    void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments) {
        if (target != GL_FRAMEBUFFER) {
            std::cout << "[Error] Invalid target for glInvalidateFramebuffer" << std::endl;
            return;
        }

        for (GLsizei i = 0; i < numAttachments; ++i) {
            GLenum attachment = attachments[i];
            switch (attachment) {
            case GL_COLOR_ATTACHMENT0:
                invalidatedAttachments.insert(COLOR_ATTACHMENT0);
                std::cout << "[Info] Color Attachment 0 invalidated." << std::endl;
                break;
            case GL_COLOR_ATTACHMENT1:
                invalidatedAttachments.insert(COLOR_ATTACHMENT1);
                std::cout << "[Info] Color Attachment 1 invalidated." << std::endl;
                break;
            case GL_COLOR_ATTACHMENT2:
                invalidatedAttachments.insert(COLOR_ATTACHMENT2);
                std::cout << "[Info] Color Attachment 2 invalidated." << std::endl;
                break;
            case GL_DEPTH_ATTACHMENT:
                invalidatedAttachments.insert(DEPTH_ATTACHMENT);
                std::cout << "[Info] Depth Attachment invalidated." << std::endl;
                break;
            case GL_STENCIL_ATTACHMENT:
                invalidatedAttachments.insert(STENCIL_ATTACHMENT);
                std::cout << "[Info] Stencil Attachment invalidated." << std::endl;
                break;
            default:
                std::cout << "[Error] Invalid attachment specified for glInvalidateFramebuffer" << std::endl;
                break;
            }
        }
    }

    long getCurrentTimeMicroseconds() {
        struct timeval time;
        gettimeofday(&time, nullptr);
        return time.tv_sec * 1000000 + time.tv_usec;
    }
    
    /*
    
    GLsync glFenceSync(GLenum condition, GLbitfield flags) {
        GLuint fbo, renderbuffer;
        gl4es_glGenFramebuffers(1, &fbo);
        gl4es_glGenRenderbuffers(1, &renderbuffer);

        gl4es_glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        gl4es_glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);

        gl4es_glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 1, 1);
        gl4es_glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);

        return reinterpret_cast<GLsync>(fbo);
    }
    
    void glDeleteSync(GLsync sync) {
        if (sync == nullptr) return;

        GLuint fbo = static_cast<GLuint>(reinterpret_cast<uintptr_t>(sync));
        gl4es_glDeleteFramebuffers(1, &fbo);
    }

    void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
        if (sync == nullptr) return;

        GLuint fbo = static_cast<GLuint>(reinterpret_cast<uintptr_t>(sync));
        long startTime = getCurrentTimeMicroseconds();
        long elapsedTime = 0;

        GLint status = 0;

        while (status != GL_TRUE) {
            gl4es_glClear(GL_COLOR_BUFFER_BIT);

            gl4es_glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &status);

            if (status != 0) {
                status = GL_TRUE;
            }

            gl4es_glFlush();

            elapsedTime = getCurrentTimeMicroseconds() - startTime;
            if (elapsedTime >= timeout) {
                std::cout << "Timeout occurred while waiting for sync object." << std::endl;
                break;
            }

            usleep(1000);
        }
    }

    void glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values) {
        if (sync == nullptr) return;

        GLuint fbo = static_cast<GLuint>(reinterpret_cast<uintptr_t>(sync));

        if (pname == GL_SYNC_STATUS) {
            GLint status = 0;
            gl4es_glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &status);
            if (values) {
                values[0] = (status != 0) ? GL_TRUE : GL_FALSE;
            }
            if (length) *length = 1;
        }
    }

    GLboolean glIsSync(GLsync sync) {
        if (sync == nullptr) return GL_FALSE;

        GLuint fbo = static_cast<GLuint>(reinterpret_cast<uintptr_t>(sync));

        GLint status = 0;
        gl4es_glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &status);

        return (status != 0) ? GL_TRUE : GL_FALSE;
    }

    GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
        if (sync == nullptr) return GL_TIMEOUT_EXPIRED;

        GLuint fbo = static_cast<GLuint>(reinterpret_cast<uintptr_t>(sync));
        long startTime = getCurrentTimeMicroseconds();
        long elapsedTime = 0;

        GLint status = 0;

        while (status != GL_TRUE) {
            gl4es_glClear(GL_COLOR_BUFFER_BIT);

            gl4es_glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &status);

            if (status != 0) {
                status = GL_TRUE;
            }

            gl4es_glFlush();

            elapsedTime = getCurrentTimeMicroseconds() - startTime;
            if (elapsedTime >= timeout) {
                return GL_TIMEOUT_EXPIRED;
            }

            usleep(1000);
        }
        return GL_CONDITION_SATISFIED;
    }
    
    */
}