from math import sin, pi
from argparse import ArgumentParser

from time import time, sleep, monotonic

from serial import serial_for_url
#from serial.tools.list_ports import comports
#from serial.tools.list_ports_common import ListPortInfo

from json import dumps, loads, JSONDecodeError

from serial.threaded import ReaderThread, LineReader


PRINT_INFO  = True
PRINT_DEBUG = False
PRINT_ERROR = True

MESSAGE_END = b"\n"
MESSAGE_WIRELESSID = "RDL-D2EB"

BAUD_WIRED = 115200
BAUD_WIRELESS = 56000


class CommunicationError(RuntimeError):
	pass


class Communication(LineReader):
	MESSAGE_ID = ""
	COM_TYPE = "None"
	TERMINATOR = MESSAGE_END

	CONNECTION_CHECK = False

	def connection_made(self, transport):
		super(Communication, self).connection_made(transport)
		sleep(0.1)
		if PRINT_INFO: print(f'[{self.COM_TYPE}] \033[92mport opened\033[00m')

	def write_line(self, text: str):
		if PRINT_DEBUG: print(f'[{self.COM_TYPE}] line \033[94msent\033[00m: {repr(self.MESSAGE_ID + text)}')
		super().write_line(self.MESSAGE_ID + text)

	def handle_line(self, line: str):
		if PRINT_DEBUG: print(f'[{self.COM_TYPE}] raw \033[93mreceived\033[00m: {repr(line)}')
		
		index = line.find(self.MESSAGE_ID)
		if index <= -1: return
		line = line[index + len(self.MESSAGE_ID):]
		
		if PRINT_DEBUG: print(f'[{self.COM_TYPE}] line \033[93mreceived\033[00m: {repr(line)}')

		if line == f'ping {self.COM_TYPE}':
			self.write_line(f'pong {self.COM_TYPE}')
			return
		elif line == f'pong {self.COM_TYPE}':
			if PRINT_INFO: print(f'[{self.COM_TYPE}] \033[92mRecieved Pong\033[00m')
			self.CONNECTION_CHECK = False
			return

		try:
			json = loads(line)
		except JSONDecodeError: pass
		except Exception as exc:
			if PRINT_ERROR: print(f'[{self.COM_TYPE}] \033[91mJSON loads Failed:\033[00m {type(exc).__name__}({str(exc)})')
		else:
			if json:
				self.handle_json(json)

	def connection_lost(self, exc):
		if exc:
			if PRINT_ERROR: print(f'[{self.COM_TYPE}] \033[91mport closed with error:\033[00m {type(exc).__name__}({str(exc)})')
		else:
			if PRINT_INFO: print(f'[{self.COM_TYPE}] \033[91mport closed\033[00m')


	def handle_json(self, json: dict[str, any]):
		if PRINT_DEBUG: print(f'[{self.COM_TYPE}] json \033[93mreceived\033[00m: {dumps(json)}')

		if "type" not in json:
			if PRINT_ERROR: print(f'[{self.COM_TYPE}] [handle_json] NO_MESSAGE_TYPE')
			return

		# Message Types
		if json["type"] == "command":
			if "command" not in json:
				if PRINT_ERROR: print(f'[{self.COM_TYPE}] [handle_json command] NO_COMMAND')
			self.handle_command(json["command"], json.get('data'))

		elif json["type"] == "data":
			if "data" not in json:
				if PRINT_ERROR: print(f'[{self.COM_TYPE}] [handle_json data] NO_DATA')
			self.handle_data(json["data"])

		else:
			if PRINT_ERROR: print(f'[{self.COM_TYPE}] [handle_json] UNKNOWN_MESSAGE_TYPE ({json["type"]})')

	def handle_command(self, command: str, data: str = None):
		if PRINT_INFO:
			if data:
				print(f'Command: {command} [{data}]')
			else:
				print(f'Command: {command}')

	def handle_data(self, data: object):
		if PRINT_DEBUG: print(f'[{self.COM_TYPE}] \033[92mData packet recieved\033[00m')


class CommunicatorThread(ReaderThread):
	protocol: Communication

	def ping(self, timeout: float = 2, blocking: bool = True):
		self.protocol.CONNECTION_CHECK = True
		self.protocol.write_line(f'ping {self.protocol.COM_TYPE}')
		if blocking:
			start = time()
			while self.protocol.CONNECTION_CHECK:
				if time() - start > timeout: raise TimeoutError()
				sleep(0.1)

	def send(self, message: str):
		self.protocol.write_line(message)
		

class DualCommunication():
	def __init__(self, comWired: str, comWireless: str, protocol = Communication):
		try:
			self.threadWired = CommunicatorThread(
				serial_for_url(comWired, baudrate=BAUD_WIRED, timeout=0), protocol)
		except Exception as exc:
			if PRINT_ERROR:
				print(f'\033[91mSerial Wired Failed:\033[00m {type(exc).__name__}({str(exc)})')
			self.threadWired = None
		else:
			self.threadWired.start()
			self.threadWired.protocol.MESSAGE_ID = MESSAGE_WIRELESSID
			self.threadWired.protocol.COM_TYPE = "WIRED"
			self.threadWired.connect()
		
		try:
			self.threadWireless = CommunicatorThread(
				serial_for_url(comWireless, baudrate=BAUD_WIRELESS, timeout=0), protocol)
		except Exception as exc:
			if PRINT_ERROR:
				print(f'\033[91mSerial Wireless Failed:\033[00m {type(exc).__name__}({str(exc)})')
			self.threadWireless = None
		else:
			self.threadWireless.start()
			self.threadWireless.protocol.MESSAGE_ID = MESSAGE_WIRELESSID
			self.threadWireless.protocol.COM_TYPE = "WIRELESS"
			self.threadWireless.connect()

	def shutdown(self):
		if PRINT_INFO: print(f'\033[91mCommunication Shutdown\033[00m')
		if self.threadWired:
			if PRINT_INFO: print(f'[{self.threadWired.protocol.COM_TYPE}] \033[91mCommunication Shutdown\033[00m')
			self.threadWired.close()
			self.threadWired = None
		if self.threadWireless:
			if PRINT_INFO: print(f'[{self.threadWireless.protocol.COM_TYPE}] \033[91mCommunication Shutdown\033[00m')
			self.threadWireless.close()
			self.threadWireless = None

	def __del__(self):
		self.shutdown()

	def ping(self, timeout: int = 2, blocking: bool = True):
		if self.threadWired: self.threadWired.ping(timeout, blocking)
		if self.threadWireless: self.threadWireless.ping(timeout, blocking)




if __name__ == "__main__":
	parser = ArgumentParser()
	parser.add_argument("-cwd", "--comwired", type=str, default="COM4", required=False)
	parser.add_argument("-cwl", "--comwireless", type=str, default="COM8", required=False)
	parser.add_argument("-t", "--transmitter", action='store_true')
	args = parser.parse_args()

	com = DualCommunication(args.comwired, args.comwireless)

	if not args.transmitter:
		try:
			count = 0
			while count <= 1004:#(com.aliveWired() or com.aliveWireless())
				sleep(0.1) # Data collect

				com.threadWireless.send(dumps({"type":"data","count":count,"data":{
					"BangBangSet": 200,
					"AI": [round(sin(count/pi),2), round(sin((count+3)/pi),2), round(sin((count+5)/pi),2), round(sin((count+7)/pi),2), round(sin((count+10)/pi),2)]
				}}))

				#if count > 5:
				#	try:
				#		if not count % 100:
				#			com.threadWired.response()
				#			sleep(0.01)
				#	except CommunicationError as exc:
				#		if PRINT_ERROR: print(exc)
				#	
				#	try:
				#		if not (count + 50) % 100:
				#			com.threadWireless.response()
				#			sleep(0.01)
				#	except CommunicationError as exc:
				#		if PRINT_ERROR: print(exc)

				count += 1
			print('\033[91mTransmitter Complete\033[00m')
		except CommunicationError as exc:
			if PRINT_ERROR: print(f'Transmitter Error: {type(exc).__name__}({str(exc)})')

	else:
		if PRINT_INFO: print('\033[92mGround Station Active\033[00m')
		try:
			while True:#(com.threadWired.protocol.transport.alive or com.threadWireless.protocol.transport.alive):
				sleep(1)
		except KeyboardInterrupt: pass
		if PRINT_INFO: print('\033[91mGround Station Shutdown\033[00m')
		com.shutdown()