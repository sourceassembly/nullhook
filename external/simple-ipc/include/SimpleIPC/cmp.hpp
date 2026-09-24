/*
 * cmp.hpp
 *
 *  Created on: Mar 18, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace simple_ipc
{
class CatMemoryPool
{
public:
    struct PoolBlock
    {
        bool free;
        std::size_t size;
        std::shared_ptr<PoolBlock> prev;
        std::shared_ptr<PoolBlock> next;
    };

    struct PoolInfo
    {
        unsigned long free;
        unsigned long alloc;
        unsigned int freeblk;
        unsigned int allocblk;
        unsigned int blkcnt;
    };

    CatMemoryPool(void *base, std::size_t size) : base(base), size(size)
    {
    }

    void init() const
    {
        std::memset(base, 0, size);
        PoolBlock zeroth_block{};
        zeroth_block.free = true;
        zeroth_block.next = nullptr;
        zeroth_block.prev = nullptr;
        zeroth_block.size = size;
        std::memcpy(base, &zeroth_block, sizeof(PoolBlock));
    }

    void *alloc(std::size_t size)
    {
        auto block = FindBlock(size);
        if (!block)
            return nullptr;
        ChipBlock(block, size);
        block->free = false;
        return reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(block.get()) + sizeof(PoolBlock));
    }

    void free(void *object)
    {
        auto block  = reinterpret_cast<PoolBlock *>(reinterpret_cast<std::uintptr_t>(object) - sizeof(PoolBlock));
        block->free = true;
        MendBlock(block);
    }

    void statistics(PoolInfo &info) const
    {
        std::memset(&info, 0, sizeof(PoolInfo));
        auto current = reinterpret_cast<PoolBlock *>(base);
        while (current)
        {
            if (current->free)
            {
                info.freeblk++;
                info.free += current->size;
            }
            info.blkcnt++;

            if (!current->next)
                break;
            current = real_pointer(current->next.get());
        }
        info.alloc    = size - info.free;
        info.allocblk = info.blkcnt - info.freeblk;
    }

    template <typename T> T *real_pointer(T *pointer) const
    {
        return reinterpret_cast<T *>(reinterpret_cast<std::uintptr_t>(base) + reinterpret_cast<std::uintptr_t>(pointer));
    }

    template <typename T> void *pool_pointer(T *pointer) const
    {
        return reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(pointer) - reinterpret_cast<std::uintptr_t>(base));
    }

private:
    void *base;
    const std::size_t size;

    std::shared_ptr<PoolBlock> FindBlock(std::size_t size) const
    {
        auto current = reinterpret_cast<PoolBlock *>(base);
        while (current)
        {
            if (current->free && current->size >= size)
                return reinterpret_cast<const std::shared_ptr<simple_ipc::CatMemoryPool::PoolBlock> &>(current);
            if (!current->next)
                break;
            current = real_pointer(current->next.get());
        }
        return nullptr;
    }

    void ChipBlock(const std::shared_ptr<PoolBlock> &block, std::size_t size) const
    {
        if (block->size - sizeof(PoolBlock) > size)
        {
            unsigned int old_size = block->size;
            block->size           = size;
            PoolBlock new_block{};
            new_block.prev    = std::make_shared<PoolBlock>(*block);
            new_block.next    = block->next;
            new_block.free    = true;
            new_block.size    = old_size - (size + sizeof(PoolBlock));
            void *p_new_block = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(pool_pointer(block.get())) + sizeof(PoolBlock) + block->size);
            if (block->next)
                real_pointer(block->next.get())->prev = std::make_shared<PoolBlock>(*static_cast<PoolBlock *>(p_new_block));
            block->next = std::make_shared<PoolBlock>(*static_cast<PoolBlock *>(p_new_block));
            std::memcpy(real_pointer(p_new_block), &new_block, sizeof(PoolBlock));
        }
    }

    void MendBlock(PoolBlock *block)
    {
        if (block->prev)
        {
            auto cur_prev = real_pointer(block->prev.get());
            if (cur_prev->free)
            {
                MendBlock(cur_prev);
                return;
            }
        }
        if (block->next)
        {
            auto cur_next = real_pointer(block->next.get());
            while (cur_next->free)
            {
                block->size += sizeof(PoolBlock) + cur_next->size;
                DeleteBlock(cur_next);
                if (block->next)
                    cur_next = real_pointer(block->next.get());
                else
                    break;
            }
        }
    }

    void DeleteBlock(PoolBlock *block) const
    {
        if (block->next)
            real_pointer(block->next.get())->prev = block->prev;
        if (block->prev)
            real_pointer(block->prev.get())->next = block->next;
    }
};
} // namespace simple_ipc
