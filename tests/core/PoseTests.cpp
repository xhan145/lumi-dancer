// Character rig tests: interpolation, angular blending, sanitisation, bounds.
#include "TestFramework.h"

#include "rig/CharacterPose.h"

using namespace lumi;

LD_TEST (pose_lerp_midpoint_and_endpoints)
{
    CharacterPose a, b;
    a.root.position = { -0.1f, 0.0f };
    b.root.position = { 0.1f, 0.2f };
    a.head.rotationRadians = 0.0f;
    b.head.rotationRadians = 0.4f;
    a.eyeOpenAmount = 1.0f;
    b.eyeOpenAmount = 0.0f;

    const CharacterPose start = lerpPose (a, b, 0.0f);
    const CharacterPose mid   = lerpPose (a, b, 0.5f);
    const CharacterPose end   = lerpPose (a, b, 1.0f);

    LD_NEAR (start.root.position.x, -0.1f, 1e-6);
    LD_NEAR (mid.root.position.x, 0.0f, 1e-6);
    LD_NEAR (mid.root.position.y, 0.1f, 1e-6);
    LD_NEAR (mid.head.rotationRadians, 0.2f, 1e-6);
    LD_NEAR (mid.eyeOpenAmount, 0.5f, 1e-6);
    LD_NEAR (end.root.position.x, 0.1f, 1e-6);
}

LD_TEST (pose_lerp_uses_shortest_angle)
{
    CharacterPose a, b;
    a.leftUpperArm.rotationRadians = 3.0f;    // near +pi
    b.leftUpperArm.rotationRadians = -3.0f;   // near -pi

    const CharacterPose mid = lerpPose (a, b, 0.5f);
    // Shortest path crosses ±pi, so the midpoint magnitude stays > 3.
    LD_GT (std::fabs (mid.leftUpperArm.rotationRadians), 3.0f);
}

LD_TEST (pose_distance_zero_for_identical_positive_for_different)
{
    CharacterPose a;
    CharacterPose b = a;
    LD_NEAR (poseDistance (a, b), 0.0f, 1e-6);

    b.root.position.y = 0.2f;
    b.head.rotationRadians = 0.5f;
    LD_GT (poseDistance (a, b), 0.1f);
}

LD_TEST (pose_sanitize_clamps_nan_and_out_of_bounds)
{
    CharacterPose p;
    p.root.position = { 99.0f, std::numeric_limits<float>::quiet_NaN() };
    p.head.rotationRadians = 100.0f;
    p.torso.scale = -5.0f;
    p.eyeOpenAmount = 3.0f;
    p.hairBounceAmount = std::numeric_limits<float>::infinity();

    sanitizePose (p);

    LD_LE (p.root.position.x, kMaxRootOffset);
    LD_CHECK (std::isfinite (p.root.position.y));
    LD_LE (std::fabs (p.head.rotationRadians), kPi);
    LD_GE (p.torso.scale, 0.25f);
    LD_LE (p.eyeOpenAmount, 1.0f);
    LD_LE (std::fabs (p.hairBounceAmount), 1.0f);
}
