#pragma once
#include "type.h"

namespace BO{

    class Data{
    public:
        std::vector<Network*> networks;
        std::vector<ModeType*> mode_types;
        std::vector<Line*> lines;
        std::vector<Mode*> modes;

    };
}
