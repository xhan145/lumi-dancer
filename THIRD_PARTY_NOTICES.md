# Third-Party Notices

| Dependency | Version | License | Usage |
|------------|---------|---------|-------|
| [JUCE](https://juce.com) | 8.0.8 (pinned, fetched at configure time) | AGPLv3 (open-source tier) / commercial dual license | Plugin framework: VST3/Standalone wrappers, GUI, graphics |
| VST3 SDK (bundled inside JUCE) | as shipped with JUCE 8.0.8 | GPLv3 / proprietary Steinberg dual license | VST3 plugin format |

Notes:

- JUCE is fetched by CMake at configure time from the pinned 8.0.8 release
  archive; it is not vendored into this repository.
- The JUCE splash screen remains enabled (`JUCE_DISPLAY_SPLASH_SCREEN=1`) as
  required by the JUCE open-source tier.
- The Lumi mascot is rendered procedurally from vector paths defined in this
  repository's source code. No third-party character art, sprite sheets,
  fonts beyond JUCE's bundled defaults, or animation middleware are used.
- No machine-learning models, cloud services, telemetry, accounts or
  proprietary host SDKs are used. The plugin runs entirely locally.
- The JUCE-free core library (`lumi_core`) depends only on the C++20
  standard library.
