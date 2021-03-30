#include "config.h"

#include "images.hpp"

#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace phosphor
{
namespace software
{
namespace image
{

std::vector<std::string> getOptionalImages()
{
    std::istringstream optionalImagesStr(OPTIONAL_IMAGES);
    std::vector<std::string> optionalImages(
        std::istream_iterator<std::string>{optionalImagesStr},
        std::istream_iterator<std::string>());
    return optionalImages;
}

} // namespace image
} // namespace software
} // namespace phosphor
