#!/usr/bin/env python

# The zlib/libpng License
# 
# pyjpgalleg is copyright (c) 2006 by Grzegorz Adam Hankiewicz
# 
# This software is provided 'as-is', without any express or implied  
# warranty. In no event will the author be held liable for any damages  
# arising from the use of this software.
# 
# Permission is granted to anyone to use this software for any purpose,  
# including commercial applications, and to alter it and redistribute it  
# freely, subject to the following restrictions:
# 
# 
# 1. The origin of this software must not be misrepresented; you must not  
# claim that you wrote the original software. If you use this software in 
# a product, an acknowledgment in the product documentation would be  
# appreciated but is not required.
# 
# 2. Altered source versions must be plainly marked as such, and must not 
# be  misrepresented as being the original software.
# 
# 3. This notice may not be removed or altered from any source distribution.

from distutils.core import setup, Extension

module = Extension(
    "pyjpgalleg",
    include_dirs = ["/usr/local/include"],
    libraries = ["jpgal"],
    library_dirs = ["/usr/local/lib"],
    sources = ["pyjpgallegmodule.c"],
    )

if __name__ == "__main__":
    setup(name = "pyjpgalleg",
        version = "1.0",
        description = "Simple jpgalleg wrapper",
        author = "Grzegorz Adam Hankiewicz",
        author_email = "gradha@titanium.sabren.com",
        ext_modules = [module]
        )
