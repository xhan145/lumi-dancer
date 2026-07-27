// LUMI//DANCER — minimal, dependency-free test harness for the JUCE-free
// core. Register cases with LD_TEST(name){...}; assert with LD_CHECK / LD_EQ
// / LD_NEAR. main() lives in CoreTestMain.cpp and runs every registered case.
#pragma once

#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <functional>

namespace ldtest
{
struct Case { std::string name; std::function<void(struct Ctx&)> fn; };

struct Ctx
{
    int         checks = 0;
    int         fails  = 0;
    std::string current;

    void report (bool ok, const char* expr, const char* file, int line)
    {
        ++checks;
        if (!ok)
        {
            ++fails;
            std::printf ("    FAIL [%s]  %s  (%s:%d)\n", current.c_str(), expr, file, line);
        }
    }
};

inline std::vector<Case>& registry()
{
    static std::vector<Case> r;
    return r;
}

struct Registrar
{
    Registrar (const char* name, std::function<void(Ctx&)> fn)
    {
        registry().push_back ({ name, std::move (fn) });
    }
};
} // namespace ldtest

#define LD_TEST(NAME)                                                                     \
    static void NAME (ldtest::Ctx& _ctx);                                                 \
    static ldtest::Registrar _reg_##NAME (#NAME, NAME);                                   \
    static void NAME (ldtest::Ctx& _ctx)

#define LD_CHECK(EXPR)  _ctx.report ((EXPR), #EXPR, __FILE__, __LINE__)
#define LD_EQ(A, B)     _ctx.report ((A) == (B), #A " == " #B, __FILE__, __LINE__)
#define LD_NEAR(A, B, TOL) \
    _ctx.report (std::fabs ((double)(A) - (double)(B)) <= (TOL), #A " ~= " #B, __FILE__, __LINE__)
#define LD_LT(A, B)     _ctx.report ((A) <  (B), #A " < "  #B, __FILE__, __LINE__)
#define LD_LE(A, B)     _ctx.report ((A) <= (B), #A " <= " #B, __FILE__, __LINE__)
#define LD_GT(A, B)     _ctx.report ((A) >  (B), #A " > "  #B, __FILE__, __LINE__)
#define LD_GE(A, B)     _ctx.report ((A) >= (B), #A " >= " #B, __FILE__, __LINE__)
