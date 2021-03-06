import cvxopt

import numpy
import pandas
from matplotlib import pyplot
import scipy.signal

def fit(basis, gamma, norm, data):
    """Represent data in basis by find coefficients u_i
         min_{a} ||Σ_i u_i b_i(x) - data(x)||_2^2 + ᴦ||u||_norm
       where basis[] is an array of functions of a single variable x.
    """
    x = data.index.values
    y = cvxopt.matrix(data)
    Anumpy = numpy.array([b(x) for b in basis]).T
    A = cvxopt.matrix(Anumpy)
    A /= numpy.sqrt(gamma)
    y /= numpy.sqrt(gamma)
    import l1regls
    u = l1regls.l1regls(A,y)
    return u, Anumpy.dot(numpy.array(u))

def main(args):
    df = pandas.read_csv('data/nlogn.csv', index_col=[0])
    pandas.set_option('display.width', 180)
    def f(i,j):
        def inner(n):
            return n**i * numpy.log(n)**j
        return inner
    basis = []
    for i in [0,.5,1,2]:
        for j in [0,1,2]:
            basis += [f(i,j)]

    u, yu = fit(basis, args.gamma, 1, df['y1'])
    print(u)
    df['yu'] = yu
    df.plot()
    pyplot.show()

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser('Performance modeling and fitting program')
    parser.add_argument('--gamma', help='Regularization parameter', type=float, default=1)
    args = parser.parse_args()
    main(args)

