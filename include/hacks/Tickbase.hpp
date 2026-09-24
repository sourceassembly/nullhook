#pragma once

namespace hacks::tf2::tickbase
{
extern bool shifting;
extern bool in_doubletap;

int Ticks();
int MaxTicks();

void QueueRecharge();
void RequestWarp(int use);
void Reset();

bool Active();
bool DoubletapEnabled();
bool DoubletapHeld();
bool DoubletapKeyBound();
void PollDoubletapKey();

bool WriteShiftMove();
}
