#pragma once

#include <memory>
#include "StratusCommon.h"
#include "StratusThread.h"
#include "StratusEntity.h"
#include "StratusTexture.h"
#include <vector>
#include <shared_mutex>
#include <unordered_map>

namespace stratus {
    class ResourceManager {
        friend class Engine;

        ResourceManager();

    public:
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager(ResourceManager&&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;
        ResourceManager& operator=(ResourceManager&&) = delete;

        ~ResourceManager();

        static ResourceManager * Instance() { return _instance; }

        void Update();
        Async<Entity> LoadModel(const std::string&);
        TextureHandle LoadTexture(const std::string&);
        bool GetTexture(const TextureHandle, Async<Texture>&) const;

    private:
        std::unique_lock<std::shared_mutex> _LockWrite() const { return std::unique_lock<std::shared_mutex>(_mutex); }
        std::shared_lock<std::shared_mutex> _LockRead()  const { return std::shared_lock<std::shared_mutex>(_mutex); }
        Entity * _LoadModel(const std::string&) const;
        Texture * _LoadTexture(const std::string&, const TextureHandle) const;
        uint32_t _NextResourceIndex();

    private:
        static ResourceManager * _instance;
        std::vector<ThreadPtr> _threads;
        uint32_t _nextResourceVector = 0;
        mutable std::unordered_map<std::string, Async<Entity>> _loadedModels;
        mutable std::unordered_map<TextureHandle, Async<Texture>> _loadedTextures;
        mutable std::unordered_map<std::string, TextureHandle> _loadedTexturesByFile;
        mutable std::shared_mutex _mutex;
    };
}