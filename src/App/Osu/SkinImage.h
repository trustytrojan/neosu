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
    virtual void drawRaw(Graphics *g, Vector2 pos,
                         float scale);  // for objects which scale depending on external factors (e.g. hitobjects,
                                        // depending on the diameter defined by the CS)
    virtual void update(float speedMultiplier, bool useEngineTimeForAnimations = true, long curMusicPos = 0);

    void setAnimationFramerate(float fps) { m_fFrameDuration = 1.0f / std::clamp<float>(fps, 1.0f, 9999.0f); }
    void setAnimationTimeOffset(float speedMultiplier,
                                long offset);  // set this every frame (before drawing) to a fixed point in time
                                               // relative to curMusicPos where we become visible
    void setAnimationFrameForce(
        int frame);  // force set a frame, before drawing (e.g. for hitresults in UIRankingScreenRankingPanel)
    void setAnimationFrameClampUp();  // force stop the animation after the last frame, before drawing

    void setDrawClipWidthPercent(float drawClipWidthPercent) { m_fDrawClipWidthPercent = drawClipWidthPercent; }

    Vector2 getSize();      // absolute size scaled to the current resolution (depending on the osuSize as defined when
                            // loaded in Skin.cpp)
    Vector2 getSizeBase();  // default assumed size scaled to the current resolution. this is the base resolution which
                            // is used for all scaling calculations (to allow skins to overscale or underscale objects)
    Vector2 getSizeBaseRaw();  // default assumed size UNSCALED. that means that e.g. hitcircles will return either
                               // 128x128 or 256x256 depending on the @2x flag in the filename
    inline Vector2 getSizeBaseRawForScaling2x() const { return m_vBaseSizeForScaling2x; }

    Vector2 getImageSizeForCurrentFrame();  // width/height of the actual image texture as loaded from disk
    IMAGE getImageForCurrentFrame();

    float getResolutionScale();

    bool isReady();

    inline int getNumImages() const { return m_images.size(); }
    inline float getFrameDuration() const { return m_fFrameDuration; }
    inline unsigned int getFrameNumber() const { return m_iFrameCounter; }
    inline bool isMissingTexture() const { return m_bIsMissingTexture; }
    inline bool isFromDefaultSkin() const { return m_bIsFromDefaultSkin; }

    inline std::vector<std::string> getFilepathsForExport() const { return m_filepathsForExport; }

   private:
    static ConVar *m_osu_skin_mipmaps_ref;

    bool load(std::string skinElementName, std::string animationSeparator, bool ignoreDefaultSkin);
    bool loadImage(std::string skinElementName, bool ignoreDefaultSkin);

    float getScale();
    float getImageScale();

    Skin *m_skin;
    bool m_bReady;

    // scaling
    Vector2 m_vBaseSizeForScaling2x;
    Vector2 m_vSize;
    float m_fOsuSize;

    // animation
    long m_iCurMusicPos;
    unsigned int m_iFrameCounter;
    unsigned long m_iFrameCounterUnclamped;
    float m_fLastFrameTime;
    float m_fFrameDuration;
    long m_iBeatmapAnimationTimeStartOffset;

    // raw files
    std::vector<IMAGE> m_images;
    bool m_bIsMissingTexture;
    bool m_bIsFromDefaultSkin;

    // custom
    float m_fDrawClipWidthPercent;
    std::vector<std::string> m_filepathsForExport;
};
