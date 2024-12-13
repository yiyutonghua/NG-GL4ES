#ifndef GLSL_FOR_ES
#define GLSL_FOR_ES
#include <stdio.h>
#include <GL/gl.h>

#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif
__attribute__((visibility("default"))) extern char *GLSLtoGLSLES(char *glsl_code, GLenum glsl_type);
#ifdef __cplusplus
}
#endif
#endif