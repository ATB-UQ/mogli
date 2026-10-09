"""Smoke tests for the py3 Boost.Python build of mogli (ee6872e), imported as `libmogli`."""
import gc
import pickle

import pytest

m = pytest.importorskip("libmogli")


def mol(atoms, bonds):
    mo = m.Molecule()
    ns = [mo.add_atom(i, e) for i, e in enumerate(atoms)]
    for a, b in bonds:
        mo.add_edge(ns[a], ns[b])
    return mo


@pytest.fixture
def eth():
    return mol("CCO", [(0, 1), (1, 2)])


@pytest.fixture
def pro():
    return mol("CCCO", [(0, 1), (1, 2), (2, 3)])


def mcf(a, b):
    frags, m1, m2 = m.FragmentVector(), m.MatchVector(), m.MatchVector()
    # shell=0, min_core_size=1, UNCON, reduce_subgraphs, maximum, timeout 10 s
    m.maximal_common_fragments(a, b, frags, m1, m2, 0, 1, m.GenerationType.UNCON, True, True, 10)
    return frags, m1, m2


def test_basic(eth, pro):
    assert (eth.get_atom_count(), pro.get_atom_count()) == (3, 4)
    assert eth.is_connected()
    assert [eth.get_element(n) for n in eth.get_node_iter()]  # py3 __next__ on node iterator


def test_isomorphism(eth, pro):
    assert eth.is_isomorphic(mol("OCC", [(0, 1), (1, 2)]))
    assert not eth.is_isomorphic(pro)


def test_mcf_and_vector_iteration(eth, pro):
    frags, m1, m2 = mcf(eth, pro)
    assert len(frags) == len(m1) == len(m2) == 1
    # elements yielded by iteration must be usable as arguments (the return_internal_reference problem)
    for f, a, b in zip(frags, m1, m2):
        assert f.get_core_atom_count() == 3
        i1, i2 = m.IntVector(), m.IntVector()
        a.get_atom_ids(i1)
        b.get_atom_ids(i2)
        assert sorted(i1) == [0, 1, 2]
        assert sorted(i2) == [1, 2, 3]
        assert isinstance(m.pack_match(a), bytes)
        assert isinstance(m.pack_fragment(f), bytes)


def test_match_outlives_vector(eth, pro):
    _, m1, _ = mcf(eth, pro)
    x = next(iter(m1))
    del m1
    gc.collect()
    ids = m.IntVector()
    x.get_atom_ids(ids)
    assert len(ids) == 3


def test_pack_unpack_molecule(pro):
    s = m.pack_molecule(pro)
    assert isinstance(s, bytes)
    assert len(s) == 26
    p2 = m.Molecule()
    m.unpack_molecule(s, p2)
    assert p2.get_atom_count() == 4
    assert pro.is_isomorphic(p2)
    assert sorted(p2.get_element(n) for n in p2.get_node_iter()) == ["C", "C", "C", "O"]
    # NOTE: re-packing after unpack is NOT byte-identical (unpack reverses node order, lemon list
    # semantics); whether that matches the py2 build is a G1 parity question, so only check size.
    assert len(m.pack_molecule(p2)) == len(s)


def test_canonization_pack_hash(pro):
    c = m.Canonization(pro)
    packed = m.pack_canonization(c)
    assert isinstance(packed, bytes) and len(packed) == 48
    h = m.hash_canonization(c)
    assert isinstance(h, bytes) and h
    assert m.hash_canonization(m.Canonization(pro)) == h  # deterministic
    # NOTE: hash differs for the same graph built in another atom order (observed); G1 must compare
    # that behaviour against the py2 build rather than assume invariance.


def test_fragment_roundtrip(eth, pro):
    frags, _, _ = mcf(eth, pro)
    f = next(iter(frags))
    f2 = m.Fragment()
    m.unpack_fragment(m.pack_fragment(f), f2)
    assert f2.get_core_atom_count() == f.get_core_atom_count() == 3
    assert len(m.pack_fragment(f2)) == len(m.pack_fragment(f))  # byte equality: see G1 note above


def test_pickle_unsupported(pro):
    with pytest.raises(Exception):
        pickle.dumps(pro)
