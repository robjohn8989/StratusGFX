
#ifndef STRATUSGFX_LIGHT_H
#define STRATUSGFX_LIGHT_H

#include "StratusCommon.h"
#include "StratusMath.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include "StratusLog.h"
#include "StratusEngine.h"

namespace stratus {
    class InfiniteLight;
    class Light;

    typedef std::shared_ptr<InfiniteLight> InfiniteLightPtr;
    typedef std::shared_ptr<Light> LightPtr;

    enum class LightType {
        POINTLIGHT,
        SPOTLIGHT
    };

    constexpr float maxLightColor = 10000.0f;
    constexpr float minLightColor = 0.25f;
    constexpr float maxAmbientIntensity = 0.02;
    constexpr float minAmbientIntensity = 0.001;

    // Serves as a global world light
    class InfiniteLight {
        glm::vec3 _color = glm::vec3(1.0f);
        glm::vec3 _position = glm::vec3(0.0f);
        Rotation _rotation;
        // Used to calculate ambient intensity based on sun orientation
        stratus::Radians _rotSine;
        float _intensity = 4.0f;
        float _ambientIntensity = minAmbientIntensity;
        bool _enabled = true;
        bool _runAlphaTest = true;
        // This is the number of rays we march per pixel to determine the final
        // atmospheric value
        int _numAtmosphericSamples = 64;
        float _particleDensity = 0.002f;
        // If > 1, then backscattered light will be greater than forwardscattered light
        float _scatterControl = 0.004f; // 0.004 is roughly a G of 0.7
        glm::vec3 _atmosphereColor = glm::vec3(1.0f);

    public:
        InfiniteLight(const bool enabled = true)
            : _enabled(enabled) {}

        ~InfiniteLight() = default;

        InfiniteLight(const InfiniteLight&) = default;
        InfiniteLight(InfiniteLight&&) = default;
        InfiniteLight& operator=(const InfiniteLight&) = default;
        InfiniteLight& operator=(InfiniteLight&&) = default;

        // Get light color * intensity for use with lighting equations
        glm::vec3 getLuminance() const { return getColor() * getIntensity(); }

        const glm::vec3 & getColor() const { return _color; }
        void setColor(const glm::vec3 & color) { _color = glm::max(color, glm::vec3(0.0f)); }

        const glm::vec3 & getPosition() const { return _position; }
        void setPosition(const glm::vec3 & position) { _position = position; }

        const Rotation & getRotation() const { return _rotation; }
        void setRotation(const Rotation & rotation) { 
            _rotation = rotation;
            _rotSine = stratus::sine(_rotation.x);
        }

        void offsetRotation(const glm::vec3& offsets) {
            Rotation rot = _rotation;
            rot.x += Degrees(offsets.x);
            rot.y += Degrees(offsets.y);
            rot.z += Degrees(offsets.z);
            setRotation(rot);
        }

        float getIntensity() const { 
            // Reduce light intensity as sun goes down
            if (_rotSine.value() < 0.0f) {
                return std::max(minLightColor, _intensity * (1.0f + _rotSine.value()));
            }
            return _intensity; 
        }

        void setIntensity(float intensity) { _intensity = std::max(intensity, 0.0f); }

        float getAmbientIntensity() const { 
            //const float ambient = _rotSine.value() * maxAmbientIntensity;
            //return std::min(maxAmbientIntensity, std::max(ambient, minAmbientIntensity));
            return minAmbientIntensity;
        }

        bool getEnabled() const { return _enabled; }
        void setEnabled(const bool e) { _enabled = e; }

        // Enables alpha testing during cascaded shadow map creation - some scenes don't work
        // as well with this enabled
        void SetAlphaTest(const bool enabled) { _runAlphaTest = enabled; }
        bool GetAlphaTest() const { return _runAlphaTest; }

        // If scatterControl > 1, then backscattered light will be greater than forwardscattered light
        void SetAtmosphericLightingConstants(float particleDensity, float scatterControl) {
            _particleDensity = std::max(0.0f, std::min(particleDensity, 1.0f));
            _scatterControl = std::max(0.0f, scatterControl);
        }

        void SetAtmosphereColor(const glm::vec3& color) {
            _atmosphereColor = color;
        }

        // Number of rays that we march per pixel to determine final atmospheric value
        void SetNumAtmosphericSamplesPerPixel(const int numSamples) {
            _numAtmosphericSamples = numSamples;
        }

        int GetAtmosphericNumSamplesPerPixel() const {
            return _numAtmosphericSamples;
        }

        float GetAtmosphericParticleDensity() const {
            return _particleDensity;
        }

        float GetAtmosphericScatterControl() const {
            return _scatterControl;
        }

        const glm::vec3& GetAtmosphereColor() const {
            return _atmosphereColor;
        }

        virtual InfiniteLightPtr Copy() const {
            return InfiniteLightPtr(new InfiniteLight(*this));
        }
    };

    class Light {
    protected:
        glm::vec3 _position = glm::vec3(0.0f);
        glm::vec3 _color = glm::vec3(1.0f);
        glm::vec3 _baseColor = _color;
        uint64_t _lastFramePositionChanged = 0;
        uint64_t _lastFrameRadiusChanged = 0;
        float _intensity = 200.0f;
        float _radius = 1.0f;
        bool _castsShadows = true;
        // If virtual we intend to use it less as a natural light and more
        // as a way of simulating bounce lighting
        bool _virtualLight = false;
        // If true then we don't want it to be updated when dynamic entities
        // change in the scene (can still cast light, just shadows will not be updated)
        bool _staticLight = false;

        Light(const bool virtualLight, const bool staticLight)
            : _virtualLight(virtualLight), _staticLight(staticLight) {}

    public:
        Light(const bool staticLight) : Light(false, staticLight) {}
        virtual ~Light() = default;

        const glm::vec3& GetPosition() const {
            return _position;
        }

        void SetPosition(const glm::vec3& position) {
            _position = position;
            _lastFramePositionChanged = INSTANCE(Engine)->FrameCount();
        }

        bool PositionChangedWithinLastFrame() const {
            auto diff = INSTANCE(Engine)->FrameCount() - _lastFramePositionChanged;
            return diff <= 1;
        }

        /**
         * @return type of point light so that the renderer knows
         *      how to deal with it
         */
        virtual LightType getType() const = 0;

        const glm::vec3 & getColor() const {
            return _color;
        }

        const glm::vec3& getBaseColor() const {
            return _baseColor;
        }

        /**
         * Sets the color of the light where the scale
         * is not from [0.0, 1.0] but instead can be any
         * number > 0.0 for each color component. To make this
         * work, HDR support is required.
         */
        void setColor(float r, float g, float b) {
            r = std::max(0.0f, r);
            g = std::max(0.0f, g);
            b = std::max(0.0f, b);
            _color = glm::vec3(r, g, b);
            _baseColor = _color;
            _recalcColorWithIntensity();
            //_recalcRadius();
        }

        void setColor(const glm::vec3& color) {
            setColor(color.r, color.g, color.b);
        }

        /**
         * A light's color values can all be on the range of
         * [0.0, 1.0], but the intensity specifies how strong it
         * should be.
         * @param i
         */
        void setIntensity(float i) {
            if (i < 0) return;
            _intensity = i;
            _recalcColorWithIntensity();
            _recalcRadius();
        }

        float getIntensity() const {
            return _intensity;
        }

        // Gets radius but bounded
        virtual float getRadius() const {
            return std::max(150.0f, _radius);
        }

        bool RadiusChangedWithinLastFrame() const {
            auto diff = INSTANCE(Engine)->FrameCount() - _lastFrameRadiusChanged;
            return diff <= 1;

        }

        void setCastsShadows(bool enable) {
            this->_castsShadows = enable;
        }

        bool castsShadows() const {
            return this->_castsShadows;
        }

        // If true then the light will be invisible when the sun is not overhead - 
        // useful for brightening up directly-lit scenes without Static or RT GI
        bool IsVirtualLight() const { return _virtualLight; }
        bool IsStaticLight()  const { return _staticLight; }

        virtual LightPtr Copy() const = 0;

    private:
        // See https://learnopengl.com/Advanced-Lighting/Deferred-Shading for the equation
        void _recalcRadius() {
            static const float lightMin = 256.0 / 5;
            const glm::vec3 intensity = getColor(); // Factors in intensity already
            const float Imax = std::max(intensity.x, std::max(intensity.y, intensity.z));
            //_radius = sqrtf(4.0f * (Imax * lightMin - 1.0f)) / 2.0f;
            _radius = sqrtf(Imax * lightMin - 1.0f) * 2.0f;
            _lastFrameRadiusChanged = INSTANCE(Engine)->FrameCount();
        }

        void _recalcColorWithIntensity() {
            _color = _baseColor * _intensity;
            _color = glm::clamp(_color, glm::vec3(0.0f), glm::vec3(maxLightColor));
            // _color = (_color / maxLightColor) * 30.0f;
        }
    };

    class PointLight : public Light {
        friend class Renderer;
        
        // ShadowMapHandle _shadowHap = -1;

        // These are used to set up the light view matrix
        float lightNearPlane = 0.1f;
        float lightFarPlane = 500.0f;

    protected:
        PointLight(const bool virtualLight, const bool staticLight) 
            : Light(virtualLight, staticLight) {}

    public:
        PointLight(const bool staticLight) : PointLight(false, staticLight) {}

        virtual ~PointLight() = default;

        LightType getType() const override {
            return LightType::POINTLIGHT;
        }

        // ShadowMapHandle getShadowMapHandle() const {
        //     return this->_shadowHap;
        // }

        void setNearFarPlane(float nearPlane, float farPlane) {
            this->lightNearPlane = nearPlane;
            this->lightFarPlane = farPlane;
        }

        float getNearPlane() const {
            return this->lightNearPlane;
        }

        float getFarPlane() const {
            //return this->lightFarPlane;
            return this->getRadius();
        }

        LightPtr Copy() const override {
            return LightPtr(new PointLight(*this));
        }

    private:
        // void _setShadowMapHandle(ShadowMapHandle handle) {
        //     this->_shadowHap = handle;
        // }
    };

    // If you create a VPL and do not set a color for it, it will automatically
    // inherit the color of the sun at each frame. Once a manual color is set this automatic
    // changing will be disabled.
    class VirtualPointLight : public PointLight {
        friend class Renderer;

    public:
        VirtualPointLight() : PointLight(/* virtualLight = */ true, /* staticLight = */ true) {}
        virtual ~VirtualPointLight() = default;

        void SetNumShadowSamples(uint32_t samples) { _numShadowSamples = samples; }
        uint32_t GetNumShadowSamples() const { return _numShadowSamples; }

        // This MUST be done or else the engine makes copies and it will defer
        // to PointLight instead of this and then cause horrible strange errors
        LightPtr Copy() const override {
            return LightPtr(new VirtualPointLight(*this));
        }

        virtual float getRadius() const override {
            return std::max(500.0f, _radius);
        }

    private:
        uint32_t _numShadowSamples = 3;
    };
}

#endif //STRATUSGFX_LIGHT_H
