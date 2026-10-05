// Crs.h is header-only. This file makes every library build (and so every CI
// job) compile it, also before an application includes it. On families
// without a CRS the header is empty.
#include "Crs.h"
