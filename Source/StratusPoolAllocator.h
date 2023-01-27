#pragma once

#include <memory>
#include <algorithm>
#include <thread>
#include <shared_mutex>
#include <functional>

// See https://www.qt.io/blog/a-fast-and-thread-safe-pool-allocator-for-qt-part-1
// for some more information

namespace stratus {
    // Allocates memory of a pre-defined size provide optimal data locality
    // with zero fragmentation
    template<typename E, typename Lock, size_t ElemsPerChunk, size_t Chunks>
    struct __PoolAllocator {
        static_assert(ElemsPerChunk > 0);
        static_assert(Chunks > 0);

        // We need to at least be able to store the next pointer
        static constexpr size_t BytesPerElem = std::max<size_t>(sizeof(void *), sizeof(E));
        static constexpr size_t BytesPerChunk = BytesPerElem * ElemsPerChunk;

        __PoolAllocator() {
            for (size_t i = 0; i < Chunks; ++i) {
                _InitChunk();
            }
        }

        // Copying is not supported at all
        __PoolAllocator(__PoolAllocator&&) = delete;
        __PoolAllocator(const __PoolAllocator&) = delete;
        __PoolAllocator& operator=(__PoolAllocator&&) = delete;
        __PoolAllocator& operator=(const __PoolAllocator&) = delete;

        virtual ~__PoolAllocator() {
            _Chunk * c = _chunks;
            for (size_t i = 0; i < Chunks; ++i) {
                _Chunk * tmp = c;
                c = c->next;
                delete tmp;
            }

            _frontBuffer = nullptr;
            _chunks = nullptr;
        }

        template<typename ... Types>
        E * Allocate(Types ... args) {
            uint8_t * bytes = nullptr;
            {
                //auto wlf = _frontBufferLock.LockWrite();
                if (!_frontBuffer) {
                    auto wlb = _backBufferLock.LockWrite();
                    // If back buffer has free nodes, swap front with back
                    if (_backBuffer) {
                        _frontBuffer = _backBuffer;
                        _backBuffer = nullptr;
                    }
                    // If not allocate a new chunk of memory
                    else {
                        _InitChunk();
                    }
                }

                _MemBlock * next = _frontBuffer;
                _frontBuffer = _frontBuffer->next;
                bytes = reinterpret_cast<uint8_t *>(next);
            }
            return new (bytes) E(std::forward<Types>(args)...);
        }

        void Deallocate(E * ptr) {
            ptr->~E();
            auto wlb = _backBufferLock.LockWrite();
            uint8_t * bytes = reinterpret_cast<uint8_t *>(ptr);
            _MemBlock * b = reinterpret_cast<_MemBlock *>(bytes);
            b->next = _backBuffer;
            _backBuffer = b;
        }

        size_t NumChunks() const {
            //auto sl = _frontBufferLock.LockRead();
            return _numChunks;
        }

        size_t NumElems() const {
            //auto sl = _frontBufferLock.LockRead();
            return _numElems;
        }

    private:
        // Each _MemBlock sits in front of BytesPerElem memory
        struct alignas(sizeof(void *)) _MemBlock {
            _MemBlock * next;
        };

        struct _Chunk {
            uint8_t memory[BytesPerChunk];
            _Chunk * next = nullptr;
        };

        //Lock _frontBufferLock;
        Lock _backBufferLock;
        // Having two allows us to largely decouple allocations from deallocations
        // and only synchronize the two when we run out of free memory
        //
        // (we are running with the assumption that only one thread can allocate but many
        //  can deallocate for the same allocator object)
        _MemBlock * _frontBuffer = nullptr;
        _MemBlock * _backBuffer = nullptr;
        _Chunk * _chunks = nullptr;
        size_t _numChunks = 0;
        size_t _numElems = 0;

    private:
        void _InitChunk() {
            _Chunk * c = new _Chunk();
            ++_numChunks;
            _numElems += ElemsPerChunk;

            // Start at the end and add to freelist in reverse order
            uint8_t * mem = c->memory + BytesPerElem * (ElemsPerChunk - 1);
            for (size_t i = ElemsPerChunk; i > 0; --i, mem -= BytesPerElem) {
                _MemBlock * b = reinterpret_cast<_MemBlock *>(mem);
                b->next = _frontBuffer;
                _frontBuffer = b;
            }

            c->next = _chunks;
            _chunks = c;
        }
    };

    struct NoOpLock {
        struct NoOpLockHeld{};
        typedef NoOpLockHeld value_type;

        value_type LockRead() const {
            return value_type();
        }

        value_type LockWrite() const {
            return value_type();
        }
    };

    template<typename E, size_t ElemsPerChunk = 64, size_t Chunks = 1>
    struct ThreadUnsafePoolAllocator : public __PoolAllocator<E, NoOpLock, ElemsPerChunk, Chunks> {
        virtual ~ThreadUnsafePoolAllocator() = default;
    };

    struct Lock {
        struct LockHeld {
            typedef void (*UnlockFunction)(std::shared_mutex *);
            UnlockFunction unlock;
            std::shared_mutex * m;

            LockHeld(UnlockFunction unlock,
                     std::shared_mutex * m)
                : unlock(unlock), m(m) {
            }

            ~LockHeld() {
                unlock(m);
            }
        };

        typedef LockHeld value_type;

        const std::thread::id owner;
        mutable std::shared_mutex m;

        Lock(const std::thread::id& owner = std::this_thread::get_id())
            : owner(owner) {}

        value_type LockRead() const {
            _LockShared(&m);
            return value_type(_UnlockShared, &m);
        }
        
        value_type LockWrite() const {
            // Allowing the owner to always lock read gives a performance increase
            if (std::this_thread::get_id() == owner) {
                return LockRead();
            }
            _LockUnique(&m);
            return value_type(_UnlockUnique, &m);
        }

    private:
        static void _LockShared(std::shared_mutex * m) {
            m->lock_shared();
        }

        static void _UnlockShared(std::shared_mutex * m) {
            m->unlock_shared();
        }

        static void _LockUnique(std::shared_mutex * m) {
            m->lock();
        }

        static void _UnlockUnique(std::shared_mutex * m) {
            m->unlock();
        }
    };

    template<typename E, size_t ElemsPerChunk = 64, size_t Chunks = 1>
    struct ThreadSafePoolAllocator {
        typedef __PoolAllocator<E, Lock, ElemsPerChunk, Chunks> Allocator;
        static constexpr size_t BytesPerElem = Allocator::BytesPerElem;
        static constexpr size_t BytesPerChunk = Allocator::BytesPerChunk;

        struct ThreadSafePoolDeleter {
            std::shared_ptr<Allocator> allocator;
            ThreadSafePoolDeleter(const std::shared_ptr<Allocator>& allocator)
                : allocator(allocator) {}

            void operator()(E * ptr) {
                allocator->Deallocate(ptr);
            }
        };

        typedef std::unique_ptr<E, ThreadSafePoolDeleter> UniquePtr;
        typedef std::shared_ptr<E> SharedPtr;

        ThreadSafePoolAllocator() {}

        template<typename ... Types>
        static UniquePtr Allocate(Types ... args) {
            return UniquePtr(_allocator->Allocate(std::forward<Types>(args)...), ThreadSafePoolDeleter(_allocator));
        }

        template<typename ... Types>
        static SharedPtr AllocateShared(Types ... args) {
            return SharedPtr(_allocator->Allocate(std::forward<Types>(args)...), ThreadSafePoolDeleter(_allocator));
        }

        static size_t NumChunks() {
            return _allocator->NumChunks();
        }

        static size_t NumElems() {
            return _allocator->NumElems();
        }

    private:
        inline static thread_local std::shared_ptr<Allocator> _allocator = std::make_shared<Allocator>();
    };
}