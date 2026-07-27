#include "TestFramework.h"

int main()
{
    ldtest::Ctx ctx;
    int caseFails = 0;

    std::printf ("LUMI//DANCER core tests\n=======================\n");
    for (auto& testCase : ldtest::registry())
    {
        ctx.current = testCase.name;
        const int failsBefore = ctx.fails;
        testCase.fn (ctx);
        const bool ok = ctx.fails == failsBefore;
        if (! ok)
            ++caseFails;
        std::printf ("  [%s] %s\n", ok ? "PASS" : "FAIL", testCase.name.c_str());
    }

    std::printf ("=======================\n%zu cases, %d checks, %d failures\n",
                 ldtest::registry().size(), ctx.checks, ctx.fails);
    return caseFails == 0 ? 0 : 1;
}
