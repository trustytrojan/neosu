//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		empty renderer, for debugging and new OS implementations
//
// $NoKeywords: $ni
//===============================================================================//

#include "NullGraphicsInterface.h"

#include "Engine.h"
#include "NullImage.h"
#include "NullRenderTarget.h"
#include "NullShader.h"
#include "VertexArrayObject.h"

Image *NullGraphicsInterface::createImage(std::string filePath, bool mipmapped) {
    return new NullImage(filePath, mipmapped);
}

Image *NullGraphicsInterface::createImage(int width, int height, bool mipmapped) {
    return new NullImage(width, height, mipmapped);
}

RenderTarget *NullGraphicsInterface::createRenderTarget(int x, int y, int width, int height,
                                                        Graphics::MULTISAMPLE_TYPE multiSampleType) {
    return new NullRenderTarget(x, y, width, height, multiSampleType);
}

Shader *NullGraphicsInterface::createShaderFromFile(std::string vertexShaderFilePath,
                                                    std::string fragmentShaderFilePath) {
    return new NullShader(vertexShaderFilePath, fragmentShaderFilePath, false);
}

Shader *NullGraphicsInterface::createShaderFromSource(std::string vertexShader, std::string fragmentShader) {
    return new NullShader(vertexShader, fragmentShader, true);
}

VertexArrayObject *NullGraphicsInterface::createVertexArrayObject(Graphics::PRIMITIVE primitive,
                                                                  Graphics::USAGE_TYPE usage, bool keepInSystemMemory) {
    return new VertexArrayObject(primitive, usage, keepInSystemMemory);
}

void NullGraphicsInterface::drawString(McFont *font, UString text) { ; }
UString NullGraphicsInterface::getVendor() { return "<NULL>"; }
UString NullGraphicsInterface::getModel() { return "<NULL>"; }
UString NullGraphicsInterface::getVersion() { return "<NULL>"; }
