from setuptools import setup, Extension

# Define the extension module
hash_extension = Extension(
    'hash_extension', sources=['hash_extension.cpp'],
    extra_compile_args=['-std=c++11'],  # Use C++11 standard
)

# Setup script
setup(
    name='hash_extension',
    version='1.0',
    description='Python interface for C++ std::hash function',
    ext_modules=[hash_extension],
)
