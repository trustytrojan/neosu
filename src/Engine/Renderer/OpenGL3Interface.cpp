#include "OpenGL3Interface.h"

#ifdef MCENGINE_FEATURE_OPENGL

#include "Camera.h"
#include "ConVar.h"
#include "Engine.h"
#include "Font.h"
#include "OpenGL3VertexArrayObject.h"
#include "OpenGLHeaders.h"
#include "OpenGLImage.h"
#include "OpenGLRenderTarget.h"
#include "OpenGLShader.h"

using namespace std;

OpenGL3Interface::OpenGL3Interface() : Graphics() {
    // renderer
    this->bInScene = false;
    this->vResolution = engine->getScreenSize();  // initial viewport size = window size

    this->shaderTexturedGeneric = NULL;

    this->iShaderTexturedGenericAttribPosition = 0;
    this->iShaderTexturedGenericAttribUV = 1;
    this->iShaderTexturedGenericAttribCol = 2;
    this->bShaderTexturedGenericIsTextureEnabled = false;
    this->iVA = 0;
    this->iVBOVertices = 0;
    this->iVBOTexcoords = 0;
    this->iVBOTexcolors = 0;

    // persistent vars
    this->color = 0xffffffff;
}

OpenGL3Interface::~OpenGL3Interface() {
    SAFE_DELETE(this->shaderTexturedGeneric);

    glDeleteVertexArrays(1, &this->iVA);
}

void OpenGL3Interface::init() {
	// resolve GL functions
	if (!gladLoadGL())
	{
		debugLog("gladLoadGL() error\n");
		engine->showMessageErrorFatal("OpenGL Error", "Couldn't gladLoadGL()!\nThe engine will exit now.");
		engine->shutdown();
		return;
	}
	debugLogF("OpenGL Version: {}\n", reinterpret_cast<const char *>(glGetString(GL_VERSION)));

    // enable
    glEnable(GL_BLEND);

    // disable
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    // blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    // culling
    glFrontFace(GL_CCW);

    // setWireframe(true);

    std::string texturedGenericV =
        "#version 130\n"
        "\n"
        "in vec3 position;\n"
        "in vec2 uv;\n"
        "in vec4 vcolor;\n"
        "out vec2 texcoords;\n"
        "out vec4 texcolor;\n"
        "\n"
        "uniform int type;\n"
        "uniform mat4 mvp;\n"
        "\n"
        "void main() {\n"
        "	gl_Position =  mvp * vec4(position, 1.0);\n"
        "	if (type == 1)\n"
        "	{\n"
        "		texcoords = uv;\n"
        "	}\n"
        "	else if (type == 2)\n"
        "	{\n"
        "		texcolor = vcolor;"
        "	}\n"
        "}\n"
        "\n";

    std::string texturedGenericP =
        "#version 130\n"
        "\n"
        "out vec4 color;\n"
        "in vec2 texcoords;\n"
        "in vec4 texcolor;\n"
        "\n"
        "uniform int type;\n"
        "uniform vec4 col;\n"
        "uniform sampler2D tex;\n"
        "\n"
        "void main() {\n"
        "	color = col;\n"
        "	if (type == 1)\n"
        "	{\n"
        "		color = texture(tex, texcoords) * col;\n"
        "	}\n"
        "	else if (type == 2)\n"
        "	{\n"
        "		color = texcolor;"
        "	}\n"
        "}\n"
        "\n";
    this->shaderTexturedGeneric = (OpenGLShader *)this->createShaderFromSource(texturedGenericV, texturedGenericP);
    this->shaderTexturedGeneric->load();

    glGenVertexArrays(1, &this->iVA);
    glGenBuffers(1, &this->iVBOVertices);
    glGenBuffers(1, &this->iVBOTexcoords);
    glGenBuffers(1, &this->iVBOTexcolors);

    this->iShaderTexturedGenericAttribPosition = this->shaderTexturedGeneric->getAttribLocation("position");
    this->iShaderTexturedGenericAttribUV = this->shaderTexturedGeneric->getAttribLocation("uv");
    this->iShaderTexturedGenericAttribCol = this->shaderTexturedGeneric->getAttribLocation("vcolor");

    glBindVertexArray(this->iVA);

    // TODO: handle cases where more than 512 elements are in a vao
    glBindBuffer(GL_ARRAY_BUFFER, this->iVBOVertices);
    glVertexAttribPointer(this->iShaderTexturedGenericAttribPosition, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
                          (GLvoid *)0);
    glBufferData(GL_ARRAY_BUFFER, 512 * sizeof(Vector3), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(this->iShaderTexturedGenericAttribPosition);

    glBindBuffer(GL_ARRAY_BUFFER, this->iVBOTexcoords);
    glVertexAttribPointer(this->iShaderTexturedGenericAttribUV, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat),
                          (GLvoid *)0);
    glBufferData(GL_ARRAY_BUFFER, 512 * sizeof(Vector2), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(this->iShaderTexturedGenericAttribUV);

    glBindBuffer(GL_ARRAY_BUFFER, this->iVBOTexcolors);
    glVertexAttribPointer(this->iShaderTexturedGenericAttribCol, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                          (GLvoid *)0);
    glBufferData(GL_ARRAY_BUFFER, 512 * sizeof(Vector4), NULL, GL_STREAM_DRAW);
    glEnableVertexAttribArray(this->iShaderTexturedGenericAttribCol);
}

void OpenGL3Interface::beginScene() {
    this->bInScene = true;

    Matrix4 defaultProjectionMatrix =
        Camera::buildMatrixOrtho2D(0, this->vResolution.x, this->vResolution.y, 0, -1.0f, 1.0f);

    // push main transforms
    this->pushTransform();
    this->setProjectionMatrix(defaultProjectionMatrix);
    this->translate(cv_r_globaloffset_x.getFloat(), cv_r_globaloffset_y.getFloat());

    // and apply them
    this->updateTransform();

    // set clear color and clear
    // glClearColor(1, 1, 1, 1);
    // glClearColor(0.9568f, 0.9686f, 0.9882f, 1);
    glClearColor(0, 0, 0, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // display any errors of previous frames
    this->handleGLErrors();

    // enable default shader
    this->shaderTexturedGeneric->enable();
}

void OpenGL3Interface::endScene() {
    this->popTransform();

    this->checkStackLeaks();

    if(this->clipRectStack.size() > 0) {
        engine->showMessageErrorFatal("ClipRect Stack Leak", "Make sure all push*() have a pop*()!");
        engine->shutdown();
    }

    this->bInScene = false;
}

void OpenGL3Interface::clearDepthBuffer() { glClear(GL_DEPTH_BUFFER_BIT); }

void OpenGL3Interface::setColor(Color color) {
    this->color = color;
    this->shaderTexturedGeneric->setUniform4f(
        "col", ((unsigned char)(this->color >> 16)) / 255.0f, ((unsigned char)(this->color >> 8)) / 255.0f,
        ((unsigned char)(this->color >> 0)) / 255.0f, ((unsigned char)(this->color >> 24)) / 255.0f);
}

void OpenGL3Interface::setAlpha(float alpha) {
    this->color &= 0x00ffffff;
    this->color |= ((int)(255.0f * alpha)) << 24;
    this->setColor(this->color);
}

void OpenGL3Interface::drawPixels(int x, int y, int width, int height, Graphics::DRAWPIXELS_TYPE type,
                                  const void *pixels) {
    // TODO
    /*
    glRasterPos2i(x, y+height); // '+height' because of opengl bottom left origin, but engine top left origin
    glDrawPixels(width, height, GL_RGBA, (type == Graphics::DRAWPIXELS_TYPE::DRAWPIXELS_UBYTE ? GL_UNSIGNED_BYTE :
    GL_FLOAT), pixels);
    */
}

void OpenGL3Interface::drawPixel(int x, int y) {
    this->updateTransform();

    VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_LINES);
    vao.addVertex(x - 0.45f, y - 0.45f);
    vao.addVertex(x + 0.45f, y + 0.45f);
    this->drawVAO(&vao);
}

void OpenGL3Interface::drawLine(int x1, int y1, int x2, int y2) {
    this->updateTransform();

    VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_LINES);
    vao.addVertex(x1 + 0.5f, y1 + 0.5f);
    vao.addVertex(x2 + 0.5f, y2 + 0.5f);
    this->drawVAO(&vao);
}

void OpenGL3Interface::drawLine(Vector2 pos1, Vector2 pos2) { this->drawLine(pos1.x, pos1.y, pos2.x, pos2.y); }

void OpenGL3Interface::drawRect(int x, int y, int width, int height) {
    this->drawLine(x, y, x + width, y);
    this->drawLine(x, y, x, y + height);
    this->drawLine(x, y + height, x + width + 1, y + height);
    this->drawLine(x + width, y, x + width, y + height);
}

void OpenGL3Interface::drawRect(int x, int y, int width, int height, Color top, Color right, Color bottom, Color left) {
    this->setColor(top);
    this->drawLine(x, y, x + width, y);
    this->setColor(left);
    this->drawLine(x, y, x, y + height);
    this->setColor(bottom);
    this->drawLine(x, y + height, x + width + 1, y + height);
    this->setColor(right);
    this->drawLine(x + width, y, x + width, y + height);
}

void OpenGL3Interface::fillRect(int x, int y, int width, int height) {
    this->updateTransform();

    VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
    vao.addVertex(x, y);
    vao.addVertex(x, y + height);
    vao.addVertex(x + width, y + height);
    vao.addVertex(x + width, y);
    this->drawVAO(&vao);
}

void OpenGL3Interface::fillRoundedRect(int x, int y, int width, int height, int radius) {
    this->updateTransform();

    // TODO
    /*
    float xOffset= x + radius;
    float yOffset = y + radius;

    double i = 0;
    double factor = 0.05;

    glDisable(GL_TEXTURE_2D);
    glBegin(GL_POLYGON);

            for (i=PI; i<=(1.5*PI); i+=factor)
            {
                    glVertex2d(radius* std::cos(i) + xOffset, radius * std::sin(i) + yOffset); // top left
            }

            xOffset = x + width - radius;
            for (i=(1.5*PI); i<=(2*PI); i+=factor)
            {
                    glVertex2d(radius* std::cos(i) + xOffset, radius * std::sin(i) + yOffset); // top right
            }

            yOffset = y + height - radius;
            for (i=0; i<=(0.5*PI); i+=factor)
            {
                    glVertex2d(radius* std::cos(i) + xOffset, radius * std::sin(i) + yOffset); // bottom right
            }

            xOffset = x + radius;
            for (i=(0.5*PI); i<=PI; i+=factor)
            {
                    glVertex2d(radius* std::cos(i) + xOffset, radius * std::sin(i) + yOffset); // bottom left
            }

    glEnd();
    */
}

void OpenGL3Interface::fillGradient(int x, int y, int width, int height, Color topLeftColor, Color topRightColor,
                                    Color bottomLeftColor, Color bottomRightColor) {
    this->updateTransform();

    VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
    vao.addVertex(x, y);
    vao.addColor(topLeftColor);
    vao.addVertex(x + width, y);
    vao.addColor(topRightColor);
    vao.addVertex(x + width, y + height);
    vao.addColor(bottomRightColor);
    vao.addVertex(x, y + height);
    vao.addColor(bottomLeftColor);
    this->drawVAO(&vao);
}

void OpenGL3Interface::drawQuad(int x, int y, int width, int height) {
    this->updateTransform();

    VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
    vao.addVertex(x, y);
    vao.addTexcoord(0, 0);
    vao.addVertex(x, y + height);
    vao.addTexcoord(0, 1);
    vao.addVertex(x + width, y + height);
    vao.addTexcoord(1, 1);
    vao.addVertex(x + width, y);
    vao.addTexcoord(1, 0);
    this->drawVAO(&vao);
}

void OpenGL3Interface::drawQuad(Vector2 topLeft, Vector2 topRight, Vector2 bottomRight, Vector2 bottomLeft,
                                Color topLeftColor, Color topRightColor, Color bottomRightColor,
                                Color bottomLeftColor) {
    this->updateTransform();

    VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
    vao.addVertex(topLeft.x, topLeft.y);
    vao.addColor(topLeftColor);
    vao.addTexcoord(0, 0);
    vao.addVertex(bottomLeft.x, bottomLeft.y);
    vao.addColor(bottomLeftColor);
    vao.addTexcoord(0, 1);
    vao.addVertex(bottomRight.x, bottomRight.y);
    vao.addColor(bottomRightColor);
    vao.addTexcoord(1, 1);
    vao.addVertex(topRight.x, topRight.y);
    vao.addColor(topRightColor);
    vao.addTexcoord(1, 0);
    this->drawVAO(&vao);
}

void OpenGL3Interface::drawImage(Image *image, AnchorPoint anchor) {
    if(image == NULL) {
        debugLog("WARNING: Tried to draw image with NULL texture!\n");
        return;
    }
    if(!image->isReady()) return;

    this->updateTransform();

    f32 width = image->getWidth();
    f32 height = image->getHeight();

    f32 x, y;
    switch(anchor) {
        case AnchorPoint::CENTER:
            x = -width / 2;
            y = -height / 2;
            break;
        case AnchorPoint::TOP_LEFT:
            x = 0;
            y = 0;
            break;
        case AnchorPoint::TOP_RIGHT:
            x = -width;
            y = 0;
            break;
        case AnchorPoint::BOTTOM_LEFT:
            x = 0;
            y = -height;
            break;
        case AnchorPoint::LEFT:
            x = 0;
            y = -height / 2;
            break;
        default:
            abort();  // :-)
    }

    VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);
    vao.addVertex(x, y);
    vao.addTexcoord(0, 0);
    vao.addVertex(x, y + height);
    vao.addTexcoord(0, 1);
    vao.addVertex(x + width, y + height);
    vao.addTexcoord(1, 1);
    vao.addVertex(x + width, y);
    vao.addTexcoord(1, 0);

    image->bind();
    this->drawVAO(&vao);
    image->unbind();

    if(cv_r_debug_drawimage.getBool()) {
        this->setColor(0xbbff00ff);
        this->drawRect(x, y, width - 1, height - 1);
    }
}

void OpenGL3Interface::drawString(McFont *font, UString text) {
    if(font == NULL || text.length() < 1 || !font->isReady()) return;

    this->updateTransform();

    font->drawString(text.toUtf8());
}

void OpenGL3Interface::drawVAO(VertexArrayObject *vao) {
    if(vao == NULL) return;

    this->updateTransform();

    // if baked, then we can directly draw the buffer
    if(vao->isReady()) {
        OpenGL3VertexArrayObject *glvao = (OpenGL3VertexArrayObject *)vao;

        // configure shader
        if(glvao->getNumTexcoords0() > 0) {
            if(!this->bShaderTexturedGenericIsTextureEnabled) {
                this->shaderTexturedGeneric->setUniform1i("type", 1);
                this->bShaderTexturedGenericIsTextureEnabled = true;
            }
        } else if(this->bShaderTexturedGenericIsTextureEnabled) {
            this->bShaderTexturedGenericIsTextureEnabled = false;
            this->shaderTexturedGeneric->setUniform1i("type", 0);
        }

        // draw
        glvao->draw();
        return;
    }

    const std::vector<Vector3> &vertices = vao->getVertices();
    /// const std::vector<Vector3> &normals = vao->getNormals();
    const std::vector<std::vector<Vector2>> &texcoords = vao->getTexcoords();
    const std::vector<Color> &vcolors = vao->getColors();

    if(vertices.size() < 2) return;

    // no support for quads, because fuck you
    // rewrite all quads into triangles
    std::vector<Vector3> finalVertices = vertices;
    std::vector<std::vector<Vector2>> finalTexcoords = texcoords;
    std::vector<Vector4> colors;
    std::vector<Vector4> finalColors;

    for(size_t i = 0; i < vcolors.size(); i++) {
        Vector4 color = Vector4(COLOR_GET_Rf(vcolors[i]), COLOR_GET_Gf(vcolors[i]), COLOR_GET_Bf(vcolors[i]),
                                COLOR_GET_Af(vcolors[i]));
        colors.push_back(color);
        finalColors.push_back(color);
    }
    int maxColorIndex = colors.size() - 1;

    Graphics::PRIMITIVE primitive = vao->getPrimitive();
    if(primitive == Graphics::PRIMITIVE::PRIMITIVE_QUADS) {
        finalVertices.clear();
        for(size_t t = 0; t < finalTexcoords.size(); t++) {
            finalTexcoords[t].clear();
        }
        finalColors.clear();
        primitive = Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES;

        if(vertices.size() > 3) {
            for(size_t i = 0; i < vertices.size(); i += 4) {
                finalVertices.push_back(vertices[i + 0]);
                finalVertices.push_back(vertices[i + 1]);
                finalVertices.push_back(vertices[i + 2]);

                for(size_t t = 0; t < texcoords.size(); t++) {
                    finalTexcoords[t].push_back(texcoords[t][i + 0]);
                    finalTexcoords[t].push_back(texcoords[t][i + 1]);
                    finalTexcoords[t].push_back(texcoords[t][i + 2]);
                }

                if(colors.size() > 0) {
                    finalColors.push_back(colors[std::clamp<int>(i + 0, 0, maxColorIndex)]);
                    finalColors.push_back(colors[std::clamp<int>(i + 1, 0, maxColorIndex)]);
                    finalColors.push_back(colors[std::clamp<int>(i + 2, 0, maxColorIndex)]);
                }

                finalVertices.push_back(vertices[i + 0]);
                finalVertices.push_back(vertices[i + 2]);
                finalVertices.push_back(vertices[i + 3]);

                for(size_t t = 0; t < texcoords.size(); t++) {
                    finalTexcoords[t].push_back(texcoords[t][i + 0]);
                    finalTexcoords[t].push_back(texcoords[t][i + 2]);
                    finalTexcoords[t].push_back(texcoords[t][i + 3]);
                }

                if(colors.size() > 0) {
                    finalColors.push_back(colors[std::clamp<int>(i + 0, 0, maxColorIndex)]);
                    finalColors.push_back(colors[std::clamp<int>(i + 2, 0, maxColorIndex)]);
                    finalColors.push_back(colors[std::clamp<int>(i + 3, 0, maxColorIndex)]);
                }
            }
        }
    }

    // upload vertices to gpu
    if(finalVertices.size() > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, this->iVBOVertices);
        glBufferSubData(GL_ARRAY_BUFFER, 0, finalVertices.size() * sizeof(Vector3), &(finalVertices[0]));
    }

    // upload texcoords to gpu
    if(finalTexcoords.size() > 0 && finalTexcoords[0].size() > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, this->iVBOTexcoords);
        glBufferSubData(GL_ARRAY_BUFFER, 0, finalTexcoords[0].size() * sizeof(Vector2), &(finalTexcoords[0][0]));
    }

    // upload vertex colors to gpu
    if(finalColors.size() > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, this->iVBOTexcolors);
        glBufferSubData(GL_ARRAY_BUFFER, 0, finalColors.size() * sizeof(Vector4), &(finalColors[0]));
    }

    // TODO: multitexturing support
    // TODO: textured vertexcolors
    if(finalTexcoords.size() > 0 && finalTexcoords[0].size() > 0) {
        if(!this->bShaderTexturedGenericIsTextureEnabled) {
            this->bShaderTexturedGenericIsTextureEnabled = true;
            // GLint currentlyBoundTextureUnit = 0;
            // glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentlyBoundTextureUnit);

            this->shaderTexturedGeneric->setUniform1i("type", 1);
            // m_shaderTexturedGeneric->setUniform1i("tex", currentlyBoundTextureUnit);
        }
    } else if(this->bShaderTexturedGenericIsTextureEnabled) {
        this->bShaderTexturedGenericIsTextureEnabled = false;
        this->shaderTexturedGeneric->setUniform1i("type", 0);
    }

    if(finalColors.size() > 0) {
        this->bShaderTexturedGenericIsTextureEnabled = false;
        this->shaderTexturedGeneric->setUniform1i("type", 2);
    }

    // draw it
    glDrawArrays(primitiveToOpenGL(primitive), 0, finalVertices.size());
}

void OpenGL3Interface::setClipRect(McRect clipRect) {
    if(cv_r_debug_disable_cliprect.getBool()) return;
    // if (this->bIs3DScene) return; // HACKHACK:TODO:

    // HACKHACK: compensate for viewport changes caused by RenderTargets!
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    // debugLog("viewport = %i, %i, %i, %i\n", viewport[0], viewport[1], viewport[2], viewport[3]);

    glEnable(GL_SCISSOR_TEST);
    glScissor((int)clipRect.getX() + viewport[0],
              viewport[3] - ((int)clipRect.getY() - viewport[1] - 1 + (int)clipRect.getHeight()),
              (int)clipRect.getWidth(), (int)clipRect.getHeight());

    // debugLog("scissor = %i, %i, %i, %i\n", (int)clipRect.getX()+viewport[0],
    // viewport[3]-((int)clipRect.getY()-viewport[1]-1+(int)clipRect.getHeight()), (int)clipRect.getWidth(),
    // (int)clipRect.getHeight());
}

void OpenGL3Interface::pushClipRect(McRect clipRect) {
    if(this->clipRectStack.size() > 0)
        this->clipRectStack.push(this->clipRectStack.top().intersect(clipRect));
    else
        this->clipRectStack.push(clipRect);

    this->setClipRect(this->clipRectStack.top());
}

void OpenGL3Interface::popClipRect() {
    this->clipRectStack.pop();

    if(this->clipRectStack.size() > 0)
        this->setClipRect(this->clipRectStack.top());
    else
        this->setClipping(false);
}

void OpenGL3Interface::pushStencil() {
    // init and clear
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);

    // set mask
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
}

void OpenGL3Interface::fillStencil(bool inside) {
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilFunc(GL_NOTEQUAL, inside ? 0 : 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void OpenGL3Interface::popStencil() { glDisable(GL_STENCIL_TEST); }

void OpenGL3Interface::setClipping(bool enabled) {
    if(enabled) {
        if(this->clipRectStack.size() > 0) glEnable(GL_SCISSOR_TEST);
    } else
        glDisable(GL_SCISSOR_TEST);
}

void OpenGL3Interface::setAlphaTesting(bool enabled) {
    if(enabled)
        glEnable(GL_ALPHA_TEST);
    else
        glDisable(GL_ALPHA_TEST);
}

void OpenGL3Interface::setAlphaTestFunc(COMPARE_FUNC alphaFunc, float ref) {
    glAlphaFunc(compareFuncToOpenGL(alphaFunc), ref);
}

void OpenGL3Interface::setBlending(bool enabled) {
    if(enabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
}

void OpenGL3Interface::setBlendMode(BLEND_MODE blendMode) {
    switch(blendMode) {
        case BLEND_MODE::BLEND_MODE_ALPHA:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BLEND_MODE::BLEND_MODE_ADDITIVE:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case BLEND_MODE::BLEND_MODE_PREMUL_ALPHA:
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BLEND_MODE::BLEND_MODE_PREMUL_COLOR:
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
    }
}

void OpenGL3Interface::setDepthBuffer(bool enabled) {
    if(enabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void OpenGL3Interface::setCulling(bool culling) {
    if(culling)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}

void OpenGL3Interface::setAntialiasing(bool aa) {
    if(aa)
        glEnable(GL_MULTISAMPLE);
    else
        glDisable(GL_MULTISAMPLE);
}

void OpenGL3Interface::setWireframe(bool enabled) {
    if(enabled)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void OpenGL3Interface::flush() { glFlush(); }

std::vector<unsigned char> OpenGL3Interface::getScreenshot() {
    // TODO
    std::vector<unsigned char> temp;
    return temp;
}

UString OpenGL3Interface::getVendor() {
    // TODO
    return UString("TODO");
}

UString OpenGL3Interface::getModel() {
    // TODO
    return UString("TODO");
}

UString OpenGL3Interface::getVersion() {
    // TODO
    return UString("-1");
}

int OpenGL3Interface::getVRAMTotal() {
    // TODO
    return -1;
}

int OpenGL3Interface::getVRAMRemaining() {
    // TODO
    return -1;
}

void OpenGL3Interface::onResolutionChange(Vector2 newResolution) {
    // rebuild viewport
    this->vResolution = newResolution;
    glViewport(0, 0, this->vResolution.x, this->vResolution.y);

    // special case: custom rendertarget resolution rendering, update active projection matrix immediately
    if(this->bInScene) {
        this->projectionTransformStack.top() =
            Camera::buildMatrixOrtho2D(0, this->vResolution.x, this->vResolution.y, 0, -1.0f, 1.0f);
        this->bTransformUpToDate = false;
    }
}

Image *OpenGL3Interface::createImage(std::string filePath, bool mipmapped, bool keepInSystemMemory) {
    return new OpenGLImage(filePath, mipmapped, keepInSystemMemory);
}

Image *OpenGL3Interface::createImage(int width, int height, bool mipmapped, bool keepInSystemMemory) {
    return new OpenGLImage(width, height, mipmapped, keepInSystemMemory);
}

RenderTarget *OpenGL3Interface::createRenderTarget(int x, int y, int width, int height,
                                                   Graphics::MULTISAMPLE_TYPE multiSampleType) {
    return new OpenGLRenderTarget(x, y, width, height, multiSampleType);
}

Shader *OpenGL3Interface::createShaderFromFile(std::string vertexShaderFilePath, std::string fragmentShaderFilePath) {
    return new OpenGLShader(vertexShaderFilePath, fragmentShaderFilePath, false);
}

Shader *OpenGL3Interface::createShaderFromSource(std::string vertexShader, std::string fragmentShader) {
    return new OpenGLShader(vertexShader, fragmentShader, true);
}

VertexArrayObject *OpenGL3Interface::createVertexArrayObject(Graphics::PRIMITIVE primitive, Graphics::USAGE_TYPE usage,
                                                             bool keepInSystemMemory) {
    return new OpenGL3VertexArrayObject(primitive, usage, keepInSystemMemory);
}

void OpenGL3Interface::onTransformUpdate(Matrix4 &projectionMatrix, Matrix4 &worldMatrix) {
    this->projectionMatrix = projectionMatrix;
    this->worldMatrix = worldMatrix;

    this->MP = this->projectionMatrix * this->worldMatrix;
    this->shaderTexturedGeneric->setUniformMatrix4fv("mvp", this->MP);
}

void OpenGL3Interface::handleGLErrors() {
    int error = glGetError();
    if(error != 0) debugLog("OpenGL Error: %i on frame %i\n", error, engine->getFrameCount());
}

int OpenGL3Interface::primitiveToOpenGL(Graphics::PRIMITIVE primitive) {
    switch(primitive) {
        case Graphics::PRIMITIVE::PRIMITIVE_LINES:
            return GL_LINES;
        case Graphics::PRIMITIVE::PRIMITIVE_LINE_STRIP:
            return GL_LINE_STRIP;
        case Graphics::PRIMITIVE::PRIMITIVE_TRIANGLES:
            return GL_TRIANGLES;
        case Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_FAN:
            return GL_TRIANGLE_FAN;
        case Graphics::PRIMITIVE::PRIMITIVE_TRIANGLE_STRIP:
            return GL_TRIANGLE_STRIP;
        case Graphics::PRIMITIVE::PRIMITIVE_QUADS:
            return GL_QUADS;
    }

    return GL_TRIANGLES;
}

int OpenGL3Interface::compareFuncToOpenGL(Graphics::COMPARE_FUNC compareFunc) {
    switch(compareFunc) {
        case Graphics::COMPARE_FUNC::COMPARE_FUNC_NEVER:
            return GL_NEVER;
        case Graphics::COMPARE_FUNC::COMPARE_FUNC_LESS:
            return GL_LESS;
        case Graphics::COMPARE_FUNC::COMPARE_FUNC_EQUAL:
            return GL_EQUAL;
        case Graphics::COMPARE_FUNC::COMPARE_FUNC_LESSEQUAL:
            return GL_LEQUAL;
        case Graphics::COMPARE_FUNC::COMPARE_FUNC_GREATER:
            return GL_GREATER;
        case Graphics::COMPARE_FUNC::COMPARE_FUNC_NOTEQUAL:
            return GL_NOTEQUAL;
        case Graphics::COMPARE_FUNC::COMPARE_FUNC_GREATEREQUAL:
            return GL_GEQUAL;
        case Graphics::COMPARE_FUNC::COMPARE_FUNC_ALWAYS:
            return GL_ALWAYS;
    }

    return GL_ALWAYS;
}

#endif
