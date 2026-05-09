from setuptools import setup, Extension
import pybind11

ext_modules = [
    Extension(
        "memdb",
        [
            "client/memdb.cpp",
            "client/memdb_pybind.cpp"
        ],
        include_dirs=[
            pybind11.get_include(),
            "client"
        ],
        language="c++",
        extra_compile_args=["-std=c++17"],
        libraries=["ws2_32"],
    ),
]

setup(
    name="memdb",
    version="1.0",
    ext_modules=ext_modules,
)