import time
import threading
import os

import numpy as np
import zmq
import msgpack

import tensorflow as tf
from tensorflow.python.platform import test
from tensorflow.python.framework import dtypes
from tensorflow.python.ops import resources


zmq_module = tf.load_op_library('./build/nvzmq_ops/kernel/nvzmq_ops.so')
zmq_op = zmq_module.nv_zmq
zmq_conn_handle = zmq_module.zmq_conn_handle
allowable_dtypes = {"uint8", "uint16", "int16", "int32", "float16", "float32", "float64"}

TADDR_ARGS = 'zrpull://127.0.0.1:5678'
ZMQ_HWM = 100

class TestZMQResourceHandle(test.TestCase):

	def test_simple(self):
		with self.session():
			TADDR_VALID = 'zrpull://127.0.0.1:5555'
			output = zmq_conn_handle(TADDR_VALID, ZMQ_HWM, 0)
			resources.initialize_resources(resources.local_resources()).run()
			# assertDTypeEqual not working for resource type. it trans tf.dtype to np.dtype and resource is incompatible with numpy
			#self.assertDtypeEqual(output, dtypes.resource.as_numpy_type)
			self.assertEqual(type(output.dtype), type(dtypes.resource))
	
	def test_invalid_address_type(self):
		INVALID_ADDR = 'localhost:8089'
		with self.assertRaises(tf.errors.InvalidArgumentError):
			with self.session():
				zmq_conn_handle(INVALID_ADDR, ZMQ_HWM, 0).eval()

class TestZMQOpArguments(test.TestCase):

	def test_no_arguments(self):
		with self.assertRaises(TypeError):
			zmq_op()

	def test_invalid_type_format(self):
		with self.assertRaises(TypeError):
			zmq_op(handle=zmq_conn_handle(address=TADDR_ARGS, zmq_hwm=ZMQ_HWM, zmq_buff=0), types=tf.int32)

	def test_invalid_type_length(self):
		with self.assertRaises(ValueError):
			zmq_op(handle=zmq_conn_handle(address=TADDR_ARGS, zmq_hwm=ZMQ_HWM, zmq_buff=0), types=[])

	def test_invalid_output_type(self):
		with self.assertRaises(TypeError):
			zmq_op(handle=zmq_conn_handle(address=TADDR_ARGS, zmq_hwm=ZMQ_HWM, zmq_buff=0), types=[tf.bool])

	def test_valid_arguments(self):
		zmq_layer = zmq_op(handle=zmq_conn_handle(address=TADDR_ARGS, zmq_hwm=ZMQ_HWM, zmq_buff=0), types=[tf.int32, tf.float32])
		self.assertEqual(len(zmq_layer), 2)

		self.assertEqual(type(zmq_layer[0]), tf.Tensor)
		self.assertEqual(type(zmq_layer[1]), tf.Tensor)

		self.assertEqual(zmq_layer[0].dtype, tf.int32)
		self.assertEqual(zmq_layer[1].dtype, tf.float32)

		self.assertEqual(zmq_layer[0].shape, tf.TensorShape(None))
		self.assertEqual(zmq_layer[1].shape, tf.TensorShape(None))

class TestZMQOpParse(test.TestCase):

	def send_msgs(socket, msgs, multipart = False):
		if multipart:
			socket.send_multipart(msgs)
		else:
			for msg in msgs:
				socket.send(msg)
				time.sleep(len(msg) / 1000)

	# dlinput
	def np2dict(a, parts=None, allow_float64=False):
		"""Recursively convert numpy tensors in data structures to dictionaries."""
		if isinstance(a, np.ndarray):
			assert allow_float64 or a.dtype != np.dtype("float64")
			dtype = str(a.dtype)
			assert dtype in allowable_dtypes, dtype
			if parts is None:
				return dict(_shape=list(a.shape),
							_dtype=dtype,
							_data=a.tobytes())
			else:
				index = len(parts)
				parts.append(a.tobytes())
				return dict(_shape=list(a.shape),
							_dtype=dtype,
							_part=index)
		elif isinstance(a, list):
			return [TestZMQOpParse.np2dict(x, parts) for x in a]
		elif isinstance(a, dict):
			return {k: TestZMQOpParse.np2dict(v, parts) for k,v in a.items()}
		else:
			return a

	def test_corrupt_msg_pack_data(self):
		CORRUPT_ADDR = 'zrpull://127.0.0.1:5555'
		TSENDER_ADDR_CORRUPT = 'tcp://127.0.0.1:5555'
		ctx = zmq.Context(1)
		socket = ctx.socket(zmq.PUSH)
		try:
			socket.bind(TSENDER_ADDR_CORRUPT)
			tensor_msg = msgpack.packb([['garbage data']], use_bin_type=True)
			thread = self.checkedThread(target=TestZMQOpParse.send_msgs, args=(socket, [tensor_msg]))
			thread.start()
			with self.assertRaises(tf.errors.DataLossError):
				with self.session() as sess:
					zmq_op(handle=zmq_conn_handle(address=CORRUPT_ADDR, zmq_hwm=ZMQ_HWM, zmq_buff=0), types=[tf.int32])[0].eval()
		except Exception as e:
			self.fail()
		finally:
			thread.join()
			socket.close()
			ctx.term()

	'''
	If no timeout setting, comment following two timeout tests
	'''
	# def test_timeout(self):
	# 	TADDR_VALID = 'zrpull://127.0.0.1:5555'
	# 	output = zmq_op(handle=zmq_conn_handle(), address=TADDR_VALID, types=[tf.float32, tf.int32])
	# 	with self.assertRaises(tf.errors.CancelledError):
	# 		with tf.Session() as sess:
	# 			sess.run(output)

	# def test_timeout_multithread(self):
	# 	TADDR_VALID = 'zrpull://127.0.0.1:5555'
	# 	handle = zmq_conn_handle()
	# 	ops = []
	# 	for i in range(2):
	# 		ops.extend(zmq_op(handle=handle, address=TADDR_VALID, types=[tf.float32, tf.int32]))
	# 	with self.assertRaises(tf.errors.CancelledError):
	# 		with self.session() as sess:
	# 			sess.run(tf.tuple(ops))

	def test_single_op_valid(self):
		TADDR_VALID = 'zrpull://127.0.0.1:5555'
		TSENDER_ADDR_VALID = 'tcp://127.0.0.1:5555'
		SINGLE_DATA = [44]
		ctx = zmq.Context(1)
		socket = ctx.socket(zmq.PUSH)
		try:
			socket.bind(TSENDER_ADDR_VALID)
			tensor_data1 = np.arange(16, dtype=np.uint8).reshape((4,4))
			tensor_data2 = np.array(SINGLE_DATA, dtype=np.int32)
			tensor_data_list = [tensor_data1, tensor_data2]
			packed = msgpack.dumps(TestZMQOpParse.np2dict(tensor_data_list))
			thread = self.checkedThread(target=TestZMQOpParse.send_msgs, args=(socket, [packed]))
			thread.start()

			tensors = zmq_op(handle=zmq_conn_handle(address=TADDR_VALID, zmq_hwm=ZMQ_HWM, zmq_buff=0), types=[tf.uint8, tf.int32])
			with self.session() as sess:
				outputs = sess.run(tensors)
				self.assertEqual(len(outputs), 2)
				self.assertTrue(np.array_equal(outputs[0], np.arange(16, dtype=np.float32).reshape(4,4)))
				self.assertTrue(np.array_equal(outputs[1], np.array(SINGLE_DATA, dtype=np.int32)))
		except Exception as e:
			self.fail()
		finally:
			thread.join()
			socket.close()
			ctx.term()



	def test_multithread(self):
		TADDR_VALID = 'zrpull://127.0.0.1:5556'
		TSENDER_ADDR_VALID = 'tcp://127.0.0.1:5556'
		NUM_THREAD= 4
		try:
			ctx = zmq.Context(1)
			socket = ctx.socket(zmq.PUSH)
			socket.bind(TSENDER_ADDR_VALID)
			msgs = []
			expected = []
			for i in range(1, NUM_THREAD + 1):
				tensor_data1 = np.arange(i*i, dtype=np.float32).reshape((i*i))
				tensor_data2 = np.array([i], dtype=np.int32)
				tensor_data3 = np.array([i], dtype=np.uint8)
				tensor_data_list = [tensor_data1, tensor_data2, tensor_data3]
				expected.append(tensor_data_list)
				packed = msgpack.dumps(TestZMQOpParse.np2dict(tensor_data_list))
				msgs.append(packed)
			thread = self.checkedThread(target=TestZMQOpParse.send_msgs, args=(socket, msgs))
			thread.start()

			handle = zmq_conn_handle(address=TADDR_VALID, zmq_hwm=ZMQ_HWM, zmq_buff=0)
			tensor_lists = []
			for i in range(NUM_THREAD):
				tensors = zmq_op(handle=handle, types=[tf.float32, tf.int32, tf.uint8])
				tensor_lists.append(tensors)
			with self.session() as sess:
				# Writing a graph on tensorboard
				# cwd = os.getcwd()
				# writer = tf.summary.FileWriter(cwd + '/tfboardlogs/', sess.graph)
				output = sess.run(tensor_lists)
				self.assertEqual(len(output), len(expected))
				output.sort(key=lambda x: (x[0].shape[0]))
				for a, b in zip(output, expected):
					for x, y in zip(a, b):
						self.assertAllEqual(x, y)
			# writer.close()
		except Exception as e:
			self.fail()
		finally:
			thread.join()
			socket.close()
			ctx.term()

	def test_multipart(self):
		TADDR_VALID = 'zrpull://127.0.0.1:5555'
		TSENDER_ADDR_VALID = 'tcp://127.0.0.1:5555'
		SINGLE_DATA = [44]
		ctx = zmq.Context(1)
		socket = ctx.socket(zmq.PUSH)
		try:
			socket.bind(TSENDER_ADDR_VALID)
			tensor_data1 = np.arange(16, dtype=np.uint8).reshape((4,4))
			tensor_data2 = np.array(SINGLE_DATA, dtype=np.int32)
			tensor_data_list = [tensor_data1, tensor_data2]
			parts = [None]
			packed = msgpack.dumps(TestZMQOpParse.np2dict(tensor_data_list, parts))
			parts[0] = packed

			thread = self.checkedThread(target=TestZMQOpParse.send_msgs, args=(socket, parts, True))
			thread.start()

			tensors = zmq_op(handle=zmq_conn_handle(address=TADDR_VALID, zmq_hwm=ZMQ_HWM, zmq_buff=0), types=[tf.uint8, tf.int32])
			with self.session() as sess:
				outputs = sess.run(tensors)
				self.assertEqual(len(outputs), 2)
				self.assertTrue(np.array_equal(outputs[0], np.arange(16, dtype=np.float32).reshape(4,4)))
				self.assertTrue(np.array_equal(outputs[1], np.array(SINGLE_DATA, dtype=np.int32)))
		except Exception as e:
			self.fail()
		finally:
			thread.join()
			socket.close()
			ctx.term()




if __name__ == '__main__':
	test.main()
