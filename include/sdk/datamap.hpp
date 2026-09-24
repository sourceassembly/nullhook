#pragma once

#include <cstddef>
#include "datamap.h"

static_assert(sizeof(typedescription_t) == 96, "Linux TF2 typedescription_t is 96 bytes");
static_assert(offsetof(typedescription_t, fieldSizeInBytes) == 72);
static_assert(offsetof(typedescription_t, fieldTolerance) == 92);

datamap_t *PredDescMap(void *entity);
int DatamapField(void *entity, const char *name);
int DatamapFieldAny(const char *name);
