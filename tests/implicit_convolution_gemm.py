# https://github.com/hpca-uji/convGemm
# https://github.com/NVIDIA/cutlass/blob/master/media/docs/implicit_gemm_convolution.md#implicit-gemm-algorithm
from math import floor
import numpy as np

stride_h = 2
stride_w = 2

pad_h = 1
pad_w = 1

N = 1
H, W = 5, 5
C = 1

K = 1
R, S = 3, 3

P = floor((H - R + 2 * pad_h) / stride_h + 1)
Q = floor((W - S + 2 * pad_w) / stride_w + 1)

_x = [ 
    2, 1, 1, 0, 0,
    2, 2, 0, 2, 0,
    0, 0, 2, 1, 0,
    1, 0, 2, 1, 1,
    1, 2, 2, 0, 1,
]

X = np.array(_x,).reshape([N, H, W, C])
W = np.array([1, -1, 0, 1, 0, -1, 0, 0, 1]).reshape([K, R, S, C])
Y = np.zeros([N * P * Q, K])
print(X.shape, W.shape, Y.shape)

f = lambda p, r: int(p * stride_h + R - r - 1 + pad_h)
g = lambda q, s: int(q * stride_h + S - s - 1 + pad_w)


GEMM_M = N * P * Q
GEMM_N = K
GEMM_K = C * R * S
PQ = P * Q
RS = R * S

print("PARALLELIZE ON:", GEMM_M, GEMM_N, GEMM_K)

for i in range(GEMM_M):
    n = floor(i / PQ)
    _res = i % PQ
    p = floor(_res / Q)
    q = _res % Q

    for j in range(GEMM_N):

        acc = 0
        for k in range(GEMM_K):
            c = floor(k / RS)
            _res2 = k % RS

            r = floor(_res2 / S)
            s = _res2 % S

            h = f(p, r)
            w = g(q, s)
            try:
                a = X[n, h, w, c]
            except:
                print(n, h, w, c)
                raise IndexError
            b = W[j, r, s, c]
            acc += a * b

        Y[i, j] = acc
Y = Y.reshape([N, P, Q, K])
print(X)     
print(Y)