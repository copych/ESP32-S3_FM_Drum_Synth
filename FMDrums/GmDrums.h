#pragma once

#include <stdint.h>

enum class GmDrumNote : uint8_t {
    AcousticBassDrum = 35,
    BassDrum1,
    SideStick,
    AcousticSnare,
    HandClap,
    ElectricSnare,
    LowFloorTom,
    ClosedHiHat,
    HighFloorTom,
    PedalHiHat,
    LowTom,
    OpenHiHat,
    LowMidTom,
    HiMidTom,
    CrashCymbal1,
    HighTom,
    RideCymbal1,
    ChineseCymbal,
    RideBell,
    Tambourine,
    SplashCymbal,
    Cowbell,
    CrashCymbal2,
    Vibraslap,
    RideCymbal2,
    HiBongo,
    LowBongo,
    MuteHiConga,
    OpenHiConga,
    LowConga,
    HighTimbale,
    LowTimbale,
    HighAgogo,
    LowAgogo,
    Cabasa,
    Maracas,
    ShortWhistle,
    LongWhistle,
    ShortGuiro,
    LongGuiro,
    Claves,
    HiWoodBlock,
    LowWoodBlock,
    MuteCuica,
    OpenCuica,
    MuteTriangle,
    OpenTriangle
};

inline const char* gmDrumNoteName(GmDrumNote note) {
    switch (note) {
        case GmDrumNote::AcousticBassDrum: return "Acoustic Bass Drum";
        case GmDrumNote::BassDrum1:        return "Bass Drum 1";
        case GmDrumNote::SideStick:        return "Side Stick";
        case GmDrumNote::AcousticSnare:    return "Acoustic Snare";
        case GmDrumNote::HandClap:         return "Hand Clap";
        case GmDrumNote::ElectricSnare:    return "Electric Snare";
        case GmDrumNote::LowFloorTom:      return "Low Floor Tom";
        case GmDrumNote::ClosedHiHat:      return "Closed Hi-Hat";
        case GmDrumNote::HighFloorTom:     return "High Floor Tom";
        case GmDrumNote::PedalHiHat:       return "Pedal Hi-Hat";
        case GmDrumNote::LowTom:           return "Low Tom";
        case GmDrumNote::OpenHiHat:        return "Open Hi-Hat";
        case GmDrumNote::LowMidTom:        return "Low-Mid Tom";
        case GmDrumNote::HiMidTom:         return "Hi-Mid Tom";
        case GmDrumNote::CrashCymbal1:     return "Crash Cymbal 1";
        case GmDrumNote::HighTom:          return "High Tom";
        case GmDrumNote::RideCymbal1:      return "Ride Cymbal 1";
        case GmDrumNote::ChineseCymbal:    return "Chinese Cymbal";
        case GmDrumNote::RideBell:         return "Ride Bell";
        case GmDrumNote::Tambourine:       return "Tambourine";
        case GmDrumNote::SplashCymbal:     return "Splash Cymbal";
        case GmDrumNote::Cowbell:          return "Cowbell";
        case GmDrumNote::CrashCymbal2:     return "Crash Cymbal 2";
        case GmDrumNote::Vibraslap:        return "Vibraslap";
        case GmDrumNote::RideCymbal2:      return "Ride Cymbal 2";
        case GmDrumNote::HiBongo:          return "Hi Bongo";
        case GmDrumNote::LowBongo:         return "Low Bongo";
        case GmDrumNote::MuteHiConga:      return "Mute Hi Conga";
        case GmDrumNote::OpenHiConga:      return "Open Hi Conga";
        case GmDrumNote::LowConga:         return "Low Conga";
        case GmDrumNote::HighTimbale:      return "High Timbale";
        case GmDrumNote::LowTimbale:       return "Low Timbale";
        case GmDrumNote::HighAgogo:        return "High Agogo";
        case GmDrumNote::LowAgogo:         return "Low Agogo";
        case GmDrumNote::Cabasa:           return "Cabasa";
        case GmDrumNote::Maracas:          return "Maracas";
        case GmDrumNote::ShortWhistle:     return "Short Whistle";
        case GmDrumNote::LongWhistle:      return "Long Whistle";
        case GmDrumNote::ShortGuiro:       return "Short Guiro";
        case GmDrumNote::LongGuiro:        return "Long Guiro";
        case GmDrumNote::Claves:           return "Claves";
        case GmDrumNote::HiWoodBlock:      return "Hi Wood Block";
        case GmDrumNote::LowWoodBlock:     return "Low Wood Block";
        case GmDrumNote::MuteCuica:        return "Mute Cuica";
        case GmDrumNote::OpenCuica:        return "Open Cuica";
        case GmDrumNote::MuteTriangle:     return "Mute Triangle";
        case GmDrumNote::OpenTriangle:     return "Open Triangle";
        default: return "Unknown";
    }
}
