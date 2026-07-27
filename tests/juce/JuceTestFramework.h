// LUMI//DANCER — shared harness for the JUCE-linked test suite.
#pragma once

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace jt
{
struct Ctx
{
    int checks = 0, fails = 0;
    std::string current;

    void report (bool ok, const char* expr, const char* file, int line)
    {
        ++checks;
        if (! ok)
        {
            ++fails;
            std::printf ("    FAIL [%s]  %s  (%s:%d)\n", current.c_str(), expr, file, line);
        }
    }
};

struct Case { std::string name; std::function<void (Ctx&)> fn; };

inline std::vector<Case>& registry()
{
    static std::vector<Case> r;
    return r;
}

struct Registrar
{
    Registrar (const char* name, std::function<void (Ctx&)> fn)
    {
        registry().push_back ({ name, std::move (fn) });
    }
};
} // namespace jt

#define JT_TEST(NAME)                                                              \
    static void NAME (jt::Ctx& _ctx);                                              \
    static jt::Registrar _reg_##NAME (#NAME, NAME);                                \
    static void NAME (jt::Ctx& _ctx)

#define JT_CHECK(EXPR) _ctx.report ((EXPR), #EXPR, __FILE__, __LINE__)
#define JT_EQ(A, B)    _ctx.report ((A) == (B), #A " == " #B, __FILE__, __LINE__)
#define JT_NEAR(A, B, TOL) \
    _ctx.report (std::fabs ((double) (A) - (double) (B)) <= (TOL), #A " ~= " #B, __FILE__, __LINE__)
#define JT_GT(A, B)    _ctx.report ((A) > (B), #A " > " #B, __FILE__, __LINE__)
