// TODO: fix this

#include "CBaseUIBoxShadow.h"

#include <utility>

#include "ConVar.h"
#include "Engine.h"
#include "GaussianBlurKernel.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "Shader.h"

/*
// HACKHACK: renderer dependent
#include "OpenGLHeaders.h"
*/

CBaseUIBoxShadow::CBaseUIBoxShadow(Color color, float radius, float xPos, float yPos, float xSize, float ySize,
                                   UString name)
    : CBaseUIElement(xPos, yPos, xSize, ySize, std::move(name)) {
    this->shadowColor = color;
    this->color = color;
    this->fRadius = radius;
    this->bNeedsRedraw = true;
    this->bColoredContent = false;

    this->blur =
        new GaussianBlur(0, 0, this->vSize.x + this->fRadius * 2, this->vSize.y + this->fRadius * 2, 91, this->fRadius);
}

CBaseUIBoxShadow::~CBaseUIBoxShadow() { SAFE_DELETE(this->blur); }

void CBaseUIBoxShadow::draw() {
    if(this->bNeedsRedraw) {
        this->render();
        this->bNeedsRedraw = false;
    }

    if(cv::debug_box_shadows.getBool()) {
        g->setColor(0xff00ff00);
        g->drawRect(this->vPos.x - this->fRadius, this->vPos.y - this->fRadius, this->blur->getSize().x,
                    this->blur->getSize().y);
    }

    if(!this->bVisible) return;

    /*
    // HACKHACK: switching blend funcs
    if (this->bColoredContent)
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    */

    g->setColor(this->color);
    this->blur->draw(this->vPos.x - this->fRadius, this->vPos.y - this->fRadius);

    /*
    if (this->bColoredContent)
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    */
}

void CBaseUIBoxShadow::render() {
    /*
    // HACKHACK: switching blend funcs
    if (this->bColoredContent)
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    */

    g->setClipping(false);
    this->blur->enable();
    g->setColor(this->shadowColor);
    g->fillRect(this->fRadius + 2, this->blur->getSize().y / 2.0f - this->vSize.y / 2.0f, this->vSize.x - 4,
                this->vSize.y);
    this->blur->disable();
    g->setClipping(true);

    /*
    if (this->bColoredContent)
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    */
}

void CBaseUIBoxShadow::renderOffscreen() {
    if(this->bNeedsRedraw) {
        this->render();
        this->bNeedsRedraw = false;
    }
}

CBaseUIBoxShadow *CBaseUIBoxShadow::setColor(Color color) {
    this->blur->setColor(color);
    this->color = color;
    return this;
}

CBaseUIBoxShadow *CBaseUIBoxShadow::setShadowColor(Color color) {
    this->bNeedsRedraw = true;
    this->shadowColor = color;
    return this;
}

CBaseUIBoxShadow *CBaseUIBoxShadow::setColoredContent(bool coloredContent) {
    this->bColoredContent = coloredContent;

    if(this->bColoredContent)
        this->blur->setSize(Vector2(this->vSize.x + this->fRadius * 5, this->vSize.y + this->fRadius * 5));

    this->bNeedsRedraw = true;

    return this;
}

void CBaseUIBoxShadow::onResized() {
    if(this->bColoredContent)
        this->blur->setSize(Vector2(this->vSize.x + this->fRadius * 5, this->vSize.y + this->fRadius * 5));
    else
        this->blur->setSize(Vector2(this->vSize.x + this->fRadius * 2, this->vSize.y + this->fRadius * 2));

    this->bNeedsRedraw = true;
}

//*******************************************************************************************************************************************************************************************

//**********************************//
//	Implementation of Gaussianblur  //
//**********************************//

GaussianBlur::GaussianBlur(int x, int y, int width, int height, int kernelSize, float radius) {
    this->vPos = Vector2(x, y);
    this->vSize = Vector2(width, height);
    this->iKernelSize = kernelSize;
    this->fRadius = radius;

    this->kernel = new GaussianBlurKernel(kernelSize, radius, width, height);
    this->rt = resourceManager->createRenderTarget(x, y, width, height);
    this->rt2 = resourceManager->createRenderTarget(x, y, width, height);
    this->blurShader = resourceManager->loadShader("blur.vsh", "blur.fsh", "gblur");
}

GaussianBlur::~GaussianBlur() {
    resourceManager->destroyResource(this->rt);
    this->rt = NULL;
    resourceManager->destroyResource(this->rt2);
    this->rt2 = NULL;
    SAFE_DELETE(this->kernel);
}

void GaussianBlur::draw(int x, int y) {
    this->rt->draw(x, y);

    // g->setColor(0xffff0000);
    // g->fillRect(x,y,m_vSize.x, this->vSize.y);
}

void GaussianBlur::enable() { this->rt->enable(); }

void GaussianBlur::disable() {
    this->rt->disable();

    Shader *blur = this->blurShader;

    this->rt2->enable();
    blur->enable();
    blur->setUniform1i("kernelSize", this->kernel->getKernelSize());
    blur->setUniform1fv("weights", this->kernel->getKernelSize(), this->kernel->getKernel());
    blur->setUniform1fv("offsets", this->kernel->getKernelSize(), this->kernel->getOffsetsVertical());
    blur->setUniform1i("orientation", 1);
    this->rt->draw(0, 0);
    blur->disable();
    this->rt2->disable();

    this->rt->enable();
    blur->enable();
    blur->setUniform1i("kernelSize", this->kernel->getKernelSize());
    blur->setUniform1fv("weights", this->kernel->getKernelSize(), this->kernel->getKernel());
    blur->setUniform1fv("offsets", this->kernel->getKernelSize(), this->kernel->getOffsetsHorizontal());
    blur->setUniform1i("orientation", 0);
    this->rt2->draw(this->vPos.x, this->vPos.y);
    blur->disable();
    this->rt->disable();
}

void GaussianBlur::setColor(Color color) {
    this->rt->setColor(color);
    this->rt2->setColor(color);
}

void GaussianBlur::setSize(Vector2 size) {
    this->vSize = size;

    SAFE_DELETE(this->kernel);
    this->kernel = new GaussianBlurKernel(this->iKernelSize, this->fRadius, this->vSize.x, this->vSize.y);

    this->rt->rebuild(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y);
    this->rt2->rebuild(this->vPos.x, this->vPos.y, this->vSize.x, this->vSize.y);
}
