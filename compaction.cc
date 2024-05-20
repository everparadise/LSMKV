#ifndef __COMPACTION__
#define __COMPACTION__
#include "cachedData.h"
#include <vector>
// #include "Level.h"
// #include "Section.cc"
class SST::Level;
class SST::Section;
namespace Compact
{

    class compactionStrategy
    {
    public:
        virtual static bool compact(SST::Level &levelUp, SST::Level &levelDown) = 0;
        virtual static std::vector<SST::Section> select(SST::Level &levelUp, SST::Level &levelDown) = 0;
    };

    class compactionFirstLevel : compactionStrategy
    {
    public:
        virtual static std::vector<SST::Section> select(SST::Level &levelUp, SST::Level &levelDown)
        {
            return NULL;
        }

        virtual static bool compact(SST::Level &levelUp, SST::Level &levelDown)
        {
            // std::vector<Section> sections = select(levelUp, levelDown);
            return false;
        }
    }

    class compactionOtherLevel : compactionStrategy
    {
    public:
        virtual static std::vector<Section> select(SST::Level &levelUp, SST::Level &levelDown)
        {
            return NULL;
        }

        virtual static bool compact(SST::Level &levelUp, SST::Level &levelDown)
        {
            // std::vector<Section> sections = select(levelUp, levelDown);
            return false;
        }
    }

    class compactionContext
    {
    public:
        static bool compact(SST::Level &levelUp, SST::Level &levelDown)
        {
            if (levelUp.getLevel() == 0)
            {
                return compactionFirstLevel::compact(levelUp, levelDown)
            }
            return compactionOtherLevel::compact(levelUp, levelDown);
        }
    }
}

#endif //__COMPACTION__