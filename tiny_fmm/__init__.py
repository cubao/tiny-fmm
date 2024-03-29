from pybind11_tiny_fmm import *  # noqa: F403
from pybind11_tiny_fmm import __version__  # noqa: F401


def pure_python_func(
    arg1: int = 42,
    arg2: float = 3.14,
    arg3: str = "you shall not pass",
) -> str:
    return f"int: {arg1}, float: {arg2}, str: {arg3}"
