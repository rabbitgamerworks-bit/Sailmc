// qmake puts all object files in one folder, where launcher/Version.cpp and launcher/meta/Version.cpp
// would both become Version.o. This file compiles the second one under another name (see sailmc.pro).
#include "../../launcher/meta/Version.cpp"
