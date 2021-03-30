#pragma once

#include <string>

/** Boolean variable controlling whether tuning is enabled
 * during this run.
 */
extern bool tuningEnabled;
/** String variable with the folder for writing logs if logging is enabled.
 */
extern std::string loggingPath;
/** Boolean variable whether loggingPath is non-empty. */
extern bool loggingEnabled;
