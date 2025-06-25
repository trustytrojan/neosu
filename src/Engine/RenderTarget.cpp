#include "RenderTarget.h"

#include "ConVar.h"
#include "Engine.h"
#include "VertexArrayObject.h"

RenderTarget::RenderTarget(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType) {
    this->vPos = Vector2(x, y);
    this->vSize = Vector2(width, height);
    this->multiSampleType = multiSampleType;

    this->bClearColorOnDraw = true;
    this->bClearDepthOnDraw = true;

    this->color = 0xffffffff;
    this->clearColor = 0x00000000;
}

void RenderTarget::draw(int x, int y) {
    if(!m_bReady) {
        if(cv_debug_rt.getBool()) debugLog("WARNING: RenderTarget is not ready!\n");
        return;
    }

    g->setColor(this->color);

    this->bind();
    {
        // NOTE: can't use drawQuad because opengl fucks shit up and flips framebuffers vertically (due to bottom left
        // opengl origin and top left engine origin)
        // g->drawQuad(x, y, this->vSize.x, this->vSize.y);

        // compromise: all draw*() functions of the RenderTarget class guarantee correctly flipped images.
        //             if bind() is used, no guarantee can be made about the texture orientation (assuming an anonymous
        //             Renderer)

        VertexArrayObject vao;
        {
            vao.addTexcoord(0, 1);
            vao.addVertex(x, y);

            vao.addTexcoord(0, 0);
            vao.addVertex(x, y + this->vSize.y);

            vao.addTexcoord(1, 0);
            vao.addVertex(x + this->vSize.x, y + this->vSize.y);

            vao.addTexcoord(1, 0);
            vao.addVertex(x + this->vSize.x, y + this->vSize.y);

            vao.addTexcoord(1, 1);
            vao.addVertex(x + this->vSize.x, y);

            vao.addTexcoord(0, 1);
            vao.addVertex(x, y);
        }
        g->drawVAO(&vao);
    }
    this->unbind();
}

void RenderTarget::draw(int x, int y, int width, int height) {
    if(!m_bReady) {
        if(cv_debug_rt.getBool()) debugLog("WARNING: RenderTarget is not ready!\n");
        return;
    }

    g->setColor(this->color);

    this->bind();
    {
        VertexArrayObject vao;
        {
            vao.addTexcoord(0, 1);
            vao.addVertex(x, y);

            vao.addTexcoord(0, 0);
            vao.addVertex(x, y + height);

            vao.addTexcoord(1, 0);
            vao.addVertex(x + width, y + height);

            vao.addTexcoord(1, 0);
            vao.addVertex(x + width, y + height);

            vao.addTexcoord(1, 1);
            vao.addVertex(x + width, y);

            vao.addTexcoord(0, 1);
            vao.addVertex(x, y);
        }
        g->drawVAO(&vao);
    }
    this->unbind();
}

void RenderTarget::drawRect(int x, int y, int width, int height) {
    if(!m_bReady) {
        if(cv_debug_rt.getBool()) debugLog("WARNING: RenderTarget is not ready!\n");
        return;
    }

    const float texCoordWidth0 = x / this->vSize.x;
    const float texCoordWidth1 = (x + width) / this->vSize.x;
    const float texCoordHeight1 = 1.0f - y / this->vSize.y;
    const float texCoordHeight0 = 1.0f - (y + height) / this->vSize.y;

    g->setColor(this->color);

    this->bind();
    {
        VertexArrayObject vao;
        {
            vao.addTexcoord(texCoordWidth0, texCoordHeight1);
            vao.addVertex(x, y);

            vao.addTexcoord(texCoordWidth0, texCoordHeight0);
            vao.addVertex(x, y + height);

            vao.addTexcoord(texCoordWidth1, texCoordHeight0);
            vao.addVertex(x + width, y + height);

            vao.addTexcoord(texCoordWidth1, texCoordHeight0);
            vao.addVertex(x + width, y + height);

            vao.addTexcoord(texCoordWidth1, texCoordHeight1);
            vao.addVertex(x + width, y);

            vao.addTexcoord(texCoordWidth0, texCoordHeight1);
            vao.addVertex(x, y);
        }
        g->drawVAO(&vao);
    }
    this->unbind();
}

void RenderTarget::rebuild(int x, int y, int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType) {
    this->vPos.x = x;
    this->vPos.y = y;
    this->vSize.x = width;
    this->vSize.y = height;
    this->multiSampleType = multiSampleType;

    this->reload();
}

void RenderTarget::rebuild(int x, int y, int width, int height) {
    this->rebuild(x, y, width, height, this->multiSampleType);
}

void RenderTarget::rebuild(int width, int height) {
    this->rebuild(this->vPos.x, this->vPos.y, width, height, this->multiSampleType);
}

void RenderTarget::rebuild(int width, int height, Graphics::MULTISAMPLE_TYPE multiSampleType) {
    this->rebuild(this->vPos.x, this->vPos.y, width, height, multiSampleType);
}
