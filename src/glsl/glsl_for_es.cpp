#include "glsl_for_es.h"

#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <glslang/Include/Types.h>
#include <glslang/Public/ShaderLang.h>
#include </spirv_cross/spirv_cross.hpp>
#include </spirv_cross/spirv_glsl.hpp>
#include </spirv_cross/spirv_cpp.hpp>
#include </spirv_cross/spirv.hpp>
#include <iostream>

typedef std::vector<uint32_t> Spirv;


std::string spirv_to_glsl(const std::vector<uint32_t>& spirv) {
    try {
        spirv_cross::CompilerGLSL glsl_compiler(spirv);
        spirv_cross::CompilerGLSL::Options options;
        options.version = 320;
        options.es = true;

        glsl_compiler.set_common_options(options);

        return glsl_compiler.compile();
    } catch (const std::exception& e) {
        // 捕获任何异常并返回错误信息
        printf("%s",(std::string("GLSL compilation failed: ") + e.what()).c_str());
    }
}

EShLanguage openglShaderTypeToEShLanguage(GLenum shaderType) {
    switch (shaderType) {
        case GL_VERTEX_SHADER:
            return EShLangVertex;
        case GL_TESS_CONTROL_SHADER:
            return EShLangTessControl;
        case GL_TESS_EVALUATION_SHADER:
            return EShLangTessEvaluation;
        case GL_GEOMETRY_SHADER:
            return EShLangGeometry;
        case GL_FRAGMENT_SHADER:
            return EShLangFragment;
        case GL_COMPUTE_SHADER:
            return EShLangCompute;
        default:
            std::cerr << "Unsupported shader type!" << std::endl;
            return EShLangCount;
    }
}

char* GLSLtoGLSLES(char* glsl_code, GLenum glsl_type) {
    printf("GLSLtoGLSLES:\n");
    //printf("%s\n",glsl_code);
    //InitializeGlslang();
    Spirv spirv_code;
    std::string log;
/*
    SpirvHelper::Init();
    if(SpirvHelper::GLSLtoSPV(openglShaderTypeToEShLanguage(glsl_type),glsl_code, spirv_code))
        printf("GLSLtoSPIRV right\n");
    else
        printf("GLSLtoSPIRV error\n");
    SpirvHelper::Finalize();*/
    printf("toSPIRV LOG:\n");
    //printf("%s\n", log.c_str());
    //FinalizeGlslang();
    printf("to GLSL\n");
    std::string glsl_es_code;
    glsl_es_code = spirv_to_glsl(spirv_code);
    printf("GLSLES:\n");
    //printf("%s\n", glsl_es_code.c_str());
    char* result = new char[glsl_es_code.size() + 1];  
    std::copy(glsl_es_code.begin(), glsl_es_code.end(), result);
    result[glsl_es_code.size()] = '\0';
    printf("GLSLtoGLSLES END\n");
    return result;
}

