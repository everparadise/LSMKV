#ifndef __COMPACTION__
#define __COMPACTION__
#include <vector>
#include "Level.hpp"
#include "Section.hpp"

namespace Compact
{

    // class compactionStrategy
    // {
    // public:
    //     virtual static bool compact(SST::Level &levelUp, SST::Level &levelDown) = 0;
    //     virtual static std::vector<SST::Section> select(SST::Level &levelUp, SST::Level &levelDown) = 0;
    // };

    class compactionFirstLevel
    {
    public:
        static std::vector<SST::Section> select(SST::Level &levelUp, SST::Level &levelDown)
        {
            std::vector<SST::Section> sec;
            return sec;
        }

        static bool compact(SST::Level &levelUp, SST::Level &levelDown)
        {
            // std::vector<Section> sections = select(levelUp, levelDown);
            return false;
        }
    };

    class compactionOtherLevel
    {
    public:
        static std::vector<SST::Section> select(SST::Level &levelUp, SST::Level &levelDown)
        {
            std::vector<SST::Section> sec;
            return sec;
        }

        static bool compact(SST::Level &levelUp, SST::Level &levelDown)
        {
            // std::vector<Section> sections = select(levelUp, levelDown);
            return false;
        }
    };

    class compactionContext
    {
    public:
        static bool compact(SST::Level &levelUp, SST::Level &levelDown)
        {
            if (levelUp.getLevel() == 0)
            {
                return compactionFirstLevel::compact(levelUp, levelDown);
            }
            return compactionOtherLevel::compact(levelUp, levelDown);
        }
    };
}

#endif //__COMPACTION__