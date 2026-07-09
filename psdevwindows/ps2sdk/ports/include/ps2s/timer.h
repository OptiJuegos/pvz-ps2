/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

          This file is subject to the terms and conditions of the GNU Lesser
          General Public License Version 2.1. See the file "COPYING" in the
          main directory of this archive for more details. */

#ifndef ps2s_timer_h
#define ps2s_timer_h

/********************************************
 * includes
 */

#include <stack>
#include <string>
#include <vector>

#include "ps2s/eetimer.h"

/********************************************
 * macros
 */

#define REAL_NAME_OFFSET 11

// macros are defined in ps2s/debug.h -- hence you need to
// include that file to use them.. Sorry, but it has to be this way
// because of the per-module debugging

/********************************************
 * class def
 */

class CHTimer {
public:
    static void InitHierarchy(CEETimer* realTimer,
        CEETimer::tResolution resolution);
    static void DeleteHierarchy();
    static void DisplayHierarchy(void);
    static void UpdateHierarchy(void);

    static void StartTimer(const char* name, bool enableDisplay = true);
    static unsigned int StopTimer(const char* name);

protected:
    CHTimer(const char* name, bool enableDisplay)
        : Name(name)
        , pUniqueName(name)
        , uiElapsedTicks(0)
        , Children()
    {
        Children.reserve(InitialChildrenSize);
    }

    ~CHTimer(void);

    void Start(void) { uiStartTicks = pRealTimer->GetTicks(); };

    unsigned int Stop(void)
    {
        unsigned int ticks = pRealTimer->GetTicks();
        uiElapsedTicks += ticks - uiStartTicks;

        return ticks;
    }

    void AddChild(CHTimer* newChild) { Children.push_back(newChild); }

    CHTimer* GetChild(const char* namePtr)
    {
        CHTimer* child = NULL;
        for (unsigned int i = 0; i < Children.size(); i++) {
            if (Children[i]->pUniqueName == namePtr) {
                child = Children[i];
                break;
            }
        }
        return child;
    }

    void Display(const std::string& path);
    void Update(void);

    std::string Name;
    const char* pUniqueName;

    unsigned int uiStartTicks;
    unsigned int uiElapsedTicks;

private:
    static const unsigned int InitialChildrenSize = 10;
    std::vector<CHTimer*> Children;

    static CHTimer* pTopTimer;
    static std::stack<CHTimer*, std::vector<CHTimer*> >* TimerStack;

    static CEETimer* pRealTimer;
};

#endif // ps2s_timer_h
