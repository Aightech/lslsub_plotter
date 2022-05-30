#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vispy: gallery 2
# Copyright (c) Vispy Development Team. All Rights Reserved.
# Distributed under the (new) BSD License. See LICENSE.txt for more info.

"""
Multiple real-time digital signals with GLSL-based clipping.
"""

import sys
import vispy
vispy.use('PyQt6')
from vispy import gloo
from vispy import app
from vispy import scene
from vispy.scene.visuals import Text

import numpy as np
import math

from pylsl import StreamInlet, resolve_stream

import scipy as sp
from scipy import signal
from scipy import fft
from scipy.fft import rfft






# Number of cols and rows in the table.
stream_name = sys.argv[1]
first_ch = int(sys.argv[2])
nrows = int(sys.argv[3])
ncols = int(sys.argv[4])

min_v = int(sys.argv[5])
max_v =  int(sys.argv[6])
one_plot = int(sys.argv[7])
a= 2./float(max_v-min_v)
b= 1-a*max_v

auto_scale = int(sys.argv[8])
# Number of samples per signal.
n = int(sys.argv[9])
fft_enabled = int(sys.argv[10])

order = int(sys.argv[11])
lowpass_f = float(sys.argv[12])
highpass_f =float(sys.argv[13])
stopband_low_f = float(sys.argv[14])
stopband_high_f = float(sys.argv[15])

AC_enabled = int(sys.argv[16])
rectified_enabled = int(sys.argv[17])


print("looking for " + stream_name + " stream ...")
streams = resolve_stream('name', stream_name)
if(len(streams)==0):
    print("no stream found. Exit.")
    exit(0)

inlet = StreamInlet(streams[0])

# Number of signals.
m = nrows*ncols
if(streams[0].channel_count()-first_ch < m):
    print("Error number of channel. Exit.")
    exit(0)



# Generate the signals as a (m, n) array.
y = np.zeros((m, n)).astype(np.float32)
y_processed = np.zeros((m, n)).astype(np.float32)
t = np.zeros(n).astype(np.float32)

# Color of each vertex (TODO: make it more efficient by using a GLSL-based
# color map and the index).
color = np.repeat(np.random.uniform(size=(m, 3), low=.5, high=.9),
                  n, axis=0).astype(np.float32)

# Signal 2D index of each vertex (row and col) and x-index (sample index
# within each signal).
index = np.c_[np.repeat(np.repeat(np.arange(ncols), nrows), n),
              np.repeat(np.tile(np.arange(nrows), ncols), n),
              np.tile(np.arange(n), m)].astype(np.float32)


VERT_SHADER = """
#version 120

// y coordinate of the position.
attribute float a_position;

// row, col, and time index.
attribute vec3 a_index;
varying vec3 v_index;

// 2D scaling factor (zooming).
uniform vec2 u_scale;

// Size of the table.
uniform vec2 u_size;

// Number of samples per signal.
uniform float u_n;

// Color.
attribute vec3 a_color;
varying vec4 v_color;

// Varying variables used for clipping in the fragment shader.
varying vec2 v_position;
varying vec4 v_ab;

void main() {
    float nrows = u_size.x;
    float ncols = u_size.y;

    // Compute the x coordinate from the time index.
    float x = -1 + 2*a_index.z / (u_n-1);
    vec2 position = vec2(x - (1 - 1 / u_scale.x), a_position);
"""
if(one_plot):
    VERT_SHADER += """
    // Find the affine transformation for the subplots.
    vec2 a = vec2(1., 1.)*.9;
    vec2 b = vec2(0, 0);
    """
else:
    VERT_SHADER += """
    // Find the affine transformation for the subplots.
    vec2 a = vec2(1./ncols, 1./nrows)*.9;
    vec2 b = vec2(-1 + 2*(a_index.x+.5) / ncols,
    -1 + 2*(a_index.y+.5) / nrows);
    """
    

VERT_SHADER +=""" 
// Apply the static subplot transformation + scaling.
    gl_Position = vec4(a*u_scale*position+b, 0.0, 1.0);

    v_color = vec4(a_color, 1.);
    v_index = a_index;

    // For clipping test in the fragment shader.
    v_position = gl_Position.xy;
    v_ab = vec4(a, b);
}
"""

FRAG_SHADER = """
#version 120

varying vec4 v_color;
varying vec3 v_index;

varying vec2 v_position;
varying vec4 v_ab;

void main() {
    gl_FragColor = v_color;

    // Discard the fragments between the signals (emulate glMultiDrawArrays).
    if ((fract(v_index.x) > 0.) || (fract(v_index.y) > 0.))
        discard;

    // Clipping test.
    vec2 test = abs((v_position.xy-v_ab.zw)/v_ab.xy);
    if ((test.x > 1) || (test.y > 1))
        discard;
}
"""


class Canvas(app.Canvas):
    def __init__(self):
        app.Canvas.__init__(self, title='Use your wheel to zoom!',
                            keys='interactive')
        self.program = gloo.Program(VERT_SHADER, FRAG_SHADER)
        self.program['a_position'] = y.reshape(-1, 1)
        self.program['a_color'] = color
        self.program['a_index'] = index
        self.program['u_scale'] = (1., 0.5)
        self.program['u_size'] = (nrows, ncols)
        self.program['u_n'] = n

        gloo.set_viewport(0, 0, *self.physical_size)

        self._timer = app.Timer('auto', connect=self.on_timer, start=True)

        gloo.set_state(clear_color='black', blend=True,
                       blend_func=('src_alpha', 'one_minus_src_alpha'))

        self.show()

    def on_resize(self, event):
        gloo.set_viewport(0, 0, *event.physical_size)

    def on_mouse_wheel(self, event):
        dx = np.sign(event.delta[1]) * .05
        scale_x, scale_y = self.program['u_scale']
        scale_x_new, scale_y_new = (scale_x * math.exp(0.0*dx),
                                    scale_y * math.exp(1.0*dx))
        self.program['u_scale'] = (max(1, scale_x_new), max(1, scale_y_new))
        self.update()

    def on_timer(self, event):
        """Add some data at the end of each signal (real-time signals)."""
        sample, timestamp = inlet.pull_chunk()
        if(timestamp):
             global a, b
             
             k=len(sample)
             s = np.array(sample)
             y[:, :-k] = y[:, k:]
             y[:, -k:] = s[:,first_ch:first_ch+m].transpose()
             t[:-k] = t[k:]
             t[-k:] = timestamp[:]


             y_processed = y.copy()
             if(AC_enabled):
                 y_processed = y_processed - np.mean(y_processed)
            
             if(t[0]==0):
                 fs = len(timestamp)/( timestamp[-1]- timestamp[0])
             else:
                 fs = len(t)/( t[-1]- t[-0])
             print("fs:" +str(fs))
             
             nyq = fs/2
             
             if(lowpass_f!=-1.):
                 low = lowpass_f/nyq
                 sos = sp.signal.butter(order,low, btype='lowpass',output='sos')
                 y_processed = sp.signal.sosfilt(sos, y_processed)
                 #y_processed[:,:order*4] = y_processed[:,order*4+1]

             if(highpass_f!=-1.):
                 high = highpass_f/nyq
                 sos = sp.signal.butter(order,high, btype='highpass',output='sos')
                 y_processed = sp.signal.sosfilt(sos, y_processed)

             if(stopband_low_f!=-1.):
                 low = stopband_low_f/nyq
                 high = stopband_high_f/nyq
                 sos = sp.signal.butter(order,[low,high], btype='bandstop',output='sos')
                 y_processed = sp.signal.sosfilt(sos, y_processed)
                 
             if(rectified_enabled):
                 y_processed = np.abs(y_processed)
                 
            
             if(fft_enabled):
                 #y_processed[:,:int(n/2)+1]=np.abs(sp.fft.rfft(y_processed))[0]/1000
                 y_processed[:,::2]= np.abs(sp.fft.rfft(y_processed))[0][:-1]/1000
                 y_processed[:,1::2]= y_processed[:,::2]
                 

             if(auto_scale==1):
                 min_v = np.min(y_processed[:,:][0])
                 max_v = np.max(y_processed[:,:][0])
                 a= 2./float(max_v-min_v)
                 b= 1-a*max_v
             
             self.program['a_position'].set_data(y_processed.ravel().astype(np.float32)*a + b)
             self.update()
             if(one_plot):
                 print(y_processed[:, -1])
                 #print(s[:,first_ch:first_ch+m][0])

    def on_draw(self, event):
        gloo.clear()
        self.program.draw('line_strip')

if __name__ == '__main__':
    c = Canvas()
    app.run()
