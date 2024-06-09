#include "ModFPoSu.h"

#include <sstream>

#include "AnimationHandler.h"
#include "BackgroundImageHandler.h"
#include "Beatmap.h"
#include "Camera.h"
#include "ConVar.h"
#include "Engine.h"
#include "Environment.h"
#include "KeyBindings.h"
#include "Keyboard.h"
#include "ModSelector.h"
#include "Mouse.h"
#include "OpenGL3Interface.h"
#include "OpenGLES2Interface.h"
#include "OpenGLHeaders.h"
#include "OpenGLLegacyInterface.h"
#include "OptionsMenu.h"
#include "Osu.h"
#include "ResourceManager.h"
#include "Skin.h"

ConVar osu_mod_fposu("osu_mod_fposu", false, FCVAR_DEFAULT);

ConVar fposu_mouse_dpi("fposu_mouse_dpi", 400, FCVAR_DEFAULT);
ConVar fposu_mouse_cm_360("fposu_mouse_cm_360", 30.0f, FCVAR_DEFAULT);
ConVar fposu_absolute_mode("fposu_absolute_mode", false, FCVAR_DEFAULT);

ConVar fposu_distance("fposu_distance", 0.5f, FCVAR_DEFAULT);
ConVar fposu_playfield_position_x("fposu_playfield_position_x", 0.0f, FCVAR_DEFAULT);
ConVar fposu_playfield_position_y("fposu_playfield_position_y", 0.0f, FCVAR_DEFAULT);
ConVar fposu_playfield_position_z("fposu_playfield_position_z", 0.0f, FCVAR_DEFAULT);
ConVar fposu_playfield_rotation_x("fposu_playfield_rotation_x", 0.0f, FCVAR_DEFAULT);
ConVar fposu_playfield_rotation_y("fposu_playfield_rotation_y", 0.0f, FCVAR_DEFAULT);
ConVar fposu_playfield_rotation_z("fposu_playfield_rotation_z", 0.0f, FCVAR_DEFAULT);
ConVar fposu_fov("fposu_fov", 103.0f, FCVAR_DEFAULT);
ConVar fposu_zoom_fov("fposu_zoom_fov", 45.0f, FCVAR_DEFAULT);
ConVar fposu_zoom_sensitivity_ratio("fposu_zoom_sensitivity_ratio", 1.0f, FCVAR_DEFAULT,
                                    "replicates zoom_sensitivity_ratio behavior on css/csgo/tf2/etc.");
ConVar fposu_zoom_anim_duration("fposu_zoom_anim_duration", 0.065f, FCVAR_DEFAULT,
                                "time in seconds for the zoom/unzoom animation");
ConVar fposu_zoom_toggle("fposu_zoom_toggle", false, FCVAR_DEFAULT, "whether the zoom key acts as a toggle");
ConVar fposu_vertical_fov("fposu_vertical_fov", false, FCVAR_DEFAULT);
ConVar fposu_curved("fposu_curved", true, FCVAR_DEFAULT);
ConVar fposu_skybox("fposu_skybox", true, FCVAR_DEFAULT);
ConVar fposu_cube("fposu_cube", true, FCVAR_DEFAULT);
ConVar fposu_cube_size("fposu_cube_size", 500.0f, FCVAR_DEFAULT);
ConVar fposu_cube_tint_r("fposu_cube_tint_r", 255, FCVAR_DEFAULT, "from 0 to 255");
ConVar fposu_cube_tint_g("fposu_cube_tint_g", 255, FCVAR_DEFAULT, "from 0 to 255");
ConVar fposu_cube_tint_b("fposu_cube_tint_b", 255, FCVAR_DEFAULT, "from 0 to 255");
ConVar fposu_invert_vertical("fposu_invert_vertical", false, FCVAR_DEFAULT);
ConVar fposu_invert_horizontal("fposu_invert_horizontal", false, FCVAR_DEFAULT);

ConVar fposu_noclip("fposu_noclip", false, FCVAR_DEFAULT);
ConVar fposu_noclipspeed("fposu_noclipspeed", 2.0f, FCVAR_DEFAULT);
ConVar fposu_noclipaccelerate("fposu_noclipaccelerate", 20.0f, FCVAR_DEFAULT);
ConVar fposu_noclipfriction("fposu_noclipfriction", 10.0f, FCVAR_DEFAULT);

ConVar fposu_draw_cursor_trail("fposu_draw_cursor_trail", true, FCVAR_DEFAULT);
ConVar fposu_draw_scorebarbg_on_top("fposu_draw_scorebarbg_on_top", false, FCVAR_DEFAULT);
ConVar fposu_transparent_playfield("fposu_transparent_playfield", false, FCVAR_DEFAULT,
                                   "only works if background dim is 100% and background brightness is 0%");

ConVar fposu_mod_strafing("fposu_mod_strafing", false, FCVAR_UNLOCKED);
ConVar fposu_mod_strafing_strength_x("fposu_mod_strafing_strength_x", 0.3f, FCVAR_DEFAULT);
ConVar fposu_mod_strafing_frequency_x("fposu_mod_strafing_frequency_x", 0.1f, FCVAR_DEFAULT);
ConVar fposu_mod_strafing_strength_y("fposu_mod_strafing_strength_y", 0.1f, FCVAR_DEFAULT);
ConVar fposu_mod_strafing_frequency_y("fposu_mod_strafing_frequency_y", 0.2f, FCVAR_DEFAULT);
ConVar fposu_mod_strafing_strength_z("fposu_mod_strafing_strength_z", 0.15f, FCVAR_DEFAULT);
ConVar fposu_mod_strafing_frequency_z("fposu_mod_strafing_frequency_z", 0.15f, FCVAR_DEFAULT);

ConVar fposu_mod_3d_depthwobble("fposu_mod_3d_depthwobble", false, FCVAR_UNLOCKED);

constexpr const float ModFPoSu::SIZEDIV3D;
constexpr const int ModFPoSu::SUBDIVISIONS;

ModFPoSu::ModFPoSu() {
    // convar refs
    m_mouse_sensitivity_ref = convar->getConVarByName("mouse_sensitivity");
    m_osu_draw_beatmap_background_image_ref = convar->getConVarByName("osu_draw_beatmap_background_image");
    m_osu_background_dim_ref = convar->getConVarByName("osu_background_dim");

    // vars
    m_fCircumLength = 0.0f;
    m_camera = new Camera(Vector3(0, 0, 0), Vector3(0, 0, -1));
    m_bKeyLeftDown = false;
    m_bKeyUpDown = false;
    m_bKeyRightDown = false;
    m_bKeyDownDown = false;
    m_bKeySpaceDown = false;
    m_bKeySpaceUpDown = false;
    m_bZoomKeyDown = false;
    m_bZoomed = false;
    m_fZoomFOVAnimPercent = 0.0f;

    m_fEdgeDistance = 0.0f;
    m_bCrosshairIntersectsScreen = false;

    // load resources
    m_vao = engine->getResourceManager()->createVertexArrayObject();
    m_vaoCube = engine->getResourceManager()->createVertexArrayObject();

    m_hitcircleShader = NULL;

    // convar callbacks
    fposu_curved.setCallback(fastdelegate::MakeDelegate(this, &ModFPoSu::onCurvedChange));
    fposu_distance.setCallback(fastdelegate::MakeDelegate(this, &ModFPoSu::onDistanceChange));
    fposu_noclip.setCallback(fastdelegate::MakeDelegate(this, &ModFPoSu::onNoclipChange));

    // init
    makePlayfield();
    makeBackgroundCube();
}

ModFPoSu::~ModFPoSu() { anim->deleteExistingAnimation(&m_fZoomFOVAnimPercent); }

void ModFPoSu::draw(Graphics *g) {
    if(!osu_mod_fposu.getBool()) return;

    const float fov = lerp<float>(fposu_fov.getFloat(), fposu_zoom_fov.getFloat(), m_fZoomFOVAnimPercent);
    Matrix4 projectionMatrix =
        fposu_vertical_fov.getBool()
            ? Camera::buildMatrixPerspectiveFovVertical(
                  deg2rad(fov), ((float)osu->getScreenWidth() / (float)osu->getScreenHeight()), 0.05f, 1000.0f)
            : Camera::buildMatrixPerspectiveFovHorizontal(
                  deg2rad(fov), ((float)osu->getScreenHeight() / (float)osu->getScreenWidth()), 0.05f, 1000.0f);
    Matrix4 viewMatrix = Camera::buildMatrixLookAt(
        m_camera->getPos(), m_camera->getPos() + m_camera->getViewDirection(), m_camera->getViewUp());

    // HACKHACK: there is currently no way to directly modify the viewport origin, so the only option for rendering
    // non-2d stuff with correct offsets (i.e. top left) is by rendering into a rendertarget HACKHACK: abusing
    // sliderFrameBuffer

    osu->getSliderFrameBuffer()->enable();
    {
        const Vector2 resolutionBackup = g->getResolution();
        g->onResolutionChange(
            osu->getSliderFrameBuffer()
                ->getSize());  // set renderer resolution to game resolution (to correctly support letterboxing etc.)
        {
            g->clearDepthBuffer();
            g->pushTransform();
            {
                g->setWorldMatrix(viewMatrix);
                g->setProjectionMatrix(projectionMatrix);

                g->setBlending(false);
                {
                    // regular fposu "2d" render path

                    g->setDepthBuffer(true);
                    {
                        // axis lines at (0, 0, 0)
                        if(fposu_noclip.getBool()) {
                            static VertexArrayObject vao(Graphics::PRIMITIVE::PRIMITIVE_LINES);
                            vao.empty();
                            {
                                Vector3 pos = Vector3(0, 0, 0);
                                float length = 1.0f;

                                vao.addColor(0xffff0000);
                                vao.addVertex(pos.x, pos.y, pos.z);
                                vao.addColor(0xffff0000);
                                vao.addVertex(pos.x + length, pos.y, pos.z);

                                vao.addColor(0xff00ff00);
                                vao.addVertex(pos.x, pos.y, pos.z);
                                vao.addColor(0xff00ff00);
                                vao.addVertex(pos.x, pos.y + length, pos.z);

                                vao.addColor(0xff0000ff);
                                vao.addVertex(pos.x, pos.y, pos.z);
                                vao.addColor(0xff0000ff);
                                vao.addVertex(pos.x, pos.y, pos.z + length);
                            }
                            g->setColor(0xffffffff);
                            g->drawVAO(&vao);
                        }

                        // cube
                        if(fposu_cube.getBool()) {
                            osu->getSkin()->getBackgroundCube()->bind();
                            {
                                g->setColor(COLOR(255, clamp<int>(fposu_cube_tint_r.getInt(), 0, 255),
                                                  clamp<int>(fposu_cube_tint_g.getInt(), 0, 255),
                                                  clamp<int>(fposu_cube_tint_b.getInt(), 0, 255)));
                                g->drawVAO(m_vaoCube);
                            }
                            osu->getSkin()->getBackgroundCube()->unbind();
                        }
                    }
                    g->setDepthBuffer(false);

                    if(fposu_transparent_playfield.getBool()) g->setBlending(true);

                    Matrix4 worldMatrix = m_modelMatrix;
                    g->setWorldMatrixMul(worldMatrix);
                    {
                        osu->getPlayfieldBuffer()->bind();
                        {
                            g->setColor(0xffffffff);
                            g->drawVAO(m_vao);
                        }
                        osu->getPlayfieldBuffer()->unbind();
                    }

                    // (no setBlending(false), since we are already at the end)
                }
                if(!fposu_transparent_playfield.getBool()) g->setBlending(true);
            }
            g->popTransform();
        }
        g->onResolutionChange(resolutionBackup);
    }
    osu->getSliderFrameBuffer()->disable();

    // finally, draw that to the screen
    g->setBlending(false);
    { osu->getSliderFrameBuffer()->draw(g, 0, 0); }
    g->setBlending(true);
}

void ModFPoSu::update() {
    if(!osu_mod_fposu.getBool()) return;

    if(fposu_noclip.getBool()) noclipMove();

    m_modelMatrix = Matrix4();
    {
        m_modelMatrix.scale(
            1.0f, (osu->getPlayfieldBuffer()->getHeight() / osu->getPlayfieldBuffer()->getWidth()) * (m_fCircumLength),
            1.0f);

        // rotate around center
        {
            m_modelMatrix.translate(0, 0, fposu_distance.getFloat());  // (compensate for mesh offset)
            {
                m_modelMatrix.rotateX(fposu_playfield_rotation_x.getFloat());
                m_modelMatrix.rotateY(fposu_playfield_rotation_y.getFloat());
                m_modelMatrix.rotateZ(fposu_playfield_rotation_z.getFloat());
            }
            m_modelMatrix.translate(0, 0, -fposu_distance.getFloat());  // (restore)
        }

        m_modelMatrix.translate(
            fposu_playfield_position_x.getFloat(), fposu_playfield_position_y.getFloat(),
            -0.0015f + fposu_playfield_position_z
                           .getFloat());  // NOTE: slightly move back by default to avoid aliasing with background cube

        if(fposu_mod_strafing.getBool()) {
            if(osu->isInPlayMode()) {
                const long curMusicPos = osu->getSelectedBeatmap()->getCurMusicPos();

                const float speedMultiplierCompensation = 1.0f / osu->getSelectedBeatmap()->getSpeedMultiplier();

                const float x = std::sin((curMusicPos / 1000.0f) * 5 * speedMultiplierCompensation *
                                         fposu_mod_strafing_frequency_x.getFloat()) *
                                fposu_mod_strafing_strength_x.getFloat();
                const float y = std::sin((curMusicPos / 1000.0f) * 5 * speedMultiplierCompensation *
                                         fposu_mod_strafing_frequency_y.getFloat()) *
                                fposu_mod_strafing_strength_y.getFloat();
                const float z = std::sin((curMusicPos / 1000.0f) * 5 * speedMultiplierCompensation *
                                         fposu_mod_strafing_frequency_z.getFloat()) *
                                fposu_mod_strafing_strength_z.getFloat();

                m_modelMatrix.translate(x, y, z);
            }
        }
    }

    const bool isAutoCursor = (osu->getModAuto() || osu->getModAutopilot() || osu->getSelectedBeatmap()->is_watching ||
                               osu->getSelectedBeatmap()->is_spectating);

    m_bCrosshairIntersectsScreen = true;
    if(!fposu_absolute_mode.getBool() && !isAutoCursor &&
       env->getOS() == Environment::OS::WINDOWS)  // HACKHACK: windows only for now (raw input support)
    {
        // regular mouse position mode

        // calculate mouse delta
        Vector2 rawDelta = engine->getMouse()->getRawDelta() /
                           m_mouse_sensitivity_ref->getFloat();  // HACKHACK: undo engine mouse sensitivity multiplier

        // apply fposu mouse sensitivity multiplier
        const double countsPerCm = (double)fposu_mouse_dpi.getInt() / 2.54;
        const double cmPer360 = fposu_mouse_cm_360.getFloat();
        const double countsPer360 = cmPer360 * countsPerCm;
        const double multiplier = 360.0 / countsPer360;
        rawDelta *= multiplier;

        // apply zoom_sensitivity_ratio if zoomed
        if(m_bZoomed && fposu_zoom_sensitivity_ratio.getFloat() > 0.0f)
            rawDelta *=
                (fposu_zoom_fov.getFloat() / fposu_fov.getFloat()) *
                fposu_zoom_sensitivity_ratio
                    .getFloat();  // see
                                  // https://www.reddit.com/r/GlobalOffensive/comments/3vxkav/how_zoomed_sensitivity_works/

        // update camera
        if(rawDelta.x != 0.0f) m_camera->rotateY(rawDelta.x * (fposu_invert_horizontal.getBool() ? 1.0f : -1.0f));
        if(rawDelta.y != 0.0f) m_camera->rotateX(rawDelta.y * (fposu_invert_vertical.getBool() ? 1.0f : -1.0f));

        // calculate ray-mesh intersection and set new mouse pos
        Vector2 newMousePos = intersectRayMesh(m_camera->getPos(), m_camera->getViewDirection());

        const bool osCursorVisible = (env->isCursorVisible() || !env->isCursorInWindow() || !engine->hasFocus());

        if(!osCursorVisible) {
            // special case: force to center of screen if no intersection
            if(newMousePos.x == 0.0f && newMousePos.y == 0.0f) {
                m_bCrosshairIntersectsScreen = false;
                newMousePos = osu->getScreenSize() / 2;
            }

            setMousePosCompensated(newMousePos);
        }
    } else {
        // absolute mouse position mode (or auto)
        Vector2 mousePos = engine->getMouse()->getPos();
        auto beatmap = osu->getSelectedBeatmap();
        if(isAutoCursor && !beatmap->isPaused()) {
            mousePos = beatmap->getCursorPos();
        }

        m_bCrosshairIntersectsScreen = true;
        m_camera->lookAt(calculateUnProjectedVector(mousePos));
    }
}

void ModFPoSu::noclipMove() {
    const float noclipSpeed = fposu_noclipspeed.getFloat() * (engine->getKeyboard()->isShiftDown() ? 3.0f : 1.0f) *
                              (engine->getKeyboard()->isControlDown() ? 0.2f : 1);
    const float noclipAccelerate = fposu_noclipaccelerate.getFloat();
    const float friction = fposu_noclipfriction.getFloat();

    // build direction vector based on player key inputs
    Vector3 wishdir;
    {
        wishdir += (m_bKeyUpDown ? m_camera->getViewDirection() : Vector3());
        wishdir -= (m_bKeyDownDown ? m_camera->getViewDirection() : Vector3());
        wishdir += (m_bKeyLeftDown ? m_camera->getViewRight() : Vector3());
        wishdir -= (m_bKeyRightDown ? m_camera->getViewRight() : Vector3());
        wishdir += (m_bKeySpaceDown ? (m_bKeySpaceUpDown ? Vector3(0.0f, 1.0f, 0.0f) : Vector3(0.0f, -1.0f, 0.0f))
                                    : Vector3());
    }

    // normalize
    float wishspeed = 0.0f;
    {
        const float length = wishdir.length();
        if(length > 0.0f) {
            wishdir /= length;  // normalize
            wishspeed = noclipSpeed;
        }
    }

    // friction (deccelerate)
    {
        const float spd = m_vVelocity.length();
        if(spd > 0.00000001f) {
            // only apply friction once we "stop" moving (special case for noclip mode)
            if(wishspeed == 0.0f) {
                const float drop = spd * friction * engine->getFrameTime();

                float newSpeed = spd - drop;
                {
                    if(newSpeed < 0.0f) newSpeed = 0.0f;
                }
                newSpeed /= spd;

                m_vVelocity *= newSpeed;
            }
        } else
            m_vVelocity.zero();
    }

    // accelerate
    {
        float addspeed = wishspeed;
        if(addspeed > 0.0f) {
            float accelspeed = noclipAccelerate * engine->getFrameTime() * wishspeed;

            if(accelspeed > addspeed) accelspeed = addspeed;

            m_vVelocity += accelspeed * wishdir;
        }
    }

    // clamp to max speed
    if(m_vVelocity.length() > noclipSpeed) m_vVelocity.setLength(noclipSpeed);

    // move
    m_camera->setPos(m_camera->getPos() + m_vVelocity * engine->getFrameTime());
}

void ModFPoSu::onKeyDown(KeyboardEvent &key) {
    if(key == (KEYCODE)KeyBindings::FPOSU_ZOOM.getInt() && !m_bZoomKeyDown) {
        m_bZoomKeyDown = true;

        if(!m_bZoomed || fposu_zoom_toggle.getBool()) {
            if(!fposu_zoom_toggle.getBool())
                m_bZoomed = true;
            else
                m_bZoomed = !m_bZoomed;

            handleZoomedChange();
        }
    }

    if(key == KEY_A) m_bKeyLeftDown = true;
    if(key == KEY_W) m_bKeyUpDown = true;
    if(key == KEY_D) m_bKeyRightDown = true;
    if(key == KEY_S) m_bKeyDownDown = true;
    if(key == KEY_SPACE) {
        if(!m_bKeySpaceDown) m_bKeySpaceUpDown = !m_bKeySpaceUpDown;

        m_bKeySpaceDown = true;
    }
}

void ModFPoSu::onKeyUp(KeyboardEvent &key) {
    if(key == (KEYCODE)KeyBindings::FPOSU_ZOOM.getInt()) {
        m_bZoomKeyDown = false;

        if(m_bZoomed && !fposu_zoom_toggle.getBool()) {
            m_bZoomed = false;
            handleZoomedChange();
        }
    }

    if(key == KEY_A) m_bKeyLeftDown = false;
    if(key == KEY_W) m_bKeyUpDown = false;
    if(key == KEY_D) m_bKeyRightDown = false;
    if(key == KEY_S) m_bKeyDownDown = false;
    if(key == KEY_SPACE) m_bKeySpaceDown = false;
}

void ModFPoSu::handleZoomedChange() {
    if(m_bZoomed)
        anim->moveQuadOut(&m_fZoomFOVAnimPercent, 1.0f,
                          (1.0f - m_fZoomFOVAnimPercent) * fposu_zoom_anim_duration.getFloat(), true);
    else
        anim->moveQuadOut(&m_fZoomFOVAnimPercent, 0.0f, m_fZoomFOVAnimPercent * fposu_zoom_anim_duration.getFloat(),
                          true);
}

void ModFPoSu::setMousePosCompensated(Vector2 newMousePos) {
    // NOTE: letterboxing uses Mouse::setOffset() to offset the virtual engine cursor coordinate system, so we have to
    // respect that when setting a new (absolute) position
    newMousePos -= engine->getMouse()->getOffset();

    engine->getMouse()->onPosChange(newMousePos);
    env->setMousePos(newMousePos.x, newMousePos.y);
}

Vector2 ModFPoSu::intersectRayMesh(Vector3 pos, Vector3 dir) {
    std::list<VertexPair>::iterator begin = m_meshList.begin();
    std::list<VertexPair>::iterator next = ++m_meshList.begin();
    int face = 0;
    while(next != m_meshList.end()) {
        const Vector4 topLeft = (m_modelMatrix * Vector4((*begin).a.x, (*begin).a.y, (*begin).a.z, 1.0f));
        const Vector4 right = (m_modelMatrix * Vector4((*next).a.x, (*next).a.y, (*next).a.z, 1.0f));
        const Vector4 down = (m_modelMatrix * Vector4((*begin).b.x, (*begin).b.y, (*begin).b.z, 1.0f));
        // const Vector3 normal = (modelMatrix * (*begin).normal).normalize();

        const Vector3 TopLeft = Vector3(topLeft.x, topLeft.y, topLeft.z);
        const Vector3 Right = Vector3(right.x, right.y, right.z);
        const Vector3 Down = Vector3(down.x, down.y, down.z);

        const Vector3 calculatedNormal = (Right - TopLeft).cross(Down - TopLeft);

        const float denominator = calculatedNormal.dot(dir);
        const float numerator = -calculatedNormal.dot(pos - TopLeft);

        // WARNING: this is a full line trace (i.e. backwards and forwards infinitely far)
        if(denominator == 0.0f) {
            begin++;
            next++;
            face++;
            continue;
        }

        const float t = numerator / denominator;
        const Vector3 intersectionPoint = pos + dir * t;

        if(std::abs(calculatedNormal.dot(intersectionPoint - TopLeft)) < 1e-6f) {
            const float u = (intersectionPoint - TopLeft).dot(Right - TopLeft);
            const float v = (intersectionPoint - TopLeft).dot(Down - TopLeft);

            if(u >= 0 && u <= (Right - TopLeft).dot(Right - TopLeft)) {
                if(v >= 0 && v <= (Down - TopLeft).dot(Down - TopLeft)) {
                    if(denominator > 0.0f)  // only allow forwards trace
                    {
                        const float rightLength = (Right - TopLeft).length();
                        const float downLength = (Down - TopLeft).length();
                        const float x = u / (rightLength * rightLength);
                        const float y = v / (downLength * downLength);
                        const float distancePerFace =
                            (float)osu->getScreenWidth() / pow(2.0f, (float)SUBDIVISIONS);
                        const float distanceInFace = distancePerFace * x;

                        const Vector2 newMousePos =
                            Vector2((distancePerFace * face) + distanceInFace, y * osu->getScreenHeight());

                        return newMousePos;
                    }
                }
            }
        }

        begin++;
        next++;
        face++;
    }

    return Vector2(0, 0);
}

Vector3 ModFPoSu::calculateUnProjectedVector(Vector2 pos) {
    // calculate 3d position of 2d cursor on screen mesh
    const float cursorXPercent = clamp<float>(pos.x / (float)osu->getScreenWidth(), 0.0f, 1.0f);
    const float cursorYPercent = clamp<float>(pos.y / (float)osu->getScreenHeight(), 0.0f, 1.0f);

    std::list<VertexPair>::iterator begin = m_meshList.begin();
    std::list<VertexPair>::iterator next = ++m_meshList.begin();
    while(next != m_meshList.end()) {
        Vector3 topLeft = (*begin).a;
        Vector3 bottomLeft = (*begin).b;
        Vector3 topRight = (*next).a;
        // Vector3 bottomRight = (*next).b;

        const float leftTC = (*begin).textureCoordinate;
        const float rightTC = (*next).textureCoordinate;
        const float topTC = 1.0f;
        const float bottomTC = 0.0f;

        if(cursorXPercent >= leftTC && cursorXPercent <= rightTC && cursorYPercent >= bottomTC &&
           cursorYPercent <= topTC) {
            const float tcRightPercent = (cursorXPercent - leftTC) / std::abs(leftTC - rightTC);
            Vector3 right = (topRight - topLeft);
            right.setLength(right.length() * tcRightPercent);

            const float tcDownPercent = (cursorYPercent - bottomTC) / std::abs(topTC - bottomTC);
            Vector3 down = (bottomLeft - topLeft);
            down.setLength(down.length() * tcDownPercent);

            const Vector3 modelPos = (topLeft + right + down);

            const Vector4 worldPos = m_modelMatrix * Vector4(modelPos.x, modelPos.y, modelPos.z, 1.0f);

            return Vector3(worldPos.x, worldPos.y, worldPos.z);
        }

        begin++;
        next++;
    }

    return Vector3(-0.5f, 0.5f, -0.5f);
}

void ModFPoSu::makePlayfield() {
    m_vao->clear();
    m_meshList.clear();

    const float topTC = 1.0f;
    const float bottomTC = 0.0f;
    const float dist = -fposu_distance.getFloat();

    VertexPair vp1 = VertexPair(Vector3(-0.5, 0.5, dist), Vector3(-0.5, -0.5, dist), 0);
    VertexPair vp2 = VertexPair(Vector3(0.5, 0.5, dist), Vector3(0.5, -0.5, dist), 1);

    m_fEdgeDistance = Vector3(0, 0, 0).distance(Vector3(-0.5, 0.0, dist));

    m_meshList.push_back(vp1);
    m_meshList.push_back(vp2);

    std::list<VertexPair>::iterator begin = m_meshList.begin();
    std::list<VertexPair>::iterator end = m_meshList.end();
    --end;
    m_fCircumLength = subdivide(m_meshList, begin, end, SUBDIVISIONS, m_fEdgeDistance);

    begin = m_meshList.begin();
    std::list<VertexPair>::iterator next = ++m_meshList.begin();
    while(next != m_meshList.end()) {
        Vector3 topLeft = (*begin).a;
        Vector3 bottomLeft = (*begin).b;
        Vector3 topRight = (*next).a;
        Vector3 bottomRight = (*next).b;

        const float leftTC = (*begin).textureCoordinate;
        const float rightTC = (*next).textureCoordinate;

        m_vao->addVertex(topLeft);
        m_vao->addTexcoord(leftTC, topTC);
        m_vao->addVertex(topRight);
        m_vao->addTexcoord(rightTC, topTC);
        m_vao->addVertex(bottomLeft);
        m_vao->addTexcoord(leftTC, bottomTC);

        m_vao->addVertex(bottomLeft);
        m_vao->addTexcoord(leftTC, bottomTC);
        m_vao->addVertex(topRight);
        m_vao->addTexcoord(rightTC, topTC);
        m_vao->addVertex(bottomRight);
        m_vao->addTexcoord(rightTC, bottomTC);

        (*begin).normal = normalFromTriangle(topLeft, topRight, bottomLeft);

        begin++;
        next++;
    }
}

void ModFPoSu::makeBackgroundCube() {
    m_vaoCube->clear();

    const float size = fposu_cube_size.getFloat();

    // front
    m_vaoCube->addVertex(-size, -size, -size);
    m_vaoCube->addTexcoord(0.0f, 1.0f);
    m_vaoCube->addVertex(size, -size, -size);
    m_vaoCube->addTexcoord(1.0f, 1.0f);
    m_vaoCube->addVertex(size, size, -size);
    m_vaoCube->addTexcoord(1.0f, 0.0f);

    m_vaoCube->addVertex(size, size, -size);
    m_vaoCube->addTexcoord(1.0f, 0.0f);
    m_vaoCube->addVertex(-size, size, -size);
    m_vaoCube->addTexcoord(0.0f, 0.0f);
    m_vaoCube->addVertex(-size, -size, -size);
    m_vaoCube->addTexcoord(0.0f, 1.0f);

    // back
    m_vaoCube->addVertex(-size, -size, size);
    m_vaoCube->addTexcoord(1.0f, 1.0f);
    m_vaoCube->addVertex(size, -size, size);
    m_vaoCube->addTexcoord(0.0f, 1.0f);
    m_vaoCube->addVertex(size, size, size);
    m_vaoCube->addTexcoord(0.0f, 0.0f);

    m_vaoCube->addVertex(size, size, size);
    m_vaoCube->addTexcoord(0.0f, 0.0f);
    m_vaoCube->addVertex(-size, size, size);
    m_vaoCube->addTexcoord(1.0f, 0.0f);
    m_vaoCube->addVertex(-size, -size, size);
    m_vaoCube->addTexcoord(1.0f, 1.0f);

    // left
    m_vaoCube->addVertex(-size, size, size);
    m_vaoCube->addTexcoord(0.0f, 0.0f);
    m_vaoCube->addVertex(-size, size, -size);
    m_vaoCube->addTexcoord(1.0f, 0.0f);
    m_vaoCube->addVertex(-size, -size, -size);
    m_vaoCube->addTexcoord(1.0f, 1.0f);

    m_vaoCube->addVertex(-size, -size, -size);
    m_vaoCube->addTexcoord(1.0f, 1.0f);
    m_vaoCube->addVertex(-size, -size, size);
    m_vaoCube->addTexcoord(0.0f, 1.0f);
    m_vaoCube->addVertex(-size, size, size);
    m_vaoCube->addTexcoord(0.0f, 0.0f);

    // right
    m_vaoCube->addVertex(size, size, size);
    m_vaoCube->addTexcoord(1.0f, 0.0f);
    m_vaoCube->addVertex(size, size, -size);
    m_vaoCube->addTexcoord(0.0f, 0.0f);
    m_vaoCube->addVertex(size, -size, -size);
    m_vaoCube->addTexcoord(0.0f, 1.0f);

    m_vaoCube->addVertex(size, -size, -size);
    m_vaoCube->addTexcoord(0.0f, 1.0f);
    m_vaoCube->addVertex(size, -size, size);
    m_vaoCube->addTexcoord(1.0f, 1.0f);
    m_vaoCube->addVertex(size, size, size);
    m_vaoCube->addTexcoord(1.0f, 0.0f);

    // bottom
    m_vaoCube->addVertex(-size, -size, -size);
    m_vaoCube->addTexcoord(0.0f, 0.0f);
    m_vaoCube->addVertex(size, -size, -size);
    m_vaoCube->addTexcoord(1.0f, 0.0f);
    m_vaoCube->addVertex(size, -size, size);
    m_vaoCube->addTexcoord(1.0f, 1.0f);

    m_vaoCube->addVertex(size, -size, size);
    m_vaoCube->addTexcoord(1.0f, 1.0f);
    m_vaoCube->addVertex(-size, -size, size);
    m_vaoCube->addTexcoord(0.0f, 1.0f);
    m_vaoCube->addVertex(-size, -size, -size);
    m_vaoCube->addTexcoord(0.0f, 0.0f);

    // top
    m_vaoCube->addVertex(-size, size, -size);
    m_vaoCube->addTexcoord(0.0f, 1.0f);
    m_vaoCube->addVertex(size, size, -size);
    m_vaoCube->addTexcoord(1.0f, 1.0f);
    m_vaoCube->addVertex(size, size, size);
    m_vaoCube->addTexcoord(1.0f, 0.0f);

    m_vaoCube->addVertex(size, size, size);
    m_vaoCube->addTexcoord(1.0f, 0.0f);
    m_vaoCube->addVertex(-size, size, size);
    m_vaoCube->addTexcoord(0.0f, 0.0f);
    m_vaoCube->addVertex(-size, size, -size);
    m_vaoCube->addTexcoord(0.0f, 1.0f);
}

void ModFPoSu::onCurvedChange(UString oldValue, UString newValue) { makePlayfield(); }

void ModFPoSu::onDistanceChange(UString oldValue, UString newValue) { makePlayfield(); }

void ModFPoSu::onNoclipChange(UString oldValue, UString newValue) {
    if(fposu_noclip.getBool())
        m_camera->setPos(m_vPrevNoclipCameraPos);
    else {
        m_vPrevNoclipCameraPos = m_camera->getPos();
        m_camera->setPos(Vector3(0, 0, 0));
    }
}

float ModFPoSu::subdivide(std::list<VertexPair> &meshList, const std::list<VertexPair>::iterator &begin,
                          const std::list<VertexPair>::iterator &end, int n, float edgeDistance) {
    const Vector3 a = Vector3((*begin).a.x, 0.0f, (*begin).a.z);
    const Vector3 b = Vector3((*end).a.x, 0.0f, (*end).a.z);
    Vector3 middlePoint =
        Vector3(lerp<float>(a.x, b.x, 0.5f), lerp<float>(a.y, b.y, 0.5f), lerp<float>(a.z, b.z, 0.5f));

    if(fposu_curved.getBool()) middlePoint.setLength(edgeDistance);

    Vector3 top, bottom;
    top = bottom = middlePoint;

    top.y = (*begin).a.y;
    bottom.y = (*begin).b.y;

    const float tc = lerp<float>((*begin).textureCoordinate, (*end).textureCoordinate, 0.5f);

    VertexPair newVP = VertexPair(top, bottom, tc);
    const std::list<VertexPair>::iterator newPos = meshList.insert(end, newVP);

    float circumLength = 0.0f;

    if(n > 1) {
        circumLength += subdivide(meshList, begin, newPos, n - 1, edgeDistance);
        circumLength += subdivide(meshList, newPos, end, n - 1, edgeDistance);
    } else {
        circumLength += (*begin).a.distance(newVP.a);
        circumLength += newVP.a.distance((*end).a);
    }

    return circumLength;
}

Vector3 ModFPoSu::normalFromTriangle(Vector3 p1, Vector3 p2, Vector3 p3) {
    const Vector3 u = (p2 - p1);
    const Vector3 v = (p3 - p1);

    return u.cross(v).normalize();
}
