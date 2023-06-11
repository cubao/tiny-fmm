#include <pybind11/eigen.h>
#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "network/network.hpp"

namespace tinyfmm
{
namespace py = pybind11;
using namespace pybind11::literals;
using rvp = py::return_value_policy;

void bind_tinyfmm_network(py::module &m)
{
    py::class_<FMM::NETWORK::Network>(m, "Network", py::module_local())      //
        .def(py::init<>())
        //
        ;

}
} // namespace cubao
