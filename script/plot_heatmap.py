# -*- coding: utf-8 -*-
# -----------------------------------------------------------------------------
# Copyright (c) Vispy Development Team. All Rights Reserved.
# Distributed under the (new) BSD License. See LICENSE.txt for more info.
# -----------------------------------------------------------------------------
# vispy: gallery 100
"""
Illustrate how to plot a 2D function (an image) y=f(x,y) on the GPU.
"""
import sys
import vispy
vispy.use('PyQt6')
from vispy import app, gloo
from pylsl import StreamInlet, resolve_stream
import numpy as np
import math
from vispy.util.transforms import ortho

abc = 0


# Number of cols and rows in the table.
stream_name = sys.argv[1]
first_ch = int(sys.argv[2])
H = int(sys.argv[3])
W = int(sys.argv[4])
min = int(sys.argv[5])
max = int(sys.argv[6])
a= 2./float(max-min)
b= 1-a*max 


print("looking for " + stream_name + " stream ...")
streams = resolve_stream('name', stream_name)
if(len(streams)==0):
    print("no stream found. Exit.")
    exit(0)

inlet = StreamInlet(streams[0])

# Number of signals.
m = H*W

if(streams[0].channel_count()-first_ch < m):
    print("Error number of channel. Exit.")
    exit(0)


# Image to be displayed
img_array = np.random.uniform(0, 1, (W, H)).astype(np.float32)

# A simple texture quad
data = np.zeros(4, dtype=[('a_position', np.float32, 2),
                          ('a_texcoord', np.float32, 2)])
data['a_position'] = np.array([[0, 0], [W, 0], [0, H], [W, H]])
data['a_texcoord'] = np.array([[0, 0], [0, 1], [1, 0], [1, 1]])


VERT_SHADER = """
// Uniforms
uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform float u_antialias;

// Attributes
attribute vec2 a_position;
attribute vec2 a_texcoord;

// Varyings
varying vec2 v_texcoord;

// Main
void main (void)
{
    v_texcoord = a_texcoord;
    gl_Position = u_projection * u_view * u_model * vec4(a_position,0.0,1.0);
}
"""

FRAG_SHADER = """
uniform sampler2D u_texture;
varying vec2 v_texcoord;

vec4 jet(float x) {
    vec3 a, b;
    float c;
    if (x < 0.34) {
        a = vec3(0, 0, 0.5);
        b = vec3(0, 0.8, 0.95);
        c = (x - 0.0) / (0.34 - 0.0);
    } else if (x < 0.64) {
        a = vec3(0, 0.8, 0.95);
        b = vec3(0.85, 1, 0.04);
        c = (x - 0.34) / (0.64 - 0.34);
    } else if (x < 0.89) {
        a = vec3(0.85, 1, 0.04);
        b = vec3(0.96, 0.7, 0);
        c = (x - 0.64) / (0.89 - 0.64);
    } else {
        a = vec3(0.96, 0.7, 0);
        b = vec3(0.5, 0, 0);
        c = (x - 0.89) / (1.0 - 0.89);
    }
    return vec4(mix(a, b, c), 1.0);
}

void main()
{
    gl_FragColor = texture2D(u_texture, v_texcoord);
    gl_FragColor = jet(( gl_FragColor.x ));
}

"""


class Canvas(app.Canvas):

    def __init__(self):
        app.Canvas.__init__(self, keys='interactive', size=((W * 100), (H * 100)))

        self.program = gloo.Program(VERT_SHADER, FRAG_SHADER)
        self.texture = gloo.Texture2D(img_array, interpolation='linear')

        self.program['u_texture'] = self.texture
        self.program.bind(gloo.VertexBuffer(data))

        self.view = np.eye(4, dtype=np.float32)
        self.model = np.eye(4, dtype=np.float32)
        self.projection = np.eye(4, dtype=np.float32)

        self.program['u_model'] = self.model
        self.program['u_view'] = self.view
        self.projection = ortho(0, W, 0, H, -1, 1)
        self.program['u_projection'] = self.projection

        gloo.set_clear_color('white')

        self._timer = app.Timer('auto', connect=self.update, start=True)

        self.show()
        

    def on_resize(self, event):
        width, height = event.physical_size
        gloo.set_viewport(0, 0, width, height)
        self.projection = ortho(0, width, 0, height, -100, 100)
        self.program['u_projection'] = self.projection

        # Compute thje new size of the quad
        r = width / float(height)
        R = W / float(H)
        if r < R:
            w, h = width, width / R
            x, y = 0, int((height - h) / 2)
        else:
            w, h = height * R, height
            x, y = int((width - w) / 2), 0
        data['a_position'] = np.array(
            [[x, y], [x + w, y], [x, y + h], [x + w, y + h]])
        self.program.bind(gloo.VertexBuffer(data))

        
    def on_draw(self, event):
        gloo.clear(color=True, depth=True)
        
        sample, timestamp = inlet.pull_chunk()
        if(timestamp):
            img_array[...] = np.array(sample[len(sample)-1][first_ch:first_ch+m]).reshape(W, H)*a + b
            
        self.texture.set_data(img_array)
        self.program.draw('triangle_strip')


if __name__ == '__main__':
    c = Canvas()
    app.run()
