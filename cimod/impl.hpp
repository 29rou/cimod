//    Copyright 2021 Jij Inc.

//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at

//        http://www.apache.org/licenses/LICENSE-2.0

//    Unless required by applicable law or agreed to in writing, software 
//    distributed under the License is distributed on an "AS IS" BASIS, 
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

#include "../../src/disable_eigen_warning.hpp"

#include <pybind11_json/pybind11_json.hpp>
#include <nlohmann/json.hpp>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>

#include <binary_quadratic_model.hpp>
#include <binary_quadratic_model_dict.hpp>
#include <binary_polynomial_model.hpp>

#include <sstream>

namespace py = pybind11;

using namespace py::literals;
using namespace cimod;

template<typename IndexType, typename FloatType, typename DataType>
inline void declare_BQM(py::module& m, const std::string& name){

    using BQM = BinaryQuadraticModel<IndexType, FloatType, DataType>;

    using DenseMatrix = Eigen::Matrix<FloatType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

    auto pyclass_BQM = py::class_<BQM>(m, name.c_str());

    pyclass_BQM
        .def(py::init<Linear<IndexType, FloatType>, Quadratic<IndexType, FloatType>, FloatType, Vartype>(), "linear"_a, "quadratic"_a, "offset"_a, "vartype"_a)
        .def(py::init<Linear<IndexType, FloatType>, Quadratic<IndexType, FloatType>, Vartype>(), "linear"_a, "quadratic"_a, "vartype"_a)
        .def(py::init<Eigen::Ref<const DenseMatrix>, std::vector<IndexType>, FloatType, Vartype, bool>(), "mat"_a, "labels_vec"_a, "offset"_a, "vartype"_a, "fix_format"_a=true)
        .def(py::init<Eigen::Ref<const DenseMatrix>, std::vector<IndexType>, Vartype, bool>(), "mat"_a, "labels_vec"_a, "vartype"_a, "fix_format"_a=false)
        .def(py::init<const BQM&>(), "bqm"_a)
        .def("length", &BQM::length)
        .def("get_num_variables", &BQM::get_num_variables)
        .def("get_linear", py::overload_cast<IndexType>(&BQM::get_linear, py::const_))
        .def("get_linear", py::overload_cast<>(&BQM::get_linear, py::const_))
        .def("get_quadratic", py::overload_cast<IndexType ,IndexType>(&BQM::get_quadratic, py::const_))
        .def("get_quadratic", py::overload_cast<>(&BQM::get_quadratic, py::const_))
        .def("get_offset", &BQM::get_offset)
        .def("get_vartype", &BQM::get_vartype)
        .def("get_variables", &BQM::get_variables)
        //.def("print", &BQM::print)
        .def("empty", &BQM::empty, "vartype"_a)
        .def("add_variable", &BQM::add_variable, "v"_a, "bias"_a)
        .def("add_variables_from", &BQM::add_variables_from, "linear"_a)
        .def("add_interaction", &BQM::add_interaction, "u"_a, "v"_a, "bias"_a)
        .def("add_interactions_from", &BQM::add_interactions_from, "quadratic"_a)
        .def("remove_variable", &BQM::remove_variable, "v"_a)
        .def("remove_variables_from", &BQM::remove_variables_from, "variables"_a)
        .def("remove_interaction", &BQM::remove_interaction, "u"_a, "v"_a)
        .def("remove_interactions_from", &BQM::remove_interactions_from, "interactions"_a)
        .def("add_offset", &BQM::add_offset, "offset"_a)
        .def("remove_offset", &BQM::remove_offset)
        .def("scale", &BQM::scale, "scalar"_a, "ignored_variables"_a=std::vector<IndexType>(), "ignored_interactions"_a=std::vector<std::pair<IndexType, IndexType>>(), "ignored_offset"_a=false)
        .def("normalize", &BQM::normalize, "bias_range"_a=std::pair<FloatType, FloatType>(1.0, 1.0), "use_quadratic_range"_a=false, "quadratic_range"_a=std::pair<FloatType, FloatType>(1.0, 1.0), "ignored_variables"_a=std::vector<IndexType>(), "ignored_interactions"_a=std::vector<std::pair<IndexType, IndexType>>(), "ignored_offset"_a=false)
        .def("fix_variable", &BQM::fix_variable, "v"_a, "value"_a)
        .def("fix_variables", &BQM::fix_variables, "fixed"_a)
        .def("flip_variable", &BQM::flip_variable, "v"_a)
        //.def("contract_variables", &BQM::contract_variables, "u"_a, "v"_a)
        .def("change_vartype", py::overload_cast<const Vartype&>(&BQM::change_vartype), "vartype"_a)
        .def("change_vartype", py::overload_cast<const Vartype&, bool>(&BQM::change_vartype), "vartype"_a, "inplace"_a)
        .def("energy", &BQM::energy, "sample"_a)
        .def("energies", &BQM::energies, "samples_like"_a)
        .def("to_qubo", &BQM::to_qubo)
        .def("to_ising", &BQM::to_ising)
        .def_static("from_qubo", &BQM::from_qubo, "Q"_a, "offset"_a=0.0)
        .def_static("from_ising", &BQM::from_ising, "h"_a, "J"_a, "offset"_a=0.0)
        .def("interaction_matrix", py::overload_cast<>(&BQM::interaction_matrix, py::const_))
        //.def("to_serialiable", &BQM::to_serializable)
        //.def_static("from_serialiable", &BQM::from_serializable, "input"_a);
        .def("to_serializable", [](const BQM& self){return static_cast<py::object>(self.to_serializable());})
        .def_static("from_serializable", [](const py::object& input){return BQM::from_serializable(static_cast<nlohmann::json>(input));}, "input"_a);

    //interaction_matrix for Dict (legacy BQM) class
    if constexpr (std::is_same_v<DataType, cimod::Dict>)
        pyclass_BQM
            .def("_generate_indices", &BQM::_generate_indices)
            .def("interaction_matrix", py::overload_cast<const std::vector<IndexType>&>(&BQM::interaction_matrix, py::const_));
            

}


template<typename IndexType, typename FloatType>
inline void declare_BPM(py::module& m, const std::string& name){
   
   using BPM = BinaryPolynomialModel<IndexType, FloatType>;
   
   py::class_<BPM>(m, name.c_str())
   .def(py::init<Polynomial<IndexType, FloatType>&, const Vartype>(), "polynomial"_a, "vartype"_a)
   .def(py::init<PolynomialKeyList<IndexType>&, PolynomialValueList<FloatType>&, const Vartype>(), "keys"_a, "values"_a, "vartype"_a)
   .def(py::init<const std::vector<IndexType>&, const PolynomialKeyList<std::size_t>&, const PolynomialValueList<FloatType>&, const Vartype>(),
        "variables"_a, "keys_distance"_a, "values"_a, "vartype"_a)
   .def("get_polynomial"          , [](const BPM& self) {
      py::dict py_polynomial;
      const auto &poly_key_list   = self._get_keys();
      const auto &poly_value_list = self._get_values();
      for (std::size_t i = 0; i < poly_key_list.size(); ++i) {
         py::tuple tuple;
         for (const auto &index: poly_key_list[i]) {
            tuple = tuple + py::make_tuple(index);
         }
         py_polynomial[tuple] = poly_value_list[i];
      }
      return py_polynomial;
   })
   .def("get_polynomial"          , py::overload_cast<std::vector<IndexType>&>(&BPM::get_polynomial, py::const_), "key"_a)
   .def("get_variables_to_integers",py::overload_cast<>(&BPM::get_variables_to_integers))
   .def("get_variables_to_integers",py::overload_cast<const IndexType&>(&BPM::get_variables_to_integers), "v"_a)
   .def("_get_keys"               , &BPM::_get_keys)
   .def("_get_values"             , &BPM::_get_values)
   .def("get_variables"           , py::overload_cast<>(&BPM::get_sorted_variables))
   .def("indices"                 , &BPM::indices)
   .def("get_degree"              , &BPM::get_degree)
   .def("get_offset"              , &BPM::get_offset)
   .def("get_vartype"             , &BPM::get_vartype)
   .def("get_num_interactions"    , &BPM::get_num_interactions)
   .def("get_num_variables"       , &BPM::get_num_variables)
   .def("empty"                   , &BPM::empty, "vartype"_a)
   .def("clear"                   , &BPM::clear)
   .def("remove_interaction"      , py::overload_cast<std::vector<IndexType>&>(&BPM::remove_interaction), "key"_a)
   .def("remove_interactions_from", py::overload_cast<PolynomialKeyList<IndexType>&>(&BPM::remove_interactions_from), "keys"_a)
   .def("remove_offset"           , &BPM::remove_offset)
   .def("remove_variable"         , &BPM::remove_variable, "v"_a)
   .def("remove_variables_from"   , &BPM::remove_variables_from, "variables"_a)
   .def("add_interaction"         , py::overload_cast<std::vector<IndexType>&, const FloatType&, const Vartype>(&BPM::add_interaction), "key"_a, "value"_a, "vartype"_a = Vartype::NONE)
   .def("add_interactions_from"   , py::overload_cast<PolynomialKeyList<IndexType>&, const PolynomialValueList<FloatType>&, const Vartype>(&BPM::add_interactions_from), "keys"_a, "values"_a, "vartype"_a = Vartype::NONE)
   .def("add_interactions_from"   , py::overload_cast<const Polynomial<IndexType, FloatType>&, const Vartype>(&BPM::add_interactions_from), "polynomial"_a, "vartype"_a = Vartype::NONE)
   .def("add_offset"              , &BPM::add_offset, "offset"_a)
   .def("energy"                  , py::overload_cast<const Sample<IndexType>&, bool>(&BPM::energy, py::const_), "sample"_a, "omp_flag"_a = true)
   .def("energy"                  , py::overload_cast<const std::vector<int32_t>&, bool>(&BPM::energy), "sample"_a, "omp_flag"_a = true)
   .def("energies"                , py::overload_cast<const std::vector<Sample<IndexType>>&>(&BPM::energies, py::const_), "samples"_a)
   .def("energies"                , py::overload_cast<const std::vector<std::vector<int32_t>>&>(&BPM::energies), "samples"_a)
   .def("scale"                   , &BPM::scale, "scalar"_a, "ignored_interactions"_a = PolynomialKeyList<IndexType>{}, "ignored_offset"_a = false)
   .def("normalize"               , &BPM::normalize, "range"_a = std::pair<FloatType, FloatType>{1.0, 1.0}, "ignored_interactions"_a = PolynomialKeyList<IndexType>{}, "ignored_offset"_a = false)
   .def("change_vartype"          , py::overload_cast<const Vartype, const bool>(&BPM::change_vartype), "vartype"_a, "inplace"_a)
   .def("change_vartype"          , py::overload_cast<const Vartype>(&BPM::change_vartype), "vartype"_a)
   .def("has_variable"            , &BPM::has_variable, "v"_a)
   .def("to_hubo"                 , [](const BPM& self){
      py::dict py_polynomial;
      for (const auto &it: self.to_hubo()) {
         py::tuple tuple;
         for (const auto &index: it.first) {
            tuple = tuple + py::make_tuple(index);
         }
         py_polynomial[tuple] = it.second;
      }
      return py_polynomial;
   })
   .def("to_hising"               , [](const BPM& self) {
      py::dict py_polynomial;
      for (const auto &it: self.to_hising()) {
         py::tuple tuple;
         for (const auto &index: it.first) {
            tuple = tuple + py::make_tuple(index);
         }
         py_polynomial[tuple] = it.second;
      }
      return py_polynomial;
   })
   .def("to_serializable"         , [](const BPM& self){return static_cast<py::object>(self.to_serializable());})
   .def_static("from_serializable", [](const py::object& input){return BPM::from_serializable(static_cast<nlohmann::json>(input));}, "input"_a)
   .def_static("from_hubo"        , py::overload_cast<const Polynomial<IndexType, FloatType>&>(&BPM::from_hubo), "polynomial"_a)
   .def_static("from_hubo"        , py::overload_cast<PolynomialKeyList<IndexType>&, const PolynomialValueList<FloatType>&>(&BPM::from_hubo), "keys"_a, "value"_a)
   .def_static("from_hising"      , py::overload_cast<const Polynomial<IndexType, FloatType>&>(&BPM::from_hising), "polynomial"_a)
   .def_static("from_hising"      , py::overload_cast<PolynomialKeyList<IndexType>&, const PolynomialValueList<FloatType>&>(&BPM::from_hising), "keys"_a, "value"_a)
   .def("__repr__", [](const BPM& self) {
      const auto &poly_key_list   = self._get_keys();
      const auto &poly_value_list = self._get_values();
      std::ostringstream out;
      out << "cxxcimod.BinaryPolynomialModel({";
      for (std::size_t i = 0; i < poly_key_list.size(); ++i) {
         py::tuple tuple;
         for (const auto &it: poly_key_list[i]) {
            tuple = tuple + py::make_tuple(it);
         }
         out << tuple.attr("__repr__")();
         if (i == poly_key_list.size() - 1) {
            out << ": " << poly_value_list[i];
         }
         else {
            out << ": " << poly_value_list[i] << ", ";
         }
      }
      out << "}, ";
      if (self.get_vartype() == Vartype::SPIN) {
         out << "Vartype.SPIN" << ")";
      }
      else if (self.get_vartype() == Vartype::BINARY){
         out << "Vartype.BINARY" << ")";
      }
      else {
         out << "Vartype.NONE" << ")";
      }
      return out.str();
   });
   
   
}
