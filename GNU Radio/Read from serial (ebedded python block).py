"""
Embedded Python Blocks:

Each time this file is saved, GRC will instantiate the first class it finds
to get ports and parameters of your block. The arguments to __init__  will
be the parameters. All of them are required to have default values!
"""

import serial
import re
import numpy as np
from gnuradio import gr

class blk(gr.sync_block):  # other base classes are basic_block, decim_block, interp_block
    """
    Read data from serial port and forward it to output. 
    portNumber is number, you can see in the name of your 
    serial port in device manager, like COM16 or COM7 
    """

    decodedNumber = 0
    
    def __init__(self, portNumber = 16, baudrate = 2000000): 
        """arguments to this function show up as parameters in GRC"""
        
        portName = 'COM' + str(portNumber) # Concatenate string and number
        
        gr.sync_block.__init__(
            self,
            name = 'Serial Read',   
            in_sig = [], # no inputs 
            out_sig = [np.float32] # also you can try to use int
        )
        
        self.port = serial.Serial(portName, baudrate, timeout = 3.0) # open serial
        if (self.port.is_open): #if already opened - close and open again
            self.port.close()
            self.port.open()
        else:
            self.port.open()
            
            
    def work(self, input_items, outputItems):
        
        serialData = self.port.readline() # byte-like variable
        decoded = str(serialData) # transform into string
        
        temp = re.findall(r'\d+', decoded) #remove system symbols
        
        try:
            self.decodedNumber = temp[0] 
        except:
            outputItems[0][:] = self.decodedNumber 
        
        outputItems[0][:] = float(self.decodedNumber)
        return len(outputItems[0])
