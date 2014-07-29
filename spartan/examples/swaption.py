#!/usr/bin/env python
'''Leif Andersen - A Simple Approach to the pricing of Bermudan swaptions in
  the multifactor LIBOR market model - 1999 - Journal of computational finance.

  Replication of Table 1 p. 16 - European Payer Swaptions.

  Example converted from the Bohrium Project.
  bohrium/benchmark/Python/LMM_swaption_vec.py

'''
import numpy as np  # only for np.int
import spartan
from spartan import (arange, assign, astype, concatenate, exp, maximum,
    ones, randn, sqrt, std, randn, zeros)

# Parameter Values.
DELTA = 0.5
F_0 = 0.06
N = 100
THETA = 0.06


def mu(f, lamb):
  '''Auxiliary function.'''
  tmp = lamb*(DELTA*f[1:, :]) / (1 + DELTA*f[1:, :])  # Andreasen style
  mu = zeros(tmp.shape)
  #mu[0, :] += tmp[0, :]
  mu = assign(mu, (0, slice(None)), mu[0, :] + tmp[0, :])
  for i in xrange(mu.shape[0] - 1):
    #mu[i + 1, :] = mu[i, :] + tmp[i + 1, :]
    mu = assign(mu, (i + 1, slice(None)), mu[i, :] + tmp[i + 1, :])
  return mu


def simulate(ts_all, te_all, lamb_all):
  '''Range over a number of independent products.

  :param ts_all: DistArray
    Start dates for a series of swaptions.
  :param te_all: DistArray
    End dates for a series of swaptions.
  :param lamb_all: DistArray
    Parameter values for a series of swaptions.

  :rtype DistArray

  '''
  for ts_a, te, lamb in zip(ts_all, te_all, lamb_all):
    for ts in ts_a:
      time_structure = arange(None, 0, ts + DELTA, DELTA)
      maturity_structure = arange(None, 0, te, DELTA)

      # MODEL
      # Variance reduction technique - Antithetic Variates.
      eps_tmp = randn(time_structure.shape[0] - 1, N)
      eps = concatenate(eps_tmp, -eps_tmp, 1)

      # Forward LIBOR rates for the construction of the spot measure.
      f_kk = zeros((time_structure.shape[0], 2*N))
      #f_kk[0, :] += F_0  # XXX(rgardner): Expressions are read only.
      f_kk = assign(f_kk, (0, slice(None)), F_0 + f_kk[0, :])

      # Plane kxN of simulated LIBOR rates.
      f_kn = ones((maturity_structure.shape[0], 2*N))*F_0

      # Simulations of the plane f_kn for each time step.
      for t in xrange(1, time_structure.shape[0], 1):
        f_kn_new = ones((maturity_structure.shape[0], 2*N))*F_0
        f_kn_new = f_kn[1:, :]*exp(lamb*mu(f_kn, lamb)*DELTA-0.5*lamb*lamb *
            DELTA + lamb*eps[t - 1, :] * sqrt(DELTA))
        # XXX(rgardner): Expressions are read only.
        #f_kk[t, :] = f_kn_new[0, :]
        f_kk = assign(f_kk, (t, slice(None)), f_kn_new[0, :])
        f_kn = f_kn_new

      # PRODUCT
      # Value of zero coupon bonds.
      zcb = ones((astype(te-ts, np.int)/DELTA)+1, 2*N)
      for j in xrange(zch.shape[0] - 1):
        # XXX(rgardner): Expressions are read only.
        #zcb[j + 1, :] = zcb[j, :] / (1 + DELTA*f_kn[j, :])
        tmp = zcb[j, :] / (1 + DELTA*f_kn[j, :])
        zcb = assign(zcb, (j + 1, slice(None)), tmp)

      # Swaption price at maturity.
      swap_ts = maximum(1 - zcb[-1, :] - THETA*DELTA*sum(zcb[1:, :], 0), 0)

      # Spot measure used for discounting.
      b_ts = ones((2*N))
      for j in xrange(astype(ts/DELTA, np.int)):
        b_ts *= (1 + DELTA*f_kk[j, :])

      # Swaption prce at time 0.
      swaption = swap_ts/b_ts

      # Save expected value in bps and std.
      swaptions = concatenate(swaptions, [[sp.mean((swaption[0:N] +
          swaption[N:])/2) * 10000], [std((swaption[0:N] +
          swaption[N:])/2)/sqrt(N)*10000]], 1)
