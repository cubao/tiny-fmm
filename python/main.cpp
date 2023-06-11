#include <pybind11/pybind11.h>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

int add(int i, int j) { return i + j; }

namespace py = pybind11;

namespace cubao
{
void bind_crs_transform(py::module &m);
void bind_fast_crossing(py::module &m);
void bind_flatbush(py::module &m);
void bind_polyline_ruler(py::module &m);
} // namespace cubao

PYBIND11_MODULE(pybind11_tiny_fmm, m)
{
    m.def("add", &add, R"pbdoc(
        Add two numbers

        Some other explanation about the add function.
    )pbdoc");

    m.def(
        "subtract", [](int i, int j) { return i - j; }, R"pbdoc(
        Subtract two numbers

        Some other explanation about the subtract function.
    )pbdoc");

    cubao::bind_crs_transform(m);
    cubao::bind_fast_crossing(m);
    cubao::bind_flatbush(m);
    cubao::bind_polyline_ruler(m);

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
