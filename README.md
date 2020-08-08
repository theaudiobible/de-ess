### de-ess - a software de-esser to reduce sibilance in speech.

### Copyright (C) 2008-2013 Theophilus (http://theaudiobible.org)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


This software is released in the hope that it may be useful to others.  Currently it works on mono 44100Hz PCM wav files and needs to be modified to work in a more general fashion.  If you modify the software, please send your patches to the author to be made available for the benefit of others.

Dependency - you need FFTW from http://fftw.org/
To install FFTW in Redhat/Fedora:
sudo yum install fftw-devel

To install FFTW in Debian/Ubuntu:
sudo apt-get install fftw-dev

To build and install, just run:
make
make install

Usage:
run de-ess without any options to see how to use it.

Design Philisophy
-----------------
This de-esser uses a novel approach called Temporal Sibilance Processing (if you have a better idea for naming this technique, or if it has been done before, please enlighten the author).

The idea is to distinguish between fricatives and voiced sections of the speech signal by the number of zero crossings in time.  When a fricative is identified, it is put into one of 3 classes:
1. Short fricatives are left untouched since, even if they are quite loud, they are needed to give crispness and clarity to speech.
2. Medium-length fricatives above threshold level 1 are filtered through a FIR filter with frequency response determined by eq1.cfg.
3. Long fricatives above threshold level 2 are filtered through a FIR filter with frequency response determined by eq2.cfg.

Most of the speech file is left untouched (the samples are directly copied from source to destination).  Only fricatives that are long enough and loud enough are filtered.  The advantage of this approach over traditional approaches is that the clarity of the remaining speech is completely unaffected.

The software could be modified to classify the fricatives into more classes with different processing for each class.  A number of parameters are hard-coded and should be made into configurable options.  The software also needs to automatically detect and process stereo files.

Contributors
------------
Thanks to Indrek Bauvald for contributing the following modifications:
1. Added an option for changing the duration of ess burst to ignore from command line.
2. Took sample rate from 24-28th byte of the input wav file.

