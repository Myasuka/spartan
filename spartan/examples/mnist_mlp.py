import sys
import os
import math
import numpy as np
import numpy.random
import scipy.io as si

import spartan
import time


def _extract(prefix, md, max_dig):
  ret = []
  for dig in range(max_dig):
    samples = md[prefix + str(dig)]
    labels = np.empty([samples.shape[0], 1], dtype=np.float32)
    labels.fill(dig)
    ret.append(np.hstack((samples.astype(np.float32) / 256, labels)))
  return ret


def _split_sample_and_label(merged_mb):
  [s, l] = np.hsplit(merged_mb, [merged_mb.shape[1] - 1])
  # change label to sparse representation
  n = merged_mb.shape[0]
  ll = np.zeros([n, 10], dtype=np.float32)
  ll[np.arange(n), l.astype(int).flat] = 1
  return (spartan.from_numpy(s).optimized().evaluate(),
          spartan.from_numpy(ll).optimized().evaluate())


def load_mb_from_mat(mat_file, mb_size):
  # load from mat
  md = si.loadmat(mat_file)
  # merge all data
  train_all = np.concatenate(_extract('train', md, 10))
  test_all = np.concatenate(_extract('test', md, 10))
  # shuffle
  np.random.shuffle(train_all)
  # make minibatch
  train_mb = np.vsplit(train_all, range(mb_size, train_all.shape[0], mb_size))
  train_data = map(_split_sample_and_label, train_mb)
  # test_data = _split_sample_and_label(test_all)
  test_data = None
  print 'Training data: %d mini-batches' % len(train_mb)
  #print 'Test data: %d samples' % test_all.shape[0]
  print train_data[0][1].shape
  return (train_data, test_data)


def relu(x):
  #zm = np.zeros(x.shape)
  #return np.greater(x, zm) * x
  return (x > 0) * x


def relu_back(y, x):
  #zm = np.zeros(x.shape)
  #return np.greater(x, zm) * y
  return (x > 0) * y


def softmax(m):
  #maxval = np.max(m, axis=0)
  #centered = m - maxval
  #class_normalizer = np.log(np.max(np.exp(centered), axis=0)) + maxval
  #return np.exp(m - class_normalizer)
  maxval = spartan.max(m, axis=0)
  centered = m - maxval
  class_normalizer = spartan.log(spartan.max(spartan.exp(centered), axis=0)) + maxval
  return spartan.exp(m - class_normalizer)


class MnistTrainer:
  def __init__(self, data_file='mnist_all.mat', num_epochs=100, mb_size=256,
               eps_w=0.01, eps_b=0.01, ctx=None):
    self.data_file = data_file
    self.num_epochs = num_epochs
    self.mb_size = mb_size
    self.eps_w = eps_w
    self.eps_b = eps_b
    self.ctx = ctx
    # init weight
    l1 = 784
    l2 = mb_size
    l3 = 10
    self.l1 = l1
    self.l2 = l2
    self.l3 = l3
    #self.w1 = np.random.randn(l2, l1) * math.sqrt(4.0 / (l1 + l2))
    #self.w2 = np.random.randn(l3, l2) * math.sqrt(4.0 / (l2 + l3))
    #self.b1 = np.zeros([l2, 1])
    #self.b2 = np.zeros([l3, 1])
    self.w1 = (spartan.randn(l2, l1) * math.sqrt(4.0 / (l1 + l2))).optimized().evaluate()
    self.w2 = (spartan.randn(l3, l2) * math.sqrt(4.0 / (l2 + l3))).optimized().evaluate()
    self.b1 = (spartan.zeros([l2, 1])).optimized().evaluate()
    self.b2 = (spartan.zeros([l3, 1])).optimized().evaluate()

  def run(self):
    (train_data, test_data) = load_mb_from_mat(self.data_file, self.mb_size)
    #num_test_samples = test_data[0].shape[0]
    #(test_samples, test_labels) = test_data
    count = 1
    begin = time.time()
    begin2 = begin
    for epoch in range(self.num_epochs):
      print '---Start epoch #%d' % epoch
      # train
      for (mb_samples, mb_labels) in train_data:
        num_samples = mb_samples.shape[0]

        a1 = mb_samples.T
        target = mb_labels.T

        # ff
        #a2 = relu(np.dot(self.w1, a1) + self.b1)
        #a3 = np.dot(self.w2, a2) + self.b2
        a2 = relu(spartan.dot(self.w1, a1) + self.b1)
        a3 = spartan.dot(self.w2, a2) + self.b2
        # softmax & error
        out = softmax(a3)
        s3 = out - target
        # bp
        #s2 = np.dot(self.w2.T, s3)
        s2 = spartan.dot(self.w2.T, s3)
        s2 = relu_back(s2, a2)
        # grad
        #gw1 = np.dot(s2, a1.T) / num_samples
        #gb1 = np.sum(s2, axis=1, keepdims=True) / num_samples
        #gw2 = np.dot(s3, a2.T) / num_samples
        #gb2 = np.sum(s3, axis=1, keepdims=True) / num_samples
        gw1 = spartan.dot(s2, a1.T) / num_samples
        gb1 = (spartan.sum(s2, axis=1) / num_samples).reshape(s2.shape[0], 1)
        gw2 = spartan.dot(s3, a2.T) / num_samples
        gb2 = (spartan.sum(s3, axis=1) / num_samples).reshape(s3.shape[0], 1)
        # update
        self.w1 -= self.eps_w * gw1
        self.w2 -= self.eps_w * gw2
        self.b1 -= self.eps_b * gb1
        self.b2 -= self.eps_b * gb2

        if (count % 40 == 0):
          #correct = np.argmax(out, axis=0) - np.argmax(target, axis=0)
          #print 'Training error:', float(np.count_nonzero(correct)) / num_samples
          out.optimized().evaluate()
          correct = spartan.argmax(out, axis=0) - spartan.argmax(target, axis=0)
          print 'Training error:', (float(spartan.count_nonzero(correct).glom())
                                    / num_samples)
        count = count + 1
      out.optimized().evaluate()
      print 'spent ', (time.time() - begin)

      # test
      #a1 = test_samples.T
      #a2 = relu(np.dot(self.w1, a1) + self.b1)
      #a3 = np.dot(self.w2, a2) + self.b2
      #correct = np.argmax(a3, axis=0) - np.argmax(test_labels.T, axis=0)
      ##print correct
      #print 'Testing error:', float(np.count_nonzero(correct)) / num_test_samples
      #print '---Finish epoch #%d' % epoch


if __name__ == '__main__':
  spartan.config.parse(sys.argv)
  ctx = spartan.initialize()
  trainer = MnistTrainer(num_epochs=1, ctx=ctx,
                         mb_size=int(256 * math.sqrt(ctx.num_workers)))
  trainer.run()
