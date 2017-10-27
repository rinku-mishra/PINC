import h5py
import matplotlib as mpl
from mpl_toolkits.mplot3d import Axes3D
from numpy import *
from numpy.linalg import *
import matplotlib.pyplot as plt


file = h5py.File('../../data/pop.pop.h5','r')

pos = file['/pos/specie 0']

#for item in f.attrs.keys():
#    print item + ":", f.attrs[item]



particleNum = 1643 
Nt = 200 #pos.shape[1]

Np = pos['n=1.0'].shape[0]	# Number of particles
Nd = pos['n=1.0'].shape[1]	# Number of dimensions

print 'number of particles = %i' % Np
print 'number of dimensions = %i' % Nd
print 'picked particle number %i' % particleNum


x = []
y = []
z = []
for i in range(1,Nt):
	
	positionOfParticle = pos['n=%.1f' % i][particleNum]
	x.append(positionOfParticle[:][0])
	y.append(positionOfParticle[:][1])
	z.append(positionOfParticle[:][2])


#for i in range(0,Nt-1):
#	print "x = %f, y = %f z = %f" %(x[i],y[i],z[i])

fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
ax.plot(x, y, z, label='parametric curve')
ax.set_xlabel('X axis')
ax.set_ylabel('Y axis')
ax.set_zlabel('Z axis')
plt.show()

"""
plt.plot(positionOfParticle[:][2],positionOfParticle[:][1])
plt.title("Position Distribution of particle %i"% particleNum)
plt.ylabel("y/w_pe")
plt.xlabel("x/w_pe")
plt.savefig("pos.png")
plt.close()
"""



