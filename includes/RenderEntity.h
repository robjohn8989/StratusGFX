//
// Created by stephano on 2/17/19.
//

#ifndef STRATUSGFX_RENDERENTITY_H
#define STRATUSGFX_RENDERENTITY_H

#include "Common.h"

enum class RenderMode {
    ORTHOGRAPHIC,   // 2d - good for menus
    PERSPECTIVE     // 3d
};

enum RenderProperties {
    INVISIBLE = 2,      // material will not be rendered
    FLAT = 4,           // material will not interact with light
    DYNAMIC = 8,        // material fully interacts with all lights
    TEXTURED = 16,       // material has one or more textures
    REFLECTIVE = 32      // material reflects world around it
};

/**
 * @see http://devernay.free.fr/cours/opengl/materials.html
 *
 * A material specifies how light will interact with a surface.
 */
struct RenderMaterial {
    glm::vec3 diffuseColor;
    glm::vec3 specularColor;
    glm::vec3 ambientColor;
    float specularShininess;
    // Not required to have a texture
    TextureHandle texture = -1;
};

class RenderEntity {
    /**
     * The render mode specifies whether we should
     * be dealing with 2d or 3d.
     */
    RenderMode _mode;

    /**
     * This is used by the renderer to decide which shader
     * program to use.
     */
    RenderProperties _properties;

public:
    /**
     * @param mode determines whether 2d/3d is necessary
     * @param properties render properties which decides which
     *      shader to use
     */
    RenderEntity(RenderMode mode, RenderProperties properties);
    virtual ~RenderEntity();

    /**
     * Overrides all current render properties in favor
     * of a new set.
     */
    void setRenderProperties(RenderProperties properties);

    /**
     * Does not override current properties, and instead appends
     * one or more additional properties on top of what is
     * already there.
     */
    void appentRenderProperties(RenderProperties properties);

    RenderMode getRenderMode() const;
    RenderProperties getRenderProperties() const;

    /**
     * This gets called by the renderer when it is time
     * for the object to be drawn.
     */
    virtual void render() = 0;
};

#endif //STRATUSGFX_RENDERENTITY_H
