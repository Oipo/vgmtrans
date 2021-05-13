#pragma once
#include "VGMSeq.h"
#include "SeqTrack.h"
#include "SeqEvent.h"
#include "KonamiPS1Format.h"

class KonamiPS1Seq : public VGMSeq {
public:
    static constexpr uint32_t kHeaderSize = 16;
    static constexpr uint32_t kOffsetToFileSize = 4;
    static constexpr uint32_t kOffsetToTimebase = 8;
    static constexpr uint32_t kOffsetToTrackCount = 0x0c;
    static constexpr uint32_t kOffsetToTrackSizes = 0x10;

    KonamiPS1Seq(RawFile *file, uint32_t offset, const std::wstring &_name = L"KonamiPS1Seq");

    ~KonamiPS1Seq() override = default;

    bool GetHeaderInfo() override;
    bool GetTrackPointers() override;
    void ResetVars() override;

    static bool IsKDT1Seq(RawFile *file, uint32_t offset);

    static uint32_t GetKDT1FileSize(RawFile *file, uint32_t offset) {
        return kHeaderSize + file->GetWord(offset + kOffsetToFileSize);
    }
};

class KonamiPS1Track : public SeqTrack {
public:
    KonamiPS1Track(KonamiPS1Seq *parentSeq, uint32_t offset, uint32_t length);

    void ResetVars() override;
    bool ReadEvent() override;

private:
    bool skipDeltaTime;
};
