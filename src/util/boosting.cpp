//
// Created by martin on 24/10/16.
//

#include <climits>
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include "../../include/canonization.h"
#include "../../include/util/packing.h"
#include "../../include/mcf.h"

using namespace boost::python;
using namespace mogli;

// ---- Python 2 / 3 shims -------------------------------------------------------------------
// py2 distinguishes int/long and str/unicode; py3 has one int type. On py3, an int that fits a C
// int is treated as the py2 "int" (stored as int in boost::any), larger ones as "long".
#if PY_MAJOR_VERSION >= 3
static inline bool mogli_is_int(PyObject* o) {
  if (!PyLong_Check(o) || PyBool_Check(o)) return false;
  int overflow = 0;
  long v = PyLong_AsLongAndOverflow(o, &overflow);
  if (v == -1 && PyErr_Occurred()) { PyErr_Clear(); return false; }
  return !overflow && v >= INT_MIN && v <= INT_MAX;
}
static inline bool mogli_is_str(PyObject* o) { return PyUnicode_Check(o); }
#else
static inline bool mogli_is_int(PyObject* o) { return PyInt_Check(o); }
static inline bool mogli_is_str(PyObject* o) { return PyString_Check(o); }
#endif

// Packed/hashed data is binary: return `bytes` (py2: str), never text.
struct Bytes { std::string s; explicit Bytes(const std::string &v) : s(v) {} };
struct BytesToPython {
  static PyObject* convert(Bytes const& b) { return PyBytes_FromStringAndSize(b.s.data(), b.s.size()); }
};
static Bytes py_pack_canonization(const Canonization &o) { return Bytes(pack_canonization(o)); }
static Bytes py_hash_canonization(const Canonization &o) { return Bytes(hash_canonization(o)); }
static Bytes py_pack_fcanonization(const FragmentCanonization &o) { return Bytes(pack_fcanonization(o)); }
static Bytes py_hash_fcanonization(const FragmentCanonization &o) { return Bytes(hash_fcanonization(o)); }
static Bytes py_pack_molecule1(const Molecule &o) { return Bytes(pack_molecule(o)); }
static Bytes py_pack_molecule2(const boost::shared_ptr<Molecule> &o) { return Bytes(pack_molecule(o)); }
static Bytes py_pack_fragment1(const Fragment &o) { return Bytes(pack_fragment(o)); }
static Bytes py_pack_fragment2(const boost::shared_ptr<Fragment> &o) { return Bytes(pack_fragment(o)); }
static Bytes py_pack_match(const Match &o) { return Bytes(pack_match(o)); }

inline boost::python::object pass_through(boost::python::object const& o) { return o; }

template<class Klass, class KlassIter>
struct iterator_wrappers {

  static Klass
  next(KlassIter& o) {
    Klass next = o;
    if (next == lemon::INVALID) {
      PyErr_SetString(PyExc_StopIteration, "No more data.");
      throw_error_already_set();
    }
    ++o;
    return next;
  }

  static void
  wrap(const char* python_name) {
    class_<KlassIter>(python_name)
        .def("next", next)
        .def("__next__", next)
        .def("__iter__", pass_through);
  }

};

BOOST_PYTHON_MODULE(libmogli) {
  to_python_converter<Bytes, BytesToPython>();

  struct AnyToPython {
    static PyObject* convert(boost::any const& obj) {
      if (obj.type() == typeid(bool))
        return incref(boost::python::object(boost::any_cast<bool>(obj)).ptr());
      else if (obj.type() == typeid(int))
        return incref(boost::python::object(boost::any_cast<int>(obj)).ptr());
      else if (obj.type() == typeid(long))
        return incref(boost::python::object(boost::any_cast<long>(obj)).ptr());
      else if (obj.type() == typeid(double))
        return incref(boost::python::object(boost::any_cast<double>(obj)).ptr());
      else if (obj.type() == typeid(std::string))
        return incref(boost::python::object(boost::any_cast<std::string>(obj)).ptr());
      // unsupported payload type: raise instead of falling off the end (was undefined behaviour)
      PyErr_SetString(PyExc_TypeError, "mogli: unsupported property type in boost::any");
      return NULL;
    }
  };

  struct AnyFromPython {
    AnyFromPython() {
      converter::registry::push_back(&convertible, &construct, type_id<boost::any>());
    }

    static void* convertible(PyObject* object_ptr) {
      if (PyBool_Check(object_ptr) || mogli_is_int(object_ptr) || PyLong_Check(object_ptr) || PyFloat_Check(object_ptr) || mogli_is_str(object_ptr))
        return object_ptr;
      else
        return 0;
    }

    static void construct(PyObject* obj_ptr, converter::rvalue_from_python_stage1_data* data) {
      assert(obj_ptr != 0);
      if (PyBool_Check(obj_ptr)) {
        bool value = extract<bool>(obj_ptr);
        void *storage = ((converter::rvalue_from_python_storage<boost::any> *) data)->storage.bytes;
        new(storage) boost::any(value);
        data->convertible = storage;
      } else if (mogli_is_int(obj_ptr)) {
        int value = extract<int>(obj_ptr);
        void *storage = ((converter::rvalue_from_python_storage<boost::any> *) data)->storage.bytes;
        new(storage) boost::any(value);
        data->convertible = storage;
      } else if (PyLong_Check(obj_ptr)) {
        long value = extract<long>(obj_ptr);
        void *storage = ((converter::rvalue_from_python_storage<boost::any> *) data)->storage.bytes;
        new(storage) boost::any(value);
        data->convertible = storage;
      } else if (PyFloat_Check(obj_ptr)) {
        double value = extract<double>(obj_ptr);
        void *storage = ((converter::rvalue_from_python_storage<boost::any> *) data)->storage.bytes;
        new(storage) boost::any(value);
        data->convertible = storage;
      } else if (mogli_is_str(obj_ptr)) {
        std::string value = extract<std::string>(obj_ptr);
        void *storage = ((converter::rvalue_from_python_storage<boost::any> *) data)->storage.bytes;
        new(storage) boost::any(value);
        data->convertible = storage;
      }
    }
  };

  // register the to-python converter
  to_python_converter<boost::any, AnyToPython>();
  // register the from-python converter
  AnyFromPython();

  def("pack_canonization", py_pack_canonization);
  def("unpack_canonization", unpack_canonization);
  def("hash_canonization", py_hash_canonization);
  def("pack_fcanonization", py_pack_fcanonization);
  def("unpack_fcanonization", unpack_fcanonization);
  def("hash_fcanonization", py_hash_fcanonization);
  def("pack_molecule", py_pack_molecule1);
  def("pack_molecule", py_pack_molecule2);
  def("unpack_molecule", unpack_molecule);
  def("pack_fragment", py_pack_fragment1);
  def("pack_fragment", py_pack_fragment2);
  def("unpack_fragment", unpack_fragment);
  def("pack_match", py_pack_match);
  def("unpack_match", unpack_match);

  class_<BoolVector>("BoolVector")
      .def("__iter__", iterator<BoolVector>())
      .def(vector_indexing_suite<BoolVector>());

  class_<ShortVector>("ShortVector")
      .def("__iter__", iterator<ShortVector>())
      .def(vector_indexing_suite<ShortVector>());

  class_<IntVector>("IntVector")
      .def("__iter__", iterator<IntVector>())
      .def(vector_indexing_suite<IntVector>());

  class_<LongVector>("LongVector")
      .def("__iter__", iterator<LongVector>())
      .def(vector_indexing_suite<LongVector>());

  class_<StringVector>("StringVector")
      .def("__iter__", iterator<StringVector>())
      .def(vector_indexing_suite<StringVector>());

  class_<NodeVector>("NodeVector")
      .def("__iter__", iterator<NodeVector>())
      .def(vector_indexing_suite<NodeVector>());

  // Vector iteration policies (py3 port):
  //  * MoleculeVector / FragmentVector hold boost::shared_ptr<T>. return_internal_reference<> on a
  //    shared_ptr<T>& wrapped the *shared_ptr object* in a Python instance, which no method taking
  //    `T&` accepts (ArgumentError ... {lvalue}). return_by_value converts the shared_ptr through
  //    the class's registered shared_ptr<T> holder: Python gets the SAME C++ object (reference
  //    semantics, lifetime shared via the shared_ptr), no copy.
  //  * MatchVector holds Match by value; return_internal_reference<> (custodian: the iterator, which
  //    keeps the vector alive) works there and is kept unchanged from the py2 build.
  typedef std::vector<boost::shared_ptr<Molecule> > MoleculeVector;
  class_<MoleculeVector, boost::noncopyable>("MoleculeVector")
      .def("__iter__", iterator<MoleculeVector, return_value_policy<return_by_value> >())
      .def("__len__", &MoleculeVector::size);

  class_<FragmentVector, boost::noncopyable>("FragmentVector")
      .def("__iter__", iterator<FragmentVector, return_value_policy<return_by_value> >())
      .def("__len__", &FragmentVector::size);

  class_<MatchVector, boost::noncopyable>("MatchVector")
      .def("__iter__", iterator<MatchVector, return_internal_reference<> >())
      .def("__len__", &MatchVector::size);

  class_<Canonization>("Canonization", init<const Molecule&>())
      .def("get_colors", &Canonization::get_colors, return_value_policy<return_by_value>())
      .def("get_canonization", &Canonization::get_canonization, return_value_policy<return_by_value>())
      .def("get_node_order", &Canonization::get_node_order, return_value_policy<return_by_value>())
      .def("is_isomorphic", &Canonization::is_isomorphic);

  class_<FragmentCanonization, bases<Canonization>>("FragmentCanonization", init<const Fragment&>())
      .def("get_core_nodes", &FragmentCanonization::get_core_nodes, return_value_policy<return_by_value>())
      .def("is_isomorphic", &FragmentCanonization::is_isomorphic);

  enum_<Product::GenerationType>("GenerationType")
      .value("NO_OPT", Product::GenerationType::NO_OPT)
      .value("UNCON", Product::GenerationType::UNCON)
      .value("DEG_1", Product::GenerationType::DEG_1)
      .value("UNCON_DEG_1", Product::GenerationType::UNCON_DEG_1);

  const Node (Molecule::*add_atom1)(std::string) = &Molecule::add_atom;
  const Node (Molecule::*add_atom2)(int, std::string) = &Molecule::add_atom;
  const Node (Molecule::*add_atom3)(unsigned short) = &Molecule::add_atom;
  const Node (Molecule::*add_atom4)(int, unsigned short) = &Molecule::add_atom;

  std::string (Molecule::*write_lgf1)() = &Molecule::write_lgf;
  std::string (Molecule::*write_lgf2)(const LGFIOConfig&) = &Molecule::write_lgf;

  void (Molecule::*read_lgf1)(const std::string &) = &Molecule::read_lgf;
  void (Molecule::*read_lgf2)(const std::string &, const LGFIOConfig&) = &Molecule::read_lgf;

  const std::string (Molecule::*print_dot1)() const = &Molecule::print_dot;
  const std::string (Molecule::*print_dot2)(const StringVector&) const = &Molecule::print_dot;
  const std::string (Fragment::*print_dot3)() const = &Fragment::print_dot;

  class_<PeriodicTable, boost::noncopyable>("PeriodicTable", init<>())
      .def("add_uncolored", &PeriodicTable::add_uncolored, return_internal_reference<>())
      .def("add", &PeriodicTable::add, return_internal_reference<>());

  class_<LGFIOConfig, boost::noncopyable>("LGFIOConfig", init<std::string, std::string>())
      .def("add_bool_node_prop", &LGFIOConfig::add_bool_node_prop, return_internal_reference<>())
      .def("add_int_node_prop", &LGFIOConfig::add_int_node_prop, return_internal_reference<>())
      .def("add_double_node_prop", &LGFIOConfig::add_double_node_prop, return_internal_reference<>())
      .def("add_string_node_prop", &LGFIOConfig::add_string_node_prop, return_internal_reference<>());

  class_<Molecule, boost::shared_ptr<Molecule>, boost::noncopyable>("Molecule", init<>())
      .def(init<PeriodicTable&>())
      .def("add_atom", add_atom1)
      .def("add_atom", add_atom2)
      .def("add_atom", add_atom3)
      .def("add_atom", add_atom4)
      .def("add_edge", &Molecule::add_edge)
      .def("get_node_iter", &Molecule::get_node_iter)
      .def("get_edge_iter", &Molecule::get_edge_iter)
      .def("get_inc_edge_iter", &Molecule::get_inc_edge_iter)
      .def("get_opposite_node", &Molecule::get_opposite_node)
      .def("get_u", &Molecule::get_u)
      .def("get_v", &Molecule::get_v)
      .def("get_atom_count", &Molecule::get_atom_count)
      .def("has_node_with_id", &Molecule::has_node_with_id)
      .def("get_node_by_id", &Molecule::get_node_by_id)
      .def("get_id", &Molecule::get_id)
      .def("get_color", &Molecule::get_color)
      .def("get_element", &Molecule::get_element)
      .def("is_connected", &Molecule::is_connected)
      .def("get_connected_components", &Molecule::get_connected_components)
      .def("split", &Molecule::split)
      .def("is_isomorphic", &Molecule::is_isomorphic)
      .def("write_lgf", write_lgf1)
      .def("write_lgf", write_lgf2)
      .def("read_lgf", read_lgf1)
      .def("read_lgf", read_lgf2)
      .def("print_dot", print_dot1)
      .def("print_dot", print_dot2)
      .def("set_property", &Molecule::set_property)
      .def("get_property", &Molecule::get_property);

  class_<Fragment, boost::shared_ptr<Fragment>, bases<Molecule>, boost::noncopyable>("Fragment", init<>())
      .def("get_core_atom_count", &Fragment::get_core_atom_count)
      .def("is_core", &Fragment::is_core)
      .def("print_dot", print_dot3);

  class_<Match, boost::shared_ptr<Match>>("Match", init<>())
      .def("frag_to_mol", &Match::frag_to_mol)
      .def("merged_frag_to_mol", &Match::merged_frag_to_mol)
      .def("get_atom_ids", &Match::get_atom_ids)
      .def("map_ids", &Match::map_ids);

  void (*mcf1)(Molecule&, Molecule&, FragmentVector&, MatchVector&, MatchVector&,
               int, unsigned int, unsigned int, Product::GenerationType, bool, bool, int) = &maximal_common_fragments;
  void (*mcf2)(Molecule&, Molecule&, FragmentVector&, MatchVector&, MatchVector&,
               int, unsigned int, Product::GenerationType, bool, bool, int) = &maximal_common_fragments;

  def("maximal_common_fragments", mcf1);
  def("maximal_common_fragments", mcf2);
  def("atomic_fragments", atomic_fragments);

  class_<Node>("Node")
      .def(self == self)
      .def(self != self)
      .def(self < self);

  class_<Edge>("Edge")
      .def(self == self)
      .def(self != self)
      .def(self < self);

  iterator_wrappers<const Node, NodeIt>().wrap("NodeIt");
  iterator_wrappers<const Edge, EdgeIt>().wrap("EdgeIt");
  iterator_wrappers<const Edge, IncEdgeIt>().wrap("IncEdgeIt");

}
