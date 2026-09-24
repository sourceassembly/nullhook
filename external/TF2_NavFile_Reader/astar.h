#pragma once

#include "common.hpp"
#include <vector>
namespace nav {
    struct singleNode {
        typedef singleNode ThisClass;
        int id{-1};
        Vector pos;
        std::vector<singleNode *> children;

        inline void addChildren(singleNode *node) {
            children.push_back(node);
        }

        inline std::vector<singleNode *> FindPath(singleNode *goal) {
            std::vector<singleNode *> ret;
            singleNode *node = nullptr;
            singleNode *target = this;
            for (int i = 0; i < 100; ++i) {
                float bestscr = 99999999.0f;
                if (node) {
                    if (node->id == goal->id)
                        break;
                    ret.push_back(node);
                }
                for (auto child : target->children) {
                    bool rett = false;
                    for (auto sub : ret) {
                        if (sub->id == child->id) {
                            rett = true;
                            break;
                        }
                    }
                    if (rett)
                        continue;
                    if (child->id == goal->id) {
                        ret.push_back(child);
                        node = child;
                        target = child;
                        break;
                    }
                    if (child->pos.DistTo(goal->pos) < bestscr) {
                        bestscr = child->pos.DistTo(goal->pos);
                        node = child;
                        target = child;
                    }
                }
            }
            return ret;
        }
    };
}