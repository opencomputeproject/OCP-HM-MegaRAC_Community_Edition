#include <exception>
#include <gpioplus/chip.hpp>
#include <gpioplus/handle.hpp>
#include <string>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "pulse [chip id] [line offset]\n");
        return 1;
    }

    try
    {
        unsigned chip_id = std::stoi(argv[1]);
        uint32_t line_offset = std::stoi(argv[2]);

        gpioplus::Chip chip(chip_id);
        gpioplus::HandleFlags flags(chip.getLineInfo(line_offset).flags);
        flags.output = true;
        gpioplus::Handle handle(chip, {{line_offset, 0}}, flags,
                                "example/pulse");
        handle.setValues({1});
        handle.setValues({0});
        return 0;
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "Error: %s\n", e.what());
    }
    return 1;
}
