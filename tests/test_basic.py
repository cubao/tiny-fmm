import time

import numpy as np

import tiny_fmm as m
from tiny_fmm import FastCrossing, LineSegment, intersect_segments, tf
from tiny_fmm import Network

"""
some tests migrated from

-   https://polyline-ruler.readthedocs.io/en/latest/usage/
-   https://fast-crossing.readthedocs.io/en/latest/usage/
"""


def test_main():
    assert m.__version__ == "0.0.1"
    assert m.add(1, 2) == 3
    assert m.subtract(1, 2) == -1


def test_crs():
    lla = [123, 45, 6]
    ecef1 = tf.lla2ecef(lla)
    assert ecef1.shape == (1, 3)
    ecef2 = tf.lla2ecef(*lla)
    assert ecef2.shape == (3,)
    assert np.all(ecef2 == ecef1[0])

    enus = np.random.random((100, 3)) * 1000
    t0 = time.time()
    tf.enu2lla(enus, anchor_lla=lla)
    t1 = time.time()
    tf.enu2lla(enus, anchor_lla=lla, cheap_ruler=False)
    t2 = time.time()
    assert t2 - t1 > t1 - t0


def test_segment():
    seg = LineSegment([0, 0, 0], [10, 0, 0])
    assert 4.0 == seg.distance([5.0, 4.0, 0.0])
    assert 5.0 == seg.distance([-4.0, 3.0, 0.0])
    assert 5.0 == seg.distance([14.0, 3.0, 0.0])

    assert np.all(seg.A == [0, 0, 0])
    assert np.all(seg.B == [10, 0, 0])
    assert np.all(seg.AB == [10, 0, 0])
    assert seg.length == 10.0
    assert seg.length2 == 100.0

    seg = LineSegment([0, 0, 0], [0, 0, 0])
    assert 5.0 == seg.distance([3.0, 4.0, 0.0])
    assert 5.0 == seg.distance([-4.0, 3.0, 0.0])
    assert 13.0 == seg.distance([5.0, 12.0, 0.0])


def test_intersections():
    pt, t, s = intersect_segments([-1, 0], [1, 0], [0, -1], [0, 1])
    assert np.all(pt == [0, 0])
    assert t == 0.5
    assert s == 0.5
    pt, t, s = intersect_segments([-1, 0], [1, 0], [0, -1], [0, 3])
    assert np.all(pt == [0, 0])
    assert t == 0.5
    assert s == 0.25

    pt, t, s, _ = intersect_segments([-1, 0, 0], [1, 0, 20], [0, -1, -100], [0, 3, 300])
    assert np.all(pt == [0, 0, 5.0])
    assert t == 0.5
    assert s == 0.25

    seg1 = LineSegment([-1, 0, 0], [1, 0, 20])
    seg2 = LineSegment([0, -1, -100], [0, 3, 300])
    pt2, t2, s2, _ = seg1.intersects(seg2)
    assert np.all(pt == pt2) and t == t2 and s == s2

    A = [[-1, 0, 10], [1, 0, 10]]
    B = [[0, -1, 20], [0, 1, 20]]
    pt, t, s, half_span = LineSegment(*A).intersects(LineSegment(*B))
    assert np.all(pt == [0, 0, 15]) and t == 0.5 and s == 0.5 and half_span == 5.0
    pt, t, s, half_span = LineSegment(*B).intersects(LineSegment(*A))
    assert np.all(pt == [0, 0, 15]) and t == 0.5 and s == 0.5 and half_span == -5.0


def __sample_fc():
    fc = FastCrossing()
    # add your polylines
    """
                    2 C
                    |
                    1 D
    0               |                  5
    A---------------o------------------B
                    |
                    |
                    -2 E
    """
    fc.add_polyline(np.array([[0.0, 0.0], [5.0, 0.0]]))  # AB
    fc.add_polyline(np.array([[2.5, 2.0], [2.5, 1.0], [2.5, -2.0]]))  # CDE
    # build index
    fc.finish()
    return fc


def test_fast_crossing():
    fc = __sample_fc()

    # num_poylines
    assert 2 == fc.num_poylines()
    rulers = fc.polyline_rulers()
    assert len(rulers) == 2
    ruler0 = fc.polyline_ruler(0)
    ruler1 = fc.polyline_ruler(1)
    assert not ruler0.is_wgs84()
    assert not ruler1.is_wgs84()
    assert ruler0.length() == 5
    assert ruler1.length() == 4
    assert fc.polyline_ruler(10) is None

    # query all line segment intersections
    # [
    #    (array([2.5, 0. ]),
    #     array([0.5       , 0.33333333]),
    #     array([0, 0], dtype=int32),
    #     array([1, 1], dtype=int32))
    # ]
    ret = fc.intersections()
    # print(ret)
    assert len(ret) == 1
    for xy, ts, label1, label2 in ret:
        assert np.all(xy == [2.5, 0])
        assert np.all(ts == [0.5, 1 / 3.0])
        assert np.all(label1 == [0, 0])
        assert np.all(label2 == [1, 1])

    # query intersections against provided polyline
    polyline = np.array([[-6.0, -1.0], [-5.0, 1.0], [5.0, -1.0]])
    ret = fc.intersections(polyline)
    ret = np.array(ret)
    xy = ret[:, 0]
    ts = ret[:, 1]
    label1 = ret[:, 2]
    label2 = ret[:, 3]
    assert np.all(xy[0] == [0, 0])
    assert np.all(xy[1] == [2.5, -0.5])
    assert np.all(ts[0] == [0.5, 0])
    assert np.all(ts[1] == [0.75, 0.5])
    assert np.all(label1 == [[0, 1], [0, 1]])
    assert np.all(label2 == [[0, 0], [1, 1]])

    polyline2 = np.column_stack((polyline, np.zeros(len(polyline))))
    ret2 = np.array(fc.intersections(polyline2[:, :2]))
    assert str(ret) == str(ret2)


def test_flatbush():
    fc = __sample_fc()
    bush = fc.bush()
    hits = bush.search(min=[4, -1], max=[6, 1])
    for h in hits:
        bbox = bush.box(h)
        label = bush.label(h)
        print(h, bbox, label)
    assert len(hits) == 1 and np.all(bush.label(hits[0]) == [0, 0])
    hits = bush.search(min=[2, -3], max=[3, -1])
    for h in hits:
        bbox = bush.box(h)
        label = bush.label(h)
        print(h, bbox, label)
    assert len(hits) == 1 and np.all(bush.label(hits[0]) == [1, 1])

    hits = bush.search(min=[-10, -10], max=[10, 10])
    assert len(hits) == 3
    hits = bush.search(min=[10, 10], max=[-10, -10])
    assert len(hits) == 0


def test_tinyfmm_network():
    n = Network()
    print(n)