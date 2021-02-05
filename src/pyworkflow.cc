#include <pybind11/pybind11.h>

namespace py = pybind11;
void init_common_types(py::module_&);
void init_network_types(py::module_&);
void init_other_types(py::module_&);

PYBIND11_MODULE(cpp_pyworkflow, wf) {
    wf.doc() = "python3 binding for workflow";

    init_common_types(wf);
    init_network_types(wf);
    init_other_types(wf);
}
