from math import sin, pi
from argparse import ArgumentParser

from time import time, sleep, monotonic
from json import dumps, loads

from serial import serial_for_url
from serial.tools.list_ports import comports
from serial.tools.list_ports_common import ListPortInfo

import struct
import sysv_ipc

from Functions import Communication, DualCommunication, CommunicationError
from Functions import PRINT_INFO, PRINT_DEBUG, PRINT_ERROR


DAC_ARM = True
if DAC_ARM:
	MEMKEY_DAC = 1111
	MEMSIZE_DAC = 28
	MEMTYPE_DAC = "<Iffffff"

	MEMKEY_GL = 2222
	MEMSIZE_GL = 8
	MEMTYPE_GL = "<?xxxi"


# Transmitter Settings
#state
Data_Enable: bool = False
DataSlice_Enable: bool = False
DataBackup_Enable: bool = False
BangBang_Enable: bool = False
BangBang_Set: int = -20 # psi

#TrueStrings = ['t', 'true', 'enable', 'enabled']


class Transmitter(Communication):
	def handle_command(self, command: str, data: str = None):
		super().handle_command(command, data)
		global SharedMemory_GL

		# Transmitter Commands
		try:
			global BangBang_Enable
			global BangBang_Set
			if command == "bangbang_enable":
				if PRINT_INFO: print(f'[{self.COM_TYPE}] \033[92mCommand: bangbang_enable [{data}]\033[00m')
				#global BangBang_Enable
				#global BangBang_Set
				BangBang_Enable = data
				# SharedMemory GL
				writeBytes = struct.pack("<?xxxi", BangBang_Enable, BangBang_Set)
				SharedMemory_GL.write(writeBytes)
				self.write_line(dumps({"type":"commandresponse","command":command,"status":True,"data":BangBang_Enable}))
				
			elif command == "bangbang_set":
				if PRINT_INFO: print(f'[{self.COM_TYPE}] \033[92mCommand: bangbang_set [{data}]\033[00m')
				#global BangBang_Enable
				#global BangBang_Set
				BangBang_Set = data
				# SharedMemory GL
				writeBytes = struct.pack("<?xxxi", BangBang_Enable, BangBang_Set)
				SharedMemory_GL.write(writeBytes)
				self.write_line(dumps({"type":"commandresponse","command":command,"status":True,"data":BangBang_Set}))
				
			elif command == "data_enable":
				if PRINT_INFO: print(f'[{self.COM_TYPE}] \033[92mCommand: data_enable [{data}]\033[00m')
				global Data_Enable
				Data_Enable = data
				self.write_line(dumps({"type":"commandresponse","command":command,"status":True,"data":Data_Enable}))
				
			elif command == "data_enable":
				if PRINT_INFO: print(f'[{self.COM_TYPE}] \033[92mCommand: data_enable [{data}]\033[00m')
				global DataSlice_Enable
				DataSlice_Enable = data
				self.write_line(dumps({"type":"commandresponse","command":command,"status":True,"data":DataSlice_Enable}))
				
			elif command == "databackup_enable":
				if PRINT_INFO: print(f'[{self.COM_TYPE}] \033[92mCommand: databackup_enable [{data}]\033[00m')
				global DataBackup_Enable
				DataBackup_Enable = data
				self.write_line(dumps({"type":"commandresponse","command":command,"status":True,"data":DataBackup_Enable}))

			else:
				if PRINT_DEBUG: print(f'\033[91mUnknown Command {command} [{data}]\033[00m')

		except Exception as exc:
			self.write_line(dumps({"type":"commandresponse","command":command,"status":False,"error":f'{type(exc).__name__}({str(exc)})'}))




# ls /dev/*USB*
# raspi-gpio set 2 op dl
# python Transmitter.py -cwd /dev/ttyUSB0 -cwl /dev/ttyUSB1
# killall python
def main(comWired: str, comWireless: str):
	global SharedMemory_DAC
	global SharedMemory_GL
	com = DualCommunication(comWired, comWireless, Transmitter)
	com.ping(blocking=False)

	try:
		if DAC_ARM:
			SharedMemory_DAC = sysv_ipc.SharedMemory(MEMKEY_DAC, size=MEMSIZE_DAC)
			SharedMemory_GL = sysv_ipc.SharedMemory(MEMKEY_GL, size=MEMSIZE_GL)
		count = 0
		while True: #count <= 1004
			try:
				if DAC_ARM:
					memory_value = SharedMemory_DAC.read()
					value = struct.unpack(MEMTYPE_DAC, memory_value)
					avg = value[0]
					data = list(value[1:6])

					memory_value = struct.pack("<?xxxi", BangBang_Enable, BangBang_Set)
					SharedMemory_GL.write(memory_value)

				else:
					data = [0.,0.,0.,0.,0.,0.]
					avg = int(0)

			except Exception as exc:
				if PRINT_ERROR: print(f"RunDac Failed: {type(exc).__name__}({str(exc)})") # TODO color
			
			else:
				global Data_Enable
				global DataSlice_Enable
				global DataBackup_Enable
				if (Data_Enable or DataSlice_Enable or DataBackup_Enable) and data:
					send = dumps({"type":"data","count":count,"data":{
						"AI_avgcount": avg,
						"AI": [round(num, 2) for num in data]
					}})


					if Data_Enable:
						# Continuous
						com.threadWireless.send(send)
					elif DataSlice_Enable:
						# Single
						com.threadWireless.send(send)
						DataSlice_Enable = False
						com.threadWireless.send(dumps({"type":"commandresponse","command":"dataslice_enable","status":True,"data":False}))
						
					if DataBackup_Enable:
						# Single
						com.threadWired.send(send)
						DataBackup_Enable = False
						com.threadWired.send(dumps({"type":"commandresponse","command":"databackup_enable","status":True,"data":False}))
					

					if count >= 2000000000: count = 0
					else: count += 1

	except KeyboardInterrupt: pass
	except CommunicationError as exc:
		if PRINT_ERROR: print(f'Transmitter Error: {type(exc).__name__}({str(exc)})')
	
	finally:
		print('\033[91mTransmitter Complete\033[00m')
		com.shutdown()
		if DAC_ARM:
			SharedMemory_DAC.detach()
			SharedMemory_GL.detach()


if __name__ == '__main__':
	parser = ArgumentParser()
	parser.add_argument("-cwd", "--comwired", type=str, default="/dev/ttyUSB1", required=False)
	parser.add_argument("-cwl", "--comwireless", type=str, default="/dev/ttyUSB0", required=False)
	args = parser.parse_args()

	main(args.comwired, args.comwireless)