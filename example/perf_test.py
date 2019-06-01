import math
import time
import argparse
import numpy as np
import tensorflow as tf
from nvzmq_ops import ZmqOp
from tensorcom.zcom import Statistics
from math import inf


parser = argparse.ArgumentParser(description='Performance test')
parser.add_argument("-addr", "--address", type=str, default="zsub://127.0.0.1:7880")
parser.add_argument("-n", "--num", type=int, default=100)

args = parser.parse_args()
num = args.num    
class Stats(object):
    def __init__(self):
        self.lo = inf
        self.hi = -inf
        self.sx = 0
        self.sx2 = 0
        self.n = 0

    def __iadd__(self, x):
        self.lo = min(self.lo, np.amin(x))
        self.hi = max(self.hi, np.amax(x))
        self.sx += np.sum(x)
        self.sx2 += np.sum(x**2)
        self.n += x.size
        return self

    def summary(self):
        return "[{:.3g} {:.3g}] mean={:.3g} std={:.3g} n={:d}".format(
            self.lo, self.hi,
            self.sx/self.n,
            (self.sx2/self.n - (self.sx/self.n)**2)**.5,
            self.n
        )


def tf_client():
    zmq_op = ZmqOp(address=args.address, zmq_hwm=100)
    types = [tf.dtypes.as_dtype(np.dtype('float16')), tf.dtypes.as_dtype(np.dtype('int32'))]
    # Define static node and graph before for-loop 
    tensor_op = zmq_op.pull(types)
    
    with tf.Session() as sess:
        sess.run(tf.initialize_all_variables())
        shapes = set()
        stats = Stats()
        thoughout_stats = Statistics(1000)
        count = 0
        total = 0
        
        start = time.time()
        for i in range(num):
            outs = sess.run(tensor_op)
            thoughout_stats.add(sum([x.nbytes for x in outs]))
            count += 1
            a = outs[0]
            total += len(a)
            shapes.add((str(a.dtype),) + tuple(a.shape))
            # stats is a heavy compute here.
            # if you comment, you can see speed goes up
            stats += a.astype(np.float32)
        finish = time.time()
        print("{} batches, {} samples".format(count, total))
        print("{:.3g} s per batch, {:.3g} s per sample".format(
            (finish - start)/count, (finish - start)/total))
        print("shapes:", shapes)
        print(stats.summary())
        print(thoughout_stats.summary())

tf_client()