/*
 *  fileio.c
 *
 *  Load a normal file, or ZIP/GZ archive into ROM buffer.
 *  Returns loaded ROM size (zero if an error occured)
 *  
 *
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003  Charles Mac Donald
 *  modified by Eke-Eke (Genesis Plus GX)
 *
 *  Redistribution and use of this code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *   - Redistributions may not be sold, nor may they be used in a commercial
 *     product or activity.
 *
 *   - Redistributions that are modified from the original source must include the
 *     complete source code, including the source code for all components used by a
 *     binary built from the modified sources. However, as a special exception, the
 *     source code distributed need not include anything that is normally distributed
 *     (in either source or binary form) with the major components (compiler, kernel,
 *     and so on) of the operating system on which the executable runs, unless that
 *     component itself accompanies the executable.
 *
 *   - Redistributions must reproduce the above copyright notice, this list of
 *     conditions and the following disclaimer in the documentation and/or other
 *     materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************************/

#include "shared.h"

int load_archive(char *filename, unsigned char *buffer, int maxsize, char *extension)
{
  if (extension) {
    extension[0] = 0;
  }

  FILE* fd = fopen(filename, "rb");

  if (!fd) {
    return 0;
  }

  // Retrieve the file size.
  if (fseek(fd, 0, SEEK_END) != 0) {
    fclose(fd);

    return 0;
  }

  int size = ftell(fd);

  if (size > maxsize) {
    fclose(fd);

    return 0;
  }

  // Fill the buffer.
  if (fseek(fd, 0, SEEK_SET) != 0) {
    fclose(fd);

    return 0;
  }

  if (fread(buffer, sizeof(unsigned char), size, fd) != size) {
    fclose(fd);

    return 0;
  }

  fclose(fd);

  // Initialize the extension.
  if (extension) {
    strncpy(extension, &filename[strlen(filename) - 3], 3);
    extension[3] = 0;
  }

  return size;
}
