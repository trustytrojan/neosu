#pragma once
#include "cbase.h"

class Skin;

class Image;

class SkinImage {
   public:
    struct IMAGE {
        Image *img;
        float scale;
    };

   public:
    SkinImage(Skin *skin, std::string skinElementName, Vector2 baseSizeForScaling2x, float osuSize,
              std::string animationSeparator = "-", bool ignoreDefaultSkin = false);
    virtual ~SkinImage();

    virtual void draw(Graphics *g, Vector2 pos,
                      float scale = 1.0f);  // for objects scaled automatically to the current resolution

    // for objects which scale depending on external factors
    // (e.g. hitobjects, depending on the diameter defined by the CS)
    virtual void drawRaw(Graphics *g, Vector2 pos, float scale, AnchorPoint anchor = AnchorPoint::CENTER);

    virtual void update(float speedMultiplier, bool useEngineTimeForAnimations = true, long curMusicPos = 0);

    void setAnimationFramerate(float fps) { this->fFrameDuration = 1.0f / std::clamp<float>(fps, 1.0f, 9999.0f); }
    void setAnimationTimeOffset(float speedMultiplier,
                                long offset);  // set this every frame (before drawing) to a fixed point in time
                                               // relative to curMusicPos where we become visible
    void setAnimationFrameForce(
        int frame);  // force set a frame, before drawing (e.g. for hitresults in UIRankingScreenRankingPanel)
    void setAnimationFrameClampUp();  // force stop the animation after the last frame, before drawing

    void setDrawClipWidthPercent(float drawClipWidthPercent) { this->fDrawClipWidthPercent = drawClipWidthPercent; }

    Vector2 getSize();      // absolute size scaled to the current resolution (depending on the osuSize as defined when
                            // loaded in Skin.cpp)
    Vector2 getSizeBase();  // default assumed size scaled to the current resolution. this is the base resolution which
                            // is used for all scaling calculations (to allow skins to overscale or underscale objects)
    Vector2 getSizeBaseRaw();  // default assumed size UNSCALED. that means that e.g. hitcircles will return either
                               // 128x128 or 256x256 depending on the @2x flag in the filename
    inline Vector2 getSizeBaseRawForScaling2x() const { return this->vBaseSizeForScaling2x; }

    Vector2 getImageSizeForCurrentFrame();  // width/height of the actual image texture as loaded from disk
    IMAGE getImageForCurrentFrame();

    float getResolutionScale();

    bool isReady();

    inline int getNumImages() const { return this->images.size(); }
    inline float getFrameDuration() const { return this->fFrameDuration; }
    inline unsigned int getFrameNumber() const { return this->iFrameCounter; }
    inline bool isMissingTexture() const { return this->bIsMissingTexture; }
    inline bool isFromDefaultSkin() const { return this->bIsFromDefaultSkin; }

    inline std::vector<std::string> getFilepathsForExport() const { return this->filepathsForExport; }

    bool is_2x;

   private:
    bool load(std::string skinElementName, std::string animationSeparator, bool ignoreDefaultSkin);
    bool loadImage(std::string skinElementName, bool ignoreDefaultSkin);

    float getScale();
    float getImageScale();

    Skin *skin;
    bool bReady;

    // scaling
    Vector2 vBaseSizeForScaling2x;
    Vector2 vSize;
    float fOsuSize;

    // animation
    long iCurMusicPos;
    unsigned int iFrameCounter;
    unsigned long iFrameCounterUnclamped;
    float fLastFrameTime;
    float fFrameDuration;
    long iBeatmapAnimationTimeStartOffset;

    // raw files
    std::vector<IMAGE> images;
    bool bIsMissingTexture;
    bool bIsFromDefaultSkin;

    // custom
    float fDrawClipWidthPercent;
    std::vector<std::string> filepathsForExport;
};
