#!/usr/bin/env python
# /* -*-  indent-tabs-mode:t; tab-width: 8; c-basic-offset: 8  -*- */
# /*
# Copyright (c) 2013, Daniel M. Lofaro
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the author nor the names of its contributors may
#       be used to endorse or promote products derived from this software
#       without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# */

import hubo_ach as hubo
import ach
import sys
import time
from ctypes import *

# Open Hubo-Ach feed-forward and feed-back (reference and state) channels
n = ach.Channel(hubo.HUBO_CHAN_REF_NECK_NAME)
n.flush()
s = ach.Channel(hubo.HUBO_CHAN_STATE_NAME)
s.flush()

state = hubo.HUBO_STATE()
ref = hubo.HUBO_REF()

NK1_value=0
NKY_value=0

NKY_right_limit=-1.5/2
NKY_left_limit=1.5/2

[statuss, framesizes] = s.get(state, wait=False, last=False)

motion=sys.argv[1]
print motion
cont= True;
while (cont):
	if (motion=='r'):
		while (NKY_value>NKY_right_limit):
			NKY_value=NKY_value - 0.3
			ref.ref[hubo.NKY]=NKY_value
	elif (motion=='l'):
		while (NKY_value<NKY_left_limit):
			NKY_value=NKY_value + 0.3
			ref.ref[hubo.NKY]=NKY_value
	elif (motion=='exit'):
		cont=False;
	else:
		print "weird input";

	cont=False
	print "Joint NKY = ", NKY_value
	print "Joint NK1 = ", NK1_value

	# Write to the feed-forward channel
	n.put(ref)

# Close the connection to the channels
n.close()
s.close()

