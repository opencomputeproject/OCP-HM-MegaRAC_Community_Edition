#include <cstdio>
#include <string>

// Get the version string for a PSU and output to stdout
// In this example, it just returns the last 8 bytes as the version
constexpr int NUM_OF_BYTES = 8;

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <psu-inventory-path>\n", argv[0]);
        return 1;
    }

    std::string psu = argv[1];
    if (psu.size() < NUM_OF_BYTES)
    {
        psu.append(NUM_OF_BYTES - psu.size(), '0'); //"0", 8 - psu.size());
    }

    printf("%s", psu.substr(psu.size() - NUM_OF_BYTES).c_str());

    return 0;
}
