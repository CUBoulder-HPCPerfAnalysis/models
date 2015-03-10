import numpy
import pandas

x = numpy.arange(1,100)
y = x * numpy.log(x)
df = pandas.DataFrame(numpy.array([y, y+numpy.random.randn(y.size)]).T, index=x, columns=['y', 'y1'])
df.index.name = 'n'
df.to_csv('data/nlogn.csv')
