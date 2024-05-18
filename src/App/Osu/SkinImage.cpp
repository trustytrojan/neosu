//================ Copyright (c) 2017, PG, All rights reserved. =================//
//
// Purpose:		skin images/drawables
//
// $NoKeywords: $osuskimg
//===============================================================================//

#include "SkinImage.h"

#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"

ConVar osu_skin_animation_fps_override("osu_skin_animation_fps_override", -1.0f, FCVAR_DEFAULT);

ConVar *SkinImage::m_osu_skin_mipmaps_ref = NULL;

SkinImage::SkinImage(Skin *skin, std::string skinElementName, Vector2 baseSizeForScaling2x, float osuSize,
                     std::string animationSeparator, bool ignoreDefaultSkin) {
    m_skin = skin;
    m_vBaseSizeForScaling2x = baseSizeForScaling2x;
    m_fOsuSize = osuSize;

    m_bReady = false;

    if(m_osu_skin_mipmaps_ref == NULL) m_osu_skin_mipmaps_ref = convar->getConVarByName("osu_skin_mipmaps");

    m_iCurMusicPos = 0;
    m_iFrameCounter = 0;
    m_iFrameCounterUnclamped = 0;
    m_fLastFrameTime = 0.0f;
    m_iBeatmapAnimationTimeStartOffset = 0;

    m_bIsMissingTexture = false;
    m_bIsFromDefaultSkin = false;

    m_fDrawClipWidthPercent = 1.0f;

    // logic: first load user skin (true), and if no image could be found then load the default skin (false)
    // this is necessary so that all elements can be correctly overridden with a user skin (e.g. if the user skin only
    // has sliderb.png, but the default skin has sliderb0.png!)
    if(!load(skinElementName, animationSeparator, true)) {
        if(!ignoreDefaultSkin) load(skinElementName, animationSeparator, false);
    }

    // if we couldn't load ANYTHING at all, gracefully fallback to missing texture
    if(m_images.size() < 1) {
        m_bIsMissingTexture = true;

        IMAGE missingTexture;

        missingTexture.img = m_skin->getMissingTexture();
        missingTexture.scale = 2;

        m_images.push_back(missingTexture);
    }

    // if AnimationFramerate is defined in skin, use that. otherwise derive framerate from number of frames
    if(m_skin->getAnimationFramerate() > 0.0f)
        m_fFrameDuration = 1.0f / m_skin->getAnimationFramerate();
    else if(m_images.size() > 0)
        m_fFrameDuration = 1.0f / (float)m_images.size();
}

bool SkinImage::load(std::string skinElementName, std::string animationSeparator, bool ignoreDefaultSkin) {
    std::string animatedSkinElementStartName = skinElementName;
    animatedSkinElementStartName.append(animationSeparator);
    animatedSkinElementStartName.append("0");
    if(loadImage(animatedSkinElementStartName,
                 ignoreDefaultSkin))  // try loading the first animated element (if this exists then we continue loading
                                      // until the first missing frame)
    {
        int frame = 1;
        while(true) {
            std::string currentAnimatedSkinElementFrameName = skinElementName;
            currentAnimatedSkinElementFrameName.append(animationSeparator);
            currentAnimatedSkinElementFrameName.append(std::to_string(frame));

            if(!loadImage(currentAnimatedSkinElementFrameName, ignoreDefaultSkin))
                break;  // stop loading on the first missing frame

            frame++;

            // sanity check
            if(frame > 511) {
                debugLog("SkinImage WARNING: Force stopped loading after 512 frames!\n");
                break;
            }
        }
    } else  // load non-animated skin element
        loadImage(skinElementName, ignoreDefaultSkin);

    return m_images.size() > 0;  // if any image was found
}

bool SkinImage::loadImage(std::string skinElementName, bool ignoreDefaultSkin) {
    std::string filepath1 = m_skin->getFilePath();
    filepath1.append(skinElementName);
    filepath1.append("@2x.png");

    std::string filepath2 = m_skin->getFilePath();
    filepath2.append(skinElementName);
    filepath2.append(".png");

    std::string defaultFilePath1 = "./materials/";
    defaultFilePath1.append(Skin::OSUSKIN_DEFAULT_SKIN_PATH);
    defaultFilePath1.append(skinElementName);
    defaultFilePath1.append("@2x.png");

    std::string defaultFilePath2 = "./materials/";
    defaultFilePath2.append(Skin::OSUSKIN_DEFAULT_SKIN_PATH);
    defaultFilePath2.append(skinElementName);
    defaultFilePath2.append(".png");

    const bool existsFilepath1 = env->fileExists(filepath1);
    const bool existsFilepath2 = env->fileExists(filepath2);
    const bool existsDefaultFilePath1 = env->fileExists(defaultFilePath1);
    const bool existsDefaultFilePath2 = env->fileExists(defaultFilePath2);

    // load user skin

    // check if an @2x version of this image exists
    if(Skin::m_osu_skin_hd->getBool()) {
        // load user skin

        if(existsFilepath1) {
            IMAGE image;

            if(Skin::m_osu_skin_async->getBool()) engine->getResourceManager()->requestNextLoadAsync();

            image.img = engine->getResourceManager()->loadImageAbsUnnamed(filepath1, m_osu_skin_mipmaps_ref->getBool());
            image.scale = 2.0f;

            m_images.push_back(image);

            // export
            {
                m_filepathsForExport.push_back(filepath1);

                if(existsFilepath2) m_filepathsForExport.push_back(filepath2);
            }

            return true;  // nothing more to do here
        }
    }
    // else load the normal version

    // load user skin

    if(existsFilepath2) {
        IMAGE image;

        if(Skin::m_osu_skin_async->getBool()) engine->getResourceManager()->requestNextLoadAsync();

        image.img = engine->getResourceManager()->loadImageAbsUnnamed(filepath2, m_osu_skin_mipmaps_ref->getBool());
        image.scale = 1.0f;

        m_images.push_back(image);

        // export
        {
            m_filepathsForExport.push_back(filepath2);

            if(existsFilepath1) m_filepathsForExport.push_back(filepath1);
        }

        return true;  // nothing more to do here
    }

    if(ignoreDefaultSkin) return false;

    // load default skin

    m_bIsFromDefaultSkin = true;

    // check if an @2x version of this image exists
    if(Skin::m_osu_skin_hd->getBool()) {
        if(existsDefaultFilePath1) {
            IMAGE image;

            if(Skin::m_osu_skin_async->getBool()) engine->getResourceManager()->requestNextLoadAsync();

            image.img =
                engine->getResourceManager()->loadImageAbsUnnamed(defaultFilePath1, m_osu_skin_mipmaps_ref->getBool());
            image.scale = 2.0f;

            m_images.push_back(image);

            // export
            {
                m_filepathsForExport.push_back(defaultFilePath1);

                if(existsDefaultFilePath2) m_filepathsForExport.push_back(defaultFilePath2);
            }

            return true;  // nothing more to do here
        }
    }
    // else load the normal version

    if(existsDefaultFilePath2) {
        IMAGE image;

        if(Skin::m_osu_skin_async->getBool()) engine->getResourceManager()->requestNextLoadAsync();

        image.img =
            engine->getResourceManager()->loadImageAbsUnnamed(defaultFilePath2, m_osu_skin_mipmaps_ref->getBool());
        image.scale = 1.0f;

        m_images.push_back(image);

        // export
        {
            m_filepathsForExport.push_back(defaultFilePath2);

            if(existsDefaultFilePath1) m_filepathsForExport.push_back(defaultFilePath1);
        }

        return true;  // nothing more to do here
    }

    return false;
}

SkinImage::~SkinImage() {
    for(int i = 0; i < m_images.size(); i++) {
        if(m_images[i].img != m_skin->getMissingTexture())
            engine->getResourceManager()->destroyResource(m_images[i].img);
    }
    m_images.clear();

    m_filepathsForExport.clear();
}

void SkinImage::draw(Graphics *g, Vector2 pos, float scale) {
    if(m_images.size() < 1) return;

    scale *= getScale();  // auto scale to current resolution

    g->pushTransform();
    {
        g->scale(scale, scale);
        g->translate(pos.x, pos.y);

        Image *img = getImageForCurrentFrame().img;

        if(m_fDrawClipWidthPercent == 1.0f)
            g->drawImage(img);
        else if(img->isReady()) {
            const float realWidth = img->getWidth();
            const float realHeight = img->getHeight();

            const float width = realWidth * m_fDrawClipWidthPercent;
            const float height = realHeight;

            const float x = -realWidth / 2;
            const float y = -realHeight / 2;

            VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

            vao.addVertex(x, y);
            vao.addTexcoord(0, 0);

            vao.addVertex(x, (y + height));
            vao.addTexcoord(0, 1);

            vao.addVertex((x + width), (y + height));
            vao.addTexcoord(m_fDrawClipWidthPercent, 1);

            vao.addVertex((x + width), y);
            vao.addTexcoord(m_fDrawClipWidthPercent, 0);

            img->bind();
            { g->drawVAO(&vao); }
            img->unbind();
        }
    }
    g->popTransform();
}

void SkinImage::drawRaw(Graphics *g, Vector2 pos, float scale) {
    if(m_images.size() < 1) return;

    g->pushTransform();
    {
        g->scale(scale, scale);
        g->translate(pos.x, pos.y);

        Image *img = getImageForCurrentFrame().img;

        if(m_fDrawClipWidthPercent == 1.0f)
            g->drawImage(img);
        else if(img->isReady()) {
            const float realWidth = img->getWidth();
            const float realHeight = img->getHeight();

            const float width = realWidth * m_fDrawClipWidthPercent;
            const float height = realHeight;

            const float x = -realWidth / 2;
            const float y = -realHeight / 2;

            VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_QUADS);

            vao.addVertex(x, y);
            vao.addTexcoord(0, 0);

            vao.addVertex(x, (y + height));
            vao.addTexcoord(0, 1);

            vao.addVertex((x + width), (y + height));
            vao.addTexcoord(m_fDrawClipWidthPercent, 1);

            vao.addVertex((x + width), y);
            vao.addTexcoord(m_fDrawClipWidthPercent, 0);

            img->bind();
            { g->drawVAO(&vao); }
            img->unbind();
        }
    }
    g->popTransform();
}

void SkinImage::update(float speedMultiplier, bool useEngineTimeForAnimations, long curMusicPos) {
    if(m_images.size() < 1) return;

    m_iCurMusicPos = curMusicPos;

    const float frameDurationInSeconds =
        (osu_skin_animation_fps_override.getFloat() > 0.0f ? (1.0f / osu_skin_animation_fps_override.getFloat())
                                                           : m_fFrameDuration) /
        speedMultiplier;

    if(useEngineTimeForAnimations) {
        // as expected
        if(engine->getTime() >= m_fLastFrameTime) {
            m_fLastFrameTime = engine->getTime() + frameDurationInSeconds;

            m_iFrameCounter++;
            m_iFrameCounterUnclamped++;
            m_iFrameCounter = m_iFrameCounter % m_images.size();
        }
    } else {
        // when playing a beatmap, objects start the animation at frame 0 exactly when they first become visible (this
        // wouldn't work with the engine time method) therefore we need an offset parameter in the same time-space as
        // the beatmap (m_iBeatmapTimeAnimationStartOffset), and we need the beatmap time (curMusicPos) as a relative
        // base m_iBeatmapAnimationTimeStartOffset must be set by all hitobjects live while drawing (e.g. to their
        // m_iTime-m_iObjectTime), since we don't have any animation state saved in the hitobjects!
        m_iFrameCounter = std::max(
            (int)((curMusicPos - m_iBeatmapAnimationTimeStartOffset) / (long)(frameDurationInSeconds * 1000.0f)),
            0);  // freeze animation on frame 0 on negative offsets
        m_iFrameCounterUnclamped = m_iFrameCounter;
        m_iFrameCounter = m_iFrameCounter % m_images.size();  // clamp and wrap around to the number of frames we have
    }
}

void SkinImage::setAnimationTimeOffset(float speedMultiplier, long offset) {
    m_iBeatmapAnimationTimeStartOffset = offset;
    update(speedMultiplier, false, m_iCurMusicPos);  // force update
}

void SkinImage::setAnimationFrameForce(int frame) {
    if(m_images.size() < 1) return;
    m_iFrameCounter = frame % m_images.size();
    m_iFrameCounterUnclamped = m_iFrameCounter;
}

void SkinImage::setAnimationFrameClampUp() {
    if(m_images.size() > 0 && m_iFrameCounterUnclamped > m_images.size() - 1) m_iFrameCounter = m_images.size() - 1;
}

Vector2 SkinImage::getSize() {
    if(m_images.size() > 0)
        return getImageForCurrentFrame().img->getSize() * getScale();
    else
        return getSizeBase();
}

Vector2 SkinImage::getSizeBase() { return m_vBaseSizeForScaling2x * getResolutionScale(); }

Vector2 SkinImage::getSizeBaseRaw() { return m_vBaseSizeForScaling2x * getImageForCurrentFrame().scale; }

Vector2 SkinImage::getImageSizeForCurrentFrame() { return getImageForCurrentFrame().img->getSize(); }

float SkinImage::getScale() { return getImageScale() * getResolutionScale(); }

float SkinImage::getImageScale() {
    if(m_images.size() > 0)
        return m_vBaseSizeForScaling2x.x / getSizeBaseRaw().x;  // allow overscale and underscale
    else
        return 1.0f;
}

float SkinImage::getResolutionScale() { return Osu::getImageScale(m_vBaseSizeForScaling2x, m_fOsuSize); }

bool SkinImage::isReady() {
    if(m_bReady) return true;

    for(int i = 0; i < m_images.size(); i++) {
        if(engine->getResourceManager()->isLoadingResource(m_images[i].img)) return false;
    }

    m_bReady = true;
    return m_bReady;
}

SkinImage::IMAGE SkinImage::getImageForCurrentFrame() {
    if(m_images.size() > 0)
        return m_images[m_iFrameCounter % m_images.size()];
    else {
        IMAGE image;

        image.img = m_skin->getMissingTexture();
        image.scale = 1.0f;

        return image;
    }
}
