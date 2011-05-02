#pragma once
#include "types.h"

namespace BO{

    class Data{
    public:
        std::vector<BO::types::Network*> networks;
        std::vector<BO::types::ModeType*> mode_types;
        std::vector<BO::types::Line*> lines;
        std::vector<BO::types::Mode*> modes;

    };
}
