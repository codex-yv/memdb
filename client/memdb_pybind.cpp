/*
* creating python binding so that this feature can be used in python

* demo usages:

import memdb

db = memdb.MemDB("127.0.0.1", 7379)

db.set("name", "Youraj")

print(db.get("name"))

print(db.size())

print(db.ping())

db.delete("name")


*/


#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "memdb.h"

namespace py = pybind11;

PYBIND11_MODULE(memdb, m) {

    m.doc() = "MemDB Python bindings";

    // Exception binding
    py::register_exception<MemDBException>(m, "MemDBException");

    // MemDB class binding
    py::class_<MemDB>(m, "MemDB")

        .def(
            py::init<const std::string&, int>(),
            py::arg("host") = "127.0.0.1",
            py::arg("port") = 7379
        )

        .def(
            "set",
            &MemDB::set,
            py::arg("key"),
            py::arg("value")
        )

        .def(
            "get",
            (std::string (MemDB::*)(const std::string&))
            &MemDB::get,
            py::arg("key")
        )

        .def(
            "delete",
            &MemDB::del,
            py::arg("key")
        )

        .def(
            "ping",
            &MemDB::ping
        )

        .def(
            "size",
            &MemDB::size
        )

        .def(
            "reconnect",
            &MemDB::reconnect
        );
}