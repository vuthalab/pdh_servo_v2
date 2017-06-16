import numpy as np

# specify gain at 1 kHz of first stage integrator
gain_db_1k = 103. - 40.  # loop gain is 40dB

gain_1k = 10**(gain_db_1k/20)

# The first resistor is 1k, figure out the capacitor needed
C1 = 1/(2.0*np.pi*1e3*1e3*gain_1k)

# manually set C1
C1 = 100e-12

# First pole, switching to proportional gain
f1 = 200e3

R2 = 1/(2.0*np.pi*f1*C1)

# manually set R2
R2 = 10e3
