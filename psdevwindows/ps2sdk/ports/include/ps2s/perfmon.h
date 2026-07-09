/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef PERF_H
#define PERF_H

#include "ps2s/core.h"
#include "ps2s/types.h"

class PerfTest {

#define PERF_PROCESSOR_CYCLE 0
#define PERF_INSTRUCTION_COMPLETED 1
#define PERF_NON_BDS_INSTRUCTION_COMPLETED 2
#define PERF_COP1_INSTRUCTION_COMPLETED 3
#define PERF_COP2_INSTRUCTION_COMPLETED 4
#define PERF_DUAL_INSTRUCTION_ISSUE 5
#define PERF_SINGLE_INST_ISSUE 6
#define PERF_LOW_ORDER_BRANCH_ISSUES 7
#define PERF_BRANCH_ISSUED 8
#define PERF_BRANCH_MISPREDICTED 9
#define PERF_BTAC_MISS 10
#define PERF_TLB_MISS 11
#define PERF_DTLB_MISS 12
#define PERF_DTLB_ACCESSED 13
#define PERF_DCACHE_MISS 14
#define PERF_ICACHE_MISS 15
#define PERF_WBB_SINGLE_REQUEST 16
#define PERF_WBB_BURST_REQUEST 17
#define PERF_CPU_DATA_BUS_BUSY 18
#define PERF_CPU_ADDRESS_BUS_BUSY 19
#define PERF_NON_BLOCK_LOAD 20
#define PERF_LOAD_COMPLETED 21
#define PERF_STORE_COMPLETED 22
#define PERF_WBB_SINGLE_REQUEST_UNAVAILABLE 23
#define PERF_WBB_BURST_REQUEST_UNAVAILABLE 24
#define PERF_WBB_BURST_REQUEST_ALMOST_FULL 25
#define PERF_WBB_BURST_REQUEST_FULL 26

private:
    static const uint32_t NUM_PRF_TESTS = 27;

    typedef struct prfDef {
        uint32_t cnt_id;
        uint32_t cnt_val;
    } prfDef;

public:
    PerfTest();
    ~PerfTest();

    inline void StartSampling()
    {
        Core::ZeroCount();
        Core::SetupPerfCounters(
            prfTable[m_iCurrentTest[0]].cnt_val,
            prfTable[m_iCurrentTest[1]].cnt_val);
    };
    inline void EndSampling()
    {
        Core::HaltPerfCounters();
        m_CycleCount                  = Core::GetCount();
        m_testVals[m_iCurrentTest[0]] = Core::ReadPerfCounter0();
        m_testVals[m_iCurrentTest[1]] = Core::ReadPerfCounter1();
        StepTests();
    }

    void DumpBusStats(int nLoops);
    void DumpCycleCount(int nLoops);
    void DumpStats(int nLoops);
    void DumpStats()
    {
        DumpStats(1);
    }

    int sDumpStats(char* buffer, int nLoops);
    int sDumpStats(char* buffer)
    {
        return sDumpStats(buffer, 1);
    }

    void DebugState();

    int getNumTests() { return m_nTests; };
    int m_CycleCount;

private:
    int m_nTests;
    int m_iCurrentTest[Core::COP0_NUM_PERF_COUNTERS]; //
    static prfDef prfTable[NUM_PRF_TESTS];
    int m_Prev_testVals[NUM_PRF_TESTS];
    int m_testVals[NUM_PRF_TESTS];
    int GetNextTestID(uint32_t counter_id, uint32_t ctest);
    void StepTests();
};

#endif
