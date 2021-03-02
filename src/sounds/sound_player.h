#pragma once

class SoundPlayer
{
    public:
        SoundPlayer();
        void play_wav_from_const(const char*wav, int size);
        ~SoundPlayer();
};