//
// Created by Troldal on 2019-03-13.
//

#ifndef ZIPPY_H
#define ZIPPY_H

#ifdef Q_OS_MACOS
#include "../BookSmithEPUB/Zippy/ZipArchive.hpp"
#include "../BookSmithEPUB/Zippy/ZipEntry.hpp"
#include "../BookSmithEPUB/Zippy/ZipException.hpp"
#else
#include "Zippy/ZipArchive.hpp"
#include "Zippy/ZipEntry.hpp"
#include "Zippy/ZipException.hpp"
#endif

#endif //ZIPPY_H
