/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008 Larry Gritz
// 
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// (this is the MIT license)
/////////////////////////////////////////////////////////////////////////////


#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <iostream>
#include <iterator>

#include <boost/foreach.hpp>

#include "argparse.h"
#include "imageio.h"
using namespace OpenImageIO;


static bool verbose = false;
static bool sum = false;
static bool help = false;
static std::vector<std::string> filenames;



static void
print_info (const std::string &filename, ImageInput *input,
            ImageSpec &spec,
            bool verbose, bool sum, long long &totalsize)
{
    printf ("%s : %4d x %4d", filename.c_str(), 
            spec.width, spec.height);
    if (spec.depth > 1)
        printf (" x %4d", spec.depth);
    printf (", %d channel, %s%s", spec.nchannels,
            typestring(spec.format),
            spec.depth > 1 ? " volume" : "");
    if (sum) {
        totalsize += spec.image_bytes();
        printf (" (%.2f MB)", (float)spec.image_bytes() / (1024.0*1024.0));
    }
    printf ("\n");

    if (verbose) {
        printf ("    channel list: ");
        for (int i = 0;  i < spec.nchannels;  ++i) {
            printf ("%s%s", spec.channelnames[i].c_str(),
                    (i == spec.nchannels-1) ? "" : ", ");
        }
        printf ("\n");
        if (spec.x || spec.y || spec.z) {
            printf ("    origin: x=%d, y=%d", spec.x, spec.y);
            if (spec.depth > 1)
                printf (", z=%d\n", spec.x, spec.y, spec.z);
            printf ("\n");
        }
        if (spec.full_width != spec.width || spec.full_height != spec.height ||
            spec.full_depth != spec.depth) {
            printf ("    full (uncropped) size: %4d x %d",
                    spec.full_width, spec.full_height);
            if (spec.depth > 1)
                printf (" x %d", spec.full_depth);
            printf ("\n");
        }
        if (spec.tile_width) {
            printf ("    tile size: %d x %d",
                    spec.tile_width, spec.tile_height);
            if (spec.depth > 1)
                printf (" x %d", spec.tile_depth);
            printf ("\n");
        }
        switch (spec.linearity) {
        case ImageSpec::Linear :
            printf ("    linear color space\n");
            break;
        case ImageSpec::GammaCorrected :
            printf ("    gamma-corrected: %g\n", spec.gamma);
            break;
        case ImageSpec::sRGB :
            printf ("    sRGB color space\n");
            break;
        default:
            printf ("    unknown color space\n");
        }
        BOOST_FOREACH (const ImageIOParameter &p, spec.extra_attribs) {
            printf ("    %s: ", p.name().c_str());
            if (p.type() == PT_STRING)
                printf ("\"%s\"", *(const char **)p.data());
            else if (p.type() == PT_FLOAT)
                printf ("%g", *(const float *)p.data());
            else if (p.type() == PT_INT)
                printf ("%d", *(const int *)p.data());
            else if (p.type() == PT_UINT)
                printf ("%u", *(const unsigned int *)p.data());
            else
                printf ("<unknown data type>");
            printf ("\n");
        }
    }
}



static int
parse_files (int argc, const char *argv[])
{
    for (int i = 0;  i < argc;  i++)
        filenames.push_back (argv[i]);
    return 0;
}



int
main (int argc, const char *argv[])
{
    ArgParse ap (argc, argv);
    if (ap.parse ("Usage:  iinfo [options] filename...",
                  "%*", parse_files, "",
                  "--help", &help, "Print help message",
                  "-v", &verbose, "Verbose output",
                  "-s", &sum, "Sum the image sizes",
                  NULL) < 0) {
        std::cerr << ap.error_message() << std::endl;
        ap.usage ();
        return EXIT_FAILURE;
    }
    if (help) {
        ap.usage ();
        exit (EXIT_FAILURE);
    }

    long long totalsize = 0;
    BOOST_FOREACH (const std::string &s, filenames) {
        ImageInput *in = ImageInput::create (s.c_str(), "" /* searchpath */);
        if (! in) {
            std::cerr << OpenImageIO::error_message() << "\n";
            continue;
        }
        ImageSpec spec;
        if (in->open (s.c_str(), spec)) {
            print_info (s, in, spec, verbose, sum, totalsize);
            in->close ();
        } else {
            fprintf (stderr, "iinfo: Could not open \"%s\" : %s\n",
                     s.c_str(), in->error_message().c_str());
        }
        delete in;
    }

    if (sum) {
        double t = (double)totalsize / (1024.0*1024.0);
        if (t > 1024.0)
            printf ("Total size: %.2f GB\n", t/1024.0);
        else 
            printf ("Total size: %.2f MB\n", t);
    }

    return 0;
}
