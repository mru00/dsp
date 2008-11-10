#! /usr/bin/python

from math import *
from numpy.fft import fft

N = 1<<8
NF = float(N)
A = 1.5
f = 1500.0
phi = 0.0# -pi/2.0
SR = 22050
SRF = float(SR)

def dft(x):
	return [ sum ( [x[n] * cos((pi/N) * (n+0.5)*k)  for n in range(N)]) for k in range(N)]

def arg(x):
	return atan2(x.real, x.imag)


def phasor(x, phi):
	return cos(2.0*pi*f*x/SRF + phi)

#ph_p =  [ phasor(x) for x in range(N)]

#ff_p = fft( [ phasor(x) for x in range(N)] )

def max2(arr):
	
	max1 = max2 = (0,0)
	i = 0

	for r in arr:
		s= abs(r)
		if ( s > max2[1] ):
			max1 = max2
			max2 = (i, r)
		i = i+1

#		print (i, abs(r))
		
	return( max1, max2 )

while phi < 2.0*pi:
	df_p =dft( [ phasor(x, phi) for x in range(N)] )
	ff_p =fft( [ phasor(x, phi) for x in range(N)] )

	phi += pi/16.0

	( m1, m2) = max2(ff_p)
	( n1, n2) = max2(df_p)

	h = 0
	h = 0.5*n2[0]*SRF/NF
	print "dft: ", h, 0.5*n2[0], abs(n2[1]) ,
	print "fft: ", N-m2[0], abs(m2[1]), arg(m2[1])- phi
	
#print max2(ph_p)
