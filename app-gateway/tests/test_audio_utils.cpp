#include <iostream>
#include <string>
#include "helpers/AudioUtils.h"

using namespace WPEFramework::Plugin;

static int test_normalization() {
    int failures = 0;

    struct Case { std::string in; std::string expect; };
    Case cases[] = {
        {"DOLBY AC3", "Dolby Digital"},
        {"Dolby Digital", "Dolby Digital"},
        {"DOLBY EAC3", "Dolby Digital Plus"},
        {"Dolby Digital Plus", "Dolby Digital Plus"},
        {"some ATMOS value", "Dolby Atmos"},
        {"PCM", "Stereo"},
        {"Stereo", "Stereo"},
        {"", "Unknown"},
        {"UNKNOWN", "Unknown"},
    };

    for (const auto& c : cases) {
        auto out = NormalizeHdmiAudioFormat(c.in);
        if (out != c.expect) {
            std::cerr << "FAIL: Normalize '" << c.in << "' -> '" << out << "' (expected '" << c.expect << "')" << std::endl;
            failures++;
        }
    }

    return failures;
}

static int test_capability_json() {
    int failures = 0;

    struct Case { std::string norm; std::string mustContain; };
    Case cases[] = {
        {"Dolby Atmos", "\"dolbyAtmos\":true"},
        {"Dolby Digital", "\"dolbyDigital5.1\":true"},
        {"Dolby Digital Plus", "\"dolbyDigital5.1+\":true"},
        {"Stereo", "\"stereo\":true"},
        {"Unknown", "\"stereo\":true"},
    };

    for (const auto& c : cases) {
        auto json = BuildAudioCapabilityJsonFromFormat(c.norm);
        if (json.find(c.mustContain) == std::string::npos) {
            std::cerr << "FAIL: Capability JSON for '" << c.norm << "' does not contain " << c.mustContain
                      << " -> json=" << json << std::endl;
            failures++;
        }
    }

    return failures;
}

int main() {
    int failures = 0;
    failures += test_normalization();
    failures += test_capability_json();

    if (failures == 0) {
        std::cout << "AudioUtils tests passed." << std::endl;
    } else {
        std::cerr << failures << " AudioUtils test(s) failed." << std::endl;
    }
    return failures == 0 ? 0 : 1;
}
