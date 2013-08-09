hubo-read-trajectory
====================

Usage: ./hubo-read-trajectory -f 100 -n fileName.traj
	Options:
		-h   help menu
		-n   change trajectory
				default: no file
				atguements: filename
		-f   change frequency
			default: 25hz
			atguements: frequency
				options (hz):
					10
					25
					50
					100
					200
					500
		-c  to enable compliance mode read trajectory
		-p  to enable pause and play feaure "p" should be pressed to pause or play
		    note that in compliance mode pause will pause the motion immediately


File format (Each Column)
	RHY RHR RHP RKN RAP RAR LHY LHR LHP LKN LAP LAR RSP RSR RSY REB RWY RWR RWP LSP LSR LSY LEB LWY LWR LWP NKY NK1 NK2 WST RF1 RF2 RF3 RF4 RF5 LF1 LF2 LF3 LF4 LF5

