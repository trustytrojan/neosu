// Copyright (c) 2013, PG, All rights reserved.
#include "RenderTarget.h"

#include "ConVar.h"
#include "Engine.h"
#include "VertexArrayObject.h"

void RenderTarget::draw(int x, int y) {
    if(!this->bReady) {
        if(cv::debug_rt.getBool()) debugLog("WARNING: RenderTarget is not ready!\n");
        return;
    }

    g->setColor(this->color);

    this->bind();
    {
        // draw flipped for opengl<->engine coordinate mapping
        g->drawQuad(x, y + this->vSize.y, this->vSize.x, -this->vSize.y);
    }
    this->unbind();
}

void RenderTarget::draw(int x, int y, int width, int height) {
    if(!this->bReady) {
        if(cv::debug_rt.getBool()) debugLog("WARNING: RenderTarget is not ready!\n");
        return;
    }

    g->setColor(this->color);

    this->bind();
    {
        // draw flipped for opengl<->engine coordinate mapping
        g->drawQuad(x, y + height, width, -height);
    }
    this->unbind();
}

void RenderTarget::drawRect(int x, int y, int width, int height) {
    if(!this->bReady) {
        if(cv::debug_rt.getBool()) debugLog("WARNING: RenderTarget is not ready!\n");
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
