/*
 *  HDRMerge - HDR exposure merging software.
 *  Copyright 2012 Javier Celaya
 *  jcelaya@gmail.com
 *
 *  This file is part of HDRMerge.
 *
 *  HDRMerge is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  HDRMerge is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with HDRMerge. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <list>
#include <string>
#include "gui.hpp"
#include "Exposure.hpp"

int main(int argc, char * argv[]) {
    hdrmerge::GUI app(argc, argv);

    // Parse the list of images in command line
    std::list<char *> inFileNames;
    char * outFileName = NULL;
    for (int i = 1; i < argc; ++i) {
        if (std::string("-o") == argv[i]) {
            if (++i < argc)
                outFileName = argv[i];
        } else
            inFileNames.push_back(argv[i]);
    }

    if (outFileName == NULL || inFileNames.empty()) {
        return app.startGUI(inFileNames);
    } else {
        ExposureStack image;
        for (auto name : inFileNames)
            image.loadImage(name);
        image.sort();
        image.savePFS(outFileName);
        return 0;
    }
}

